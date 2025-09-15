/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#define _POSIX_C_SOURCE 200809L
#if defined(__APPLE__)
#define _DARWIN_C_SOURCE
#else
#define _GNU_SOURCE
#endif

#include <git2.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <mach-o/dyld.h>
#endif

#include "gitmind/cache.h"
#include "gitmind/cache/bitmap.h"
#include "cache_internal.h"
#include "gitmind/constants.h"
#include "gitmind/constants_internal.h"
#include "gitmind/context.h"
#include "gitmind/edge.h"
#include "gitmind/error.h"
#include "gitmind/journal.h"
#include "gitmind/types.h"
#include "gitmind/security/memory.h"
#include "gitmind/security/string.h"
#include "gitmind/util/ref.h"
#include "gitmind/util/memory.h"

/* Local constants */
#define CACHE_TEMP_DIR "/tmp/gitmind-cache-\x58\x58\x58\x58\x58\x58"
#define MAX_SHARD_PATH 32
#define CLOCKS_PER_MS (CLOCKS_PER_SEC / (clock_t)MILLIS_PER_SECOND)
#define OID_HASH_MULTIPLIER 31

/* In-memory edge ID map entry */
typedef struct edge_map_entry {
    gm_oid_t oid;
    roaring_bitmap_t *bitmap;
    struct edge_map_entry *next;
} edge_map_entry_t;

/* Edge ID map (simple hash table) */
typedef struct {
    edge_map_entry_t **buckets;
    size_t size;
} edge_map_t;

/* Create edge map */
static edge_map_t *edge_map_create(size_t size) {
    edge_map_t *map = calloc(1, sizeof(edge_map_t));
    if (!map)
        return NULL;

    map->buckets = calloc(size, sizeof(edge_map_entry_t *));
    if (!map->buckets) {
        free(map);
        return NULL;
    }

    map->size = size;
    return map;
}

/* Hash function for OID: lightweight mixing for better distribution */
static size_t oid_hash(const gm_oid_t *oid, size_t size) {
    const uint8_t *raw = (const uint8_t *)oid->id;
    uint32_t x = 0x9E3779B9u; /* golden ratio */
    for (int i = 0; i < GM_OID_RAWSZ; i++) {
        x ^= raw[i];
        x *= 0x85EBCA6Bu;      /* mix */
        x ^= (x >> 13);
    }
    /* final avalanche */
    x ^= x >> 16;
    x *= 0x7FEB352Du;
    x ^= x >> 15;
    return (size_t)(x % (uint32_t)size);
}

/* Add edge ID to map */
static int edge_map_add(edge_map_t *map, const gm_oid_t *oid, uint32_t edge_id) {
    size_t idx = oid_hash(oid, map->size);
    edge_map_entry_t *entry = map->buckets[idx];

    /* Find existing entry */
    while (entry) {
        if (git_oid_cmp(&entry->oid, oid) == 0) {
            gm_bitmap_add(entry->bitmap, edge_id);
            return GM_OK;
        }
        entry = entry->next;
    }

    /* Create new entry */
    entry = calloc(1, sizeof(edge_map_entry_t));
    if (!entry)
        return GM_ERR_OUT_OF_MEMORY;

    entry->oid = *oid;
    entry->bitmap = gm_bitmap_create();
    if (!entry->bitmap) {
        free(entry);
        return GM_ERR_OUT_OF_MEMORY;
    }

    gm_bitmap_add(entry->bitmap, edge_id);
    entry->next = map->buckets[idx];
    map->buckets[idx] = entry;

    return GM_OK;
}

/* Free edge map */
static void edge_map_free(edge_map_t *map) {
    if (!map)
        return;

    for (size_t i = 0; i < map->size; i++) {
        edge_map_entry_t *entry = map->buckets[i];
        while (entry) {
            edge_map_entry_t *next = entry->next;
            gm_bitmap_free(entry->bitmap);
            free(entry);
            entry = next;
        }
    }

    free(map->buckets);
    free(map);
}

