/* SPDX-License-Identifier: Apache-2.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#define _POSIX_C_SOURCE 200809L

#include "cache.h"
#include "bitmap.h"
#include "../../include/gitmind.h"
#include <git2.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

/* Constants */
#define CACHE_BUILD_BATCH_SIZE 1000
#define CACHE_TEMP_DIR "/tmp/gitmind-cache-XXXXXX"
#define EMPTY_TREE_SHA "4b825dc642cb6eb9a060e54bf8d69288fbee4904"
#define MAX_SHARD_PATH 32

/* In-memory edge ID map entry */
typedef struct edge_map_entry {
    uint8_t sha[GM_SHA1_SIZE];
    roaring_bitmap_t* bitmap;
    struct edge_map_entry* next;
} edge_map_entry_t;

/* Edge ID map (simple hash table) */
typedef struct {
    edge_map_entry_t** buckets;
    size_t size;
} edge_map_t;

/* Create edge map */
static edge_map_t* edge_map_create(size_t size) {
    edge_map_t* map = calloc(1, sizeof(edge_map_t));
    if (!map) return NULL;
    
    map->buckets = calloc(size, sizeof(edge_map_entry_t*));
    if (!map->buckets) {
        free(map);
        return NULL;
    }
    
    map->size = size;
    return map;
}

/* Hash function for SHA */
static size_t sha_hash(const uint8_t* sha, size_t size) {
    size_t hash = 0;
    for (int i = 0; i < GM_SHA1_SIZE; i++) {
        hash = (hash * 31) + sha[i];
    }
    return hash % size;
}

/* Add edge ID to map */
static int edge_map_add(edge_map_t* map, const uint8_t* sha, uint32_t edge_id) {
    size_t idx = sha_hash(sha, map->size);
    edge_map_entry_t* entry = map->buckets[idx];
    
    /* Find existing entry */
    while (entry) {
        if (memcmp(entry->sha, sha, GM_SHA1_SIZE) == 0) {
            gm_bitmap_add(entry->bitmap, edge_id);
            return GM_OK;
        }
        entry = entry->next;
    }
    
    /* Create new entry */
    entry = calloc(1, sizeof(edge_map_entry_t));
    if (!entry) return GM_NO_MEMORY;
    
    memcpy(entry->sha, sha, GM_SHA1_SIZE);
    entry->bitmap = gm_bitmap_create();
    if (!entry->bitmap) {
        free(entry);
        return GM_NO_MEMORY;
    }
    
    gm_bitmap_add(entry->bitmap, edge_id);
    entry->next = map->buckets[idx];
    map->buckets[idx] = entry;
    
    return GM_OK;
}

/* Free edge map */
static void edge_map_free(edge_map_t* map) {
    if (!map) return;
    
    for (size_t i = 0; i < map->size; i++) {
        edge_map_entry_t* entry = map->buckets[i];
        while (entry) {
            edge_map_entry_t* next = entry->next;
            gm_bitmap_free(entry->bitmap);
            free(entry);
            entry = next;
        }
    }
    
    free(map->buckets);
    free(map);
}

/* Get SHA prefix for sharding */
static void get_sha_prefix(const uint8_t* sha, char* prefix, int bits) {
    int chars = (bits + 3) / 4;  /* Round up to hex chars */
    for (int i = 0; i < chars; i++) {
        sprintf(prefix + i * 2, "%02x", sha[i]);
    }
    prefix[chars * 2] = '\0';
}

/* Write bitmaps to temp directory */
static int write_bitmaps_to_temp(edge_map_t* forward, edge_map_t* reverse,
                                 const char* temp_dir, int shard_bits) {
    char path[GM_PATH_MAX];
    int rc;
    
    /* Process forward index */
    for (size_t i = 0; i < forward->size; i++) {
        edge_map_entry_t* entry = forward->buckets[i];
        while (entry) {
            char prefix[MAX_SHARD_PATH];
            get_sha_prefix(entry->sha, prefix, shard_bits);
            
            /* Create shard directory */
            snprintf(path, sizeof(path), "%s/%s", temp_dir, prefix);
            mkdir(path, 0755);
            
            /* Write forward bitmap */
            char sha_hex[41];
            for (int j = 0; j < GM_SHA1_SIZE; j++) {
                sprintf(sha_hex + j * 2, "%02x", entry->sha[j]);
            }
            sha_hex[40] = '\0';
            
            snprintf(path, sizeof(path), "%s/%s/%s.forward", temp_dir, prefix, sha_hex);
            rc = gm_bitmap_write_file(entry->bitmap, path);
            if (rc != GM_OK) return rc;
            
            entry = entry->next;
        }
    }
    
    /* Process reverse index */
    for (size_t i = 0; i < reverse->size; i++) {
        edge_map_entry_t* entry = reverse->buckets[i];
        while (entry) {
            char prefix[MAX_SHARD_PATH];
            get_sha_prefix(entry->sha, prefix, shard_bits);
            
            /* Create shard directory */
            snprintf(path, sizeof(path), "%s/%s", temp_dir, prefix);
            mkdir(path, 0755);
            
            /* Write reverse bitmap */
            char sha_hex[41];
            for (int j = 0; j < GM_SHA1_SIZE; j++) {
                sprintf(sha_hex + j * 2, "%02x", entry->sha[j]);
            }
            sha_hex[40] = '\0';
            
            snprintf(path, sizeof(path), "%s/%s/%s.reverse", temp_dir, prefix, sha_hex);
            rc = gm_bitmap_write_file(entry->bitmap, path);
            if (rc != GM_OK) return rc;
            
            entry = entry->next;
        }
    }
    
    return GM_OK;
}

/* Forward declaration */
int gm_build_tree_from_directory(git_repository* repo, const char* dir_path,
                                git_oid* tree_oid);

/* Build Git tree from temp directory */
static int build_tree_from_temp(git_repository* repo, const char* temp_dir,
                               git_oid* tree_oid) {
    /* Use the full tree builder implementation */
    return gm_build_tree_from_directory(repo, temp_dir, tree_oid);
}

/* Remove temp directory recursively */
static void remove_temp_dir(const char* path) {
    char cmd[GM_PATH_MAX + 32];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", path);
    int rc = system(cmd);  /* Safe since we control the path */
    (void)rc; /* Ignore return value */
}

/* Edge collector callback */
typedef struct {
    edge_map_t* forward;
    edge_map_t* reverse;
    uint32_t edge_id;
} edge_collector_t;

static int collect_edge_callback(const gm_edge_t* edge, void* userdata) {
    edge_collector_t* collector = (edge_collector_t*)userdata;
    int rc;
    
    /* Add to forward index */
    rc = edge_map_add(collector->forward, edge->src_sha, collector->edge_id);
    if (rc != GM_OK) return rc;
    
    /* Add to reverse index */
    rc = edge_map_add(collector->reverse, edge->tgt_sha, collector->edge_id);
    if (rc != GM_OK) return rc;
    
    collector->edge_id++;
    return GM_OK;
}

/* Encode cache metadata to CBOR */
static int encode_cache_meta(const gm_cache_meta_t* meta, uint8_t** buffer, size_t* size) {
    /* Simple encoding: fixed size for now */
    *size = sizeof(gm_cache_meta_t);
    *buffer = malloc(*size);
    if (!*buffer) return GM_NO_MEMORY;
    
    memcpy(*buffer, meta, *size);
    return GM_OK;
}