/* Get SHA prefix for sharding */
static void get_oid_prefix(const gm_oid_t *oid, char *prefix, int bits) {
    int chars = (bits + 3) / BITS_PER_HEX_CHAR; /* Round up to hex chars */
    if (chars <= 0) { prefix[0] = '\0'; return; }
    char hex[GM_OID_HEX_CHARS];
    git_oid_fmt(hex, oid); /* not null-terminated */
    if (chars > GM_OID_HEX_CHARS) chars = GM_OID_HEX_CHARS;
    for (int i = 0; i < chars && i < MAX_SHARD_PATH - 1; i++) prefix[i] = hex[i];
    prefix[(chars < (MAX_SHARD_PATH - 1)) ? chars : (MAX_SHARD_PATH - 1)] = '\0';
}

static int oid_to_hex(const gm_oid_t *oid, char *out, size_t out_size) {
    if (out_size < SHA_HEX_SIZE) return GM_ERR_INVALID_ARGUMENT;
    (void)git_oid_tostr(out, out_size, oid);
    return GM_OK;
}

/* Write bitmaps to temp directory */
static int write_map_to_temp(edge_map_t *map, const char *temp_dir,
                             int shard_bits, const char *suffix) {
    char path[GM_PATH_MAX];
    for (size_t i = 0; i < map->size; i++) {
        edge_map_entry_t *entry = map->buckets[i];
        while (entry) {
            char prefix[MAX_SHARD_PATH];
            get_oid_prefix(&entry->oid, prefix, shard_bits);
            (void)gm_snprintf(path, sizeof(path), "%s/%s", temp_dir, prefix);
            (void)mkdir(path, DIR_PERMS_NORMAL);

            char sha_hex[SHA_HEX_SIZE];
            int hexrc = oid_to_hex(&entry->oid, sha_hex, sizeof(sha_hex));
            if (hexrc != GM_OK) {
                return hexrc;
            }
            (void)gm_snprintf(path, sizeof(path), "%s/%s/%s.%s", temp_dir,
                              prefix, sha_hex, suffix);
            int status = gm_bitmap_write_file(entry->bitmap, path);
            if (status != GM_OK) {
                return status;
            }
            entry = entry->next;
        }
    }
    return GM_OK;
}

static int write_bitmaps_to_temp(edge_map_t *forward, edge_map_t *reverse,
                                 const char *temp_dir, int shard_bits) {
    int status = write_map_to_temp(forward, temp_dir, shard_bits, "forward");
    if (status != GM_OK) return status;
    return write_map_to_temp(reverse, temp_dir, shard_bits, "reverse");
}

/* Forward declarations */
/* Declared in public headers */
static int gm_cache_rebuild_internal(git_repository *repo, const char *branch,
                                     bool force_full);

/* Build Git tree from temp directory */
static int build_tree_from_temp(git_repository *repo, const char *temp_dir,
                                git_oid *tree_oid) {
    /* Use the full tree builder implementation */
    return gm_build_tree_from_directory(repo, temp_dir, tree_oid);
}

/* Remove temp directory recursively */
#include <ftw.h>
static int unlink_cb(const char *fpath, const struct stat *sb,
                     int typeflag, struct FTW *ftwbuf) {
    (void)sb; (void)ftwbuf;
    if (typeflag == FTW_DP || typeflag == FTW_D) {
        return rmdir(fpath);
    }
    return unlink(fpath);
}
static void remove_temp_dir(const char *path) {
    (void)nftw(path, unlink_cb, 16, FTW_DEPTH | FTW_PHYS);
}

/* Edge collector callback */
typedef struct {
    edge_map_t *forward;
    edge_map_t *reverse;
    uint32_t edge_id;
} edge_collector_t;

static int collect_edge_callback(const gm_edge_t *edge, void *userdata) {
    edge_collector_t *collector = (edge_collector_t *)userdata;
    int rc;

    /* Add to forward index */
    rc = edge_map_add(collector->forward, &edge->src_oid, collector->edge_id);
    if (rc != GM_OK)
        return rc;

    /* Add to reverse index */
    rc = edge_map_add(collector->reverse, &edge->tgt_oid, collector->edge_id);
    if (rc != GM_OK)
        return rc;

    collector->edge_id++;
    return GM_OK;
}

/* Helper functions for cache rebuild */
static int cache_prepare_rebuild(gm_context_t *ctx,
                                 const char *branch __attribute__((unused)),
                                 bool force_full, gm_cache_meta_t *old_meta,
                                 bool *has_old_cache, char *temp_dir) {
    /* Load existing cache metadata if not forcing full rebuild */
    *has_old_cache = false;
    if (!force_full) {
        int rc = gm_cache_load_meta(ctx, branch, old_meta);
        *has_old_cache = (rc == GM_OK);
    }

    /* Create temp directory */
    (void)gm_snprintf(temp_dir, GM_PATH_MAX, "%s", CACHE_TEMP_DIR);
    if (!mkdtemp(temp_dir)) {
        return GM_ERR_IO_FAILED;
    }

    return GM_OK;
}