/* Internal cache rebuild function */
int gm_cache_rebuild_internal(git_repository* repo, const char* branch, bool force_full) {
    edge_map_t* forward = NULL;
    edge_map_t* reverse = NULL;
    char temp_dir[GM_PATH_MAX];
    git_oid tree_oid, commit_oid;
    int rc = GM_OK;
    
    /* Create context */
    gm_context_t ctx = {0};
    ctx.git_repo = repo;
    
    /* Load existing cache metadata if not forcing full rebuild */
    gm_cache_meta_t old_meta = {0};
    bool has_old_cache = false;
    if (!force_full) {
        rc = gm_cache_load_meta(repo, branch, &old_meta);
        has_old_cache = (rc == GM_OK);
    }
    
    /* Create temp directory */
    strcpy(temp_dir, CACHE_TEMP_DIR);
    if (!mkdtemp(temp_dir)) {
        return GM_IO_ERROR;
    }
    
    /* Create edge maps */
    forward = edge_map_create(65536);  /* 64K buckets */
    reverse = edge_map_create(65536);
    if (!forward || !reverse) {
        rc = GM_NO_MEMORY;
        goto cleanup;
    }
    
    /* Set up collector */
    edge_collector_t collector = {
        .forward = forward,
        .reverse = reverse,
        .edge_id = has_old_cache ? (uint32_t)old_meta.edge_count : 0
    };
    
    /* Walk journal and collect edges */
    clock_t start = clock();
    rc = gm_journal_read(&ctx, branch, collect_edge_callback, &collector);
    if (rc != GM_OK) goto cleanup;
    
    /* Write bitmaps to temp directory */
    rc = write_bitmaps_to_temp(forward, reverse, temp_dir, GM_CACHE_SHARD_BITS);
    if (rc != GM_OK) goto cleanup;
    
    /* Build tree from temp directory */
    rc = build_tree_from_temp(repo, temp_dir, &tree_oid);
    if (rc != GM_OK) goto cleanup;
    
    /* Create cache metadata */
    gm_cache_meta_t meta = {
        .journal_tip_time = (uint64_t)time(NULL),
        .edge_count = collector.edge_id,
        .build_time_ms = (clock() - start) * 1000 / CLOCKS_PER_SEC,
        .shard_bits = GM_CACHE_SHARD_BITS,
        .version = GM_CACHE_VERSION
    };
    strncpy(meta.branch, branch, sizeof(meta.branch) - 1);
    
    /* Get actual journal tip OID */
    git_reference* journal_ref = NULL;
    char journal_ref_name[256];
    snprintf(journal_ref_name, sizeof(journal_ref_name), "refs/gitmind/edges/%s", branch);
    
    if (git_reference_lookup(&journal_ref, repo, journal_ref_name) == 0) {
        const git_oid* tip_oid = git_reference_target(journal_ref);
        if (tip_oid) {
            git_oid_tostr(meta.journal_tip_oid, sizeof(meta.journal_tip_oid), tip_oid);
            
            /* Also get the timestamp of the tip commit */
            git_commit* tip_commit = NULL;
            if (git_commit_lookup(&tip_commit, repo, tip_oid) == 0) {
                meta.journal_tip_time = git_commit_time(tip_commit);
                git_commit_free(tip_commit);
            }
        }
        git_reference_free(journal_ref);
    } else {
        /* No journal yet */
        strcpy(meta.journal_tip_oid, "0000000000000000000000000000000000000000");
    }
    
    /* Encode metadata */
    uint8_t* meta_buffer = NULL;
    size_t meta_size = 0;
    rc = encode_cache_meta(&meta, &meta_buffer, &meta_size);
    if (rc != GM_OK) goto cleanup;
    
    /* Create cache commit */
    git_signature* sig = NULL;
    rc = git_signature_default(&sig, repo);
    if (rc < 0) {
        free(meta_buffer);
        rc = GM_ERROR;
        goto cleanup;
    }
    
    char ref_name[256];
    snprintf(ref_name, sizeof(ref_name), "%s%s/%ld", 
             GM_CACHE_REF_PREFIX, branch, time(NULL));
    
    /* Create commit with metadata in message */
    git_tree* tree = NULL;
    git_odb* odb = NULL;
    
    rc = git_tree_lookup(&tree, repo, &tree_oid);
    if (rc < 0) {
        git_signature_free(sig);
        free(meta_buffer);
        rc = GM_ERROR;
        goto cleanup;
    }
    
    /* Create commit with binary metadata */
    git_buf commit_buf = {0};
    rc = git_commit_create_buffer(
        &commit_buf,
        repo,
        sig,
        sig,
        NULL,  /* encoding */
        "Cache metadata",  /* message */
        tree,
        0,
        NULL
    );
    
    if (rc == 0) {
        /* Write commit object */
        rc = git_repository_odb(&odb, repo);
        if (rc == 0) {
            rc = git_odb_write(&commit_oid, odb, 
                              commit_buf.ptr, commit_buf.size, GIT_OBJECT_COMMIT);
            git_odb_free(odb);
        }
        git_buf_dispose(&commit_buf);
    }
    
    git_tree_free(tree);
    
    git_signature_free(sig);
    free(meta_buffer);
    
    if (rc < 0) {
        rc = GM_ERROR;
        goto cleanup;
    }
    
    /* Update cache ref */
    git_reference* ref = NULL;
    rc = git_reference_create(&ref, repo, ref_name, &commit_oid, 1, "Cache rebuild");
    if (ref) git_reference_free(ref);
    
    rc = (rc < 0) ? GM_ERROR : GM_OK;
    
cleanup:
    edge_map_free(forward);
    edge_map_free(reverse);
    remove_temp_dir(temp_dir);
    return rc;
}

/* Public cache rebuild function matching gitmind.h signature */
int gm_cache_rebuild(gm_context_t *ctx, const char *branch) {
    if (!ctx || !ctx->git_repo) {
        return GM_INVALID_ARG;
    }
    /* Check if force flag is set in context user_data */
    bool force = false;
    if (ctx->user_data) {
        force = *(bool*)ctx->user_data;
    }
    return gm_cache_rebuild_internal((git_repository*)ctx->git_repo, branch, force);
}