static int cache_collect_edges(gm_context_t *ctx, const char *branch,
                               edge_map_t *forward, edge_map_t *reverse,
                               uint32_t starting_edge_id,
                               uint32_t *total_edges) {
    edge_collector_t collector = {
        .forward = forward, .reverse = reverse, .edge_id = starting_edge_id};

    int rc = gm_journal_read(ctx, branch, collect_edge_callback, &collector);
    if (rc == GM_OK) {
        *total_edges = collector.edge_id;
    }
    return rc;
}

static int cache_build_edge_map(edge_map_t **forward, edge_map_t **reverse) {
    *forward = edge_map_create(EDGE_MAP_BUCKETS);
    *reverse = edge_map_create(EDGE_MAP_BUCKETS);

    if (!*forward || !*reverse) {
        edge_map_free(*forward);
        edge_map_free(*reverse);
        return GM_ERR_OUT_OF_MEMORY;
    }

    return GM_OK;
}

static int cache_write_bitmaps(edge_map_t *forward, edge_map_t *reverse,
                               const char *temp_dir) {
    return write_bitmaps_to_temp(forward, reverse, temp_dir,
                                 GM_CACHE_SHARD_BITS);
}

static int cache_build_trees(git_repository *repo, const char *temp_dir,
                             git_oid *tree_oid) {
    return build_tree_from_temp(repo, temp_dir, tree_oid);
}

static int cache_get_journal_tip(git_repository *repo, const char *branch,
                                 gm_cache_meta_t *meta) {
    git_reference *journal_ref = NULL;
    char journal_ref_name[REF_NAME_BUFFER_SIZE];
    {
        int brc = gm_build_ref(journal_ref_name, sizeof(journal_ref_name),
                               GITMIND_EDGES_REF_PREFIX, branch);
        if (brc != GM_OK) {
            return brc;
        }
    }

    if (git_reference_lookup(&journal_ref, repo, journal_ref_name) == 0) {
        const git_oid *tip_oid = git_reference_target(journal_ref);
        if (tip_oid) {
            git_oid_tostr(meta->journal_tip_oid, sizeof(meta->journal_tip_oid),
                          tip_oid);
            meta->journal_tip_oid_bin = *tip_oid;

            /* Also get the timestamp of the tip commit */
            git_commit *tip_commit = NULL;
            if (git_commit_lookup(&tip_commit, repo, tip_oid) == 0) {
                meta->journal_tip_time = (uint64_t)git_commit_time(tip_commit);
                git_commit_free(tip_commit);
            }
        }
        git_reference_free(journal_ref);
    } else {
        /* No journal yet */
        meta->journal_tip_oid[0] = '\0';
        gm_memset_safe(&meta->journal_tip_oid_bin, sizeof(meta->journal_tip_oid_bin), 0, sizeof(meta->journal_tip_oid_bin));
    }

    return GM_OK;
}

static int cache_create_commit(git_repository *repo, const git_oid *tree_oid,
                               const gm_cache_meta_t *meta
                               __attribute__((unused)),
                               git_oid *commit_oid) {
    git_signature *sig = NULL;
    git_tree *tree = NULL;
    git_odb *odb = NULL;
    int rc;

    /* Create signature */
    rc = git_signature_default(&sig, repo);
    if (rc < 0)
        return GM_ERR_UNKNOWN;

    /* Lookup tree */
    rc = git_tree_lookup(&tree, repo, tree_oid);
    if (rc < 0) {
        git_signature_free(sig);
        return GM_ERR_UNKNOWN;
    }

    /* Create commit buffer */
    git_buf commit_buf = {0};
    rc = git_commit_create_buffer(&commit_buf, repo, sig, sig,
                                  NULL,             /* encoding */
                                  "Cache metadata", /* message */
                                  tree, 0, NULL);

    if (rc == 0) {
        /* Write commit object */
        rc = git_repository_odb(&odb, repo);
        if (rc == 0) {
            rc = git_odb_write(commit_oid, odb, commit_buf.ptr, commit_buf.size,
                               GIT_OBJECT_COMMIT);
            git_odb_free(odb);
        }
        git_buf_dispose(&commit_buf);
    }

    git_tree_free(tree);
    git_signature_free(sig);

    return (rc < 0) ? GM_ERR_UNKNOWN : GM_OK;
}

static int cache_update_ref(git_repository *repo, const char *branch,
                            const git_oid *commit_oid) {
    char ref_name[REF_NAME_BUFFER_SIZE];
    {
        int brc = gm_build_ref(ref_name, sizeof(ref_name), GM_CACHE_REF_PREFIX, branch);
        if (brc != GM_OK) {
            return brc;
        }
    }

    git_reference *ref = NULL;
    int rc = git_reference_create(&ref, repo, ref_name, commit_oid, 1,
                                  "Cache rebuild");
    if (ref)
        git_reference_free(ref);

    return (rc < 0) ? GM_ERR_UNKNOWN : GM_OK;
}

static void cache_cleanup(edge_map_t *forward, edge_map_t *reverse,
                          const char *temp_dir) {
    edge_map_free(forward);
    edge_map_free(reverse);
    remove_temp_dir(temp_dir);
}

/* Internal cache rebuild function */
static int gm_cache_rebuild_internal(git_repository *repo, const char *branch,
                                     bool force_full) {
    edge_map_t *forward = NULL;
    edge_map_t *reverse = NULL;
    char temp_dir[GM_PATH_MAX];
    git_oid tree_oid, commit_oid;
    gm_cache_meta_t old_meta = {0};
    gm_cache_meta_t meta = {0};
    bool has_old_cache = false;
    int rc = GM_OK;
    clock_t start_time;

    /* Create context */
    gm_context_t ctx = {0};
    ctx.git_repo = repo;

    /* Prepare for rebuild */
    rc = cache_prepare_rebuild(&ctx, branch, force_full, &old_meta,
                               &has_old_cache, temp_dir);
    if (rc != GM_OK)
        return rc;

    /* Create edge maps */
    rc = cache_build_edge_map(&forward, &reverse);
    if (rc != GM_OK) {
        remove_temp_dir(temp_dir);
        return rc;
    }

    /* Collect edges from journal */
    start_time = clock();
    uint32_t starting_edge_id =
        has_old_cache ? (uint32_t)old_meta.edge_count : 0;
    uint32_t total_edges = 0;

    rc = cache_collect_edges(&ctx, branch, forward, reverse, starting_edge_id,
                             &total_edges);
    if (rc != GM_OK)
        goto cleanup;

    /* Write bitmaps to disk */
    rc = cache_write_bitmaps(forward, reverse, temp_dir);
    if (rc != GM_OK)
        goto cleanup;

    /* Build Git tree */
    rc = cache_build_trees(repo, temp_dir, &tree_oid);
    if (rc != GM_OK)
        goto cleanup;

    /* Create metadata */
    meta.journal_tip_time = (uint64_t)time(NULL);
    meta.edge_count = total_edges;
    meta.build_time_ms = (uint64_t)((clock() - start_time) / CLOCKS_PER_MS);
    meta.shard_bits = GM_CACHE_SHARD_BITS;
    meta.version = GM_CACHE_VERSION;
    (void)gm_strcpy_safe(meta.branch, GM_CACHE_BRANCH_NAME_SIZE, branch);

    /* Get journal tip info */
    rc = cache_get_journal_tip(repo, branch, &meta);
    if (rc != GM_OK)
        goto cleanup;

    /* Create cache commit */
    rc = cache_create_commit(repo, &tree_oid, &meta, &commit_oid);
    if (rc != GM_OK)
        goto cleanup;

    /* Update cache ref */
    rc = cache_update_ref(repo, branch, &commit_oid);

cleanup:
    cache_cleanup(forward, reverse, temp_dir);
    return rc;
}

/* Public cache rebuild function matching cache.h signature */
int gm_cache_rebuild(gm_context_t *ctx, const char *branch, bool force_full) {
    if (!ctx || !ctx->git_repo || !branch) {
        return GM_ERR_INVALID_ARGUMENT;
    }
    return gm_cache_rebuild_internal((git_repository *)ctx->git_repo, 
                                     branch, force_full);
}
