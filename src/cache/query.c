/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#define _POSIX_C_SOURCE 200809L

#include <git2.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../../include/gitmind.h"
#include "../../include/gitmind/constants_internal.h"
#include "bitmap.h"
#include "cache.h"

/* Constants */
#define CACHE_MAX_AGE_SECONDS 3600 /* 1 hour */
#define MAX_EDGE_IDS 100000        /* Safety limit */
#define SHA_PREFIX_BUFFER_SIZE 16  /* Buffer for SHA prefix */
#define CACHE_PATH_BUFFER_SIZE 128 /* Buffer for cache paths */
#define INITIAL_EDGE_CAPACITY 100  /* Initial allocation for edge arrays */

/* Get SHA prefix for sharding */
static void get_sha_prefix(const uint8_t *sha, char *prefix, int bits) {
    int chars = (bits + 3) / BITS_PER_HEX_CHAR; /* Round up to hex chars */
    for (int i = 0; i < chars; i++) {
        sprintf(prefix + i * HEX_CHARS_PER_BYTE, "%02x", sha[i]);
    }
    prefix[chars * HEX_CHARS_PER_BYTE] = '\0';
}

/* Load cache metadata from commit message */
int gm_cache_load_meta(git_repository *repo, const char *branch,
                       gm_cache_meta_t *meta) {
    git_reference *ref = NULL;
    git_commit *commit = NULL;
    char ref_name[REF_NAME_BUFFER_SIZE];
    int rc;

    /* Build cache ref name */
    snprintf(ref_name, sizeof(ref_name), "%s%s", GM_CACHE_REF_PREFIX, branch);

    /* Look up cache reference */
    rc = git_reference_lookup(&ref, repo, ref_name);
    if (rc < 0)
        return GM_NOT_FOUND;

    /* Get commit */
    git_oid oid;
    rc = git_reference_name_to_id(&oid, repo, ref_name);
    git_reference_free(ref);
    if (rc < 0)
        return GM_NOT_FOUND;

    rc = git_commit_lookup(&commit, repo, &oid);
    if (rc < 0)
        return GM_ERROR;

    /* Parse metadata from commit message */
    const char *msg = git_commit_message(commit);
    if (!msg || strlen(msg) < sizeof(gm_cache_meta_t)) {
        git_commit_free(commit);
        return GM_ERROR;
    }

    memcpy(meta, msg, sizeof(gm_cache_meta_t));
    git_commit_free(commit);

    return GM_OK;
}

/* Check if cache is stale */
bool gm_cache_is_stale(git_repository *repo, const char *branch) {
    gm_cache_meta_t meta;

    /* Try to load cache metadata */
    if (gm_cache_load_meta(repo, branch, &meta) != GM_OK) {
        return true; /* No cache = stale */
    }

    /* Check age */
    time_t now = time(NULL);
    if (now - meta.journal_tip_time > CACHE_MAX_AGE_SECONDS) {
        return true;
    }

    /* Check if journal has new commits since cache was built */
    git_reference *journal_ref = NULL;
    char journal_ref_name[REF_NAME_BUFFER_SIZE];
    snprintf(journal_ref_name, sizeof(journal_ref_name),
             "refs/gitmind/edges/%s", branch);

    if (git_reference_lookup(&journal_ref, repo, journal_ref_name) == 0) {
        const git_oid *current_tip = git_reference_target(journal_ref);
        if (current_tip) {
            /* Compare with cached tip OID */
            git_oid cached_tip;
            if (git_oid_fromstr(&cached_tip, meta.journal_tip_oid) == 0) {
                /* If tips don't match, cache is stale */
                if (git_oid_cmp(current_tip, &cached_tip) != 0) {
                    git_reference_free(journal_ref);
                    return true;
                }
            }
        }
        git_reference_free(journal_ref);
    }

    return false;
}

/* Load bitmap from cache tree */
static int load_bitmap_from_cache(git_repository *repo, git_tree *tree,
                                  const uint8_t *sha, const char *suffix,
                                  roaring_bitmap_t **bitmap) {
    char prefix[SHA_PREFIX_BUFFER_SIZE];
    char sha_hex[SHA_HEX_SIZE];
    char path[CACHE_PATH_BUFFER_SIZE];
    git_tree_entry *entry = NULL;
    git_blob *blob = NULL;
    int rc;

    /* Get shard prefix */
    get_sha_prefix(sha, prefix, GM_CACHE_SHARD_BITS);

    /* Convert SHA to hex */
    for (int i = 0; i < GM_SHA1_SIZE; i++) {
        sprintf(sha_hex + i * HEX_CHARS_PER_BYTE, "%02x", sha[i]);
    }
    sha_hex[GM_SHA1_SIZE * HEX_CHARS_PER_BYTE] = '\0';

    /* Build path: prefix/sha.suffix */
    snprintf(path, sizeof(path), "%s/%s.%s", prefix, sha_hex, suffix);

    /* Look up tree entry */
    rc = git_tree_entry_bypath(&entry, tree, path);
    if (rc < 0)
        return GM_NOT_FOUND;

    /* Get blob */
    rc = git_blob_lookup(&blob, repo, git_tree_entry_id(entry));
    git_tree_entry_free(entry);
    if (rc < 0)
        return GM_ERROR;

    /* Deserialize bitmap */
    const void *data = git_blob_rawcontent(blob);
    size_t size = git_blob_rawsize(blob);

    rc = gm_bitmap_deserialize(data, size, bitmap);
    git_blob_free(blob);

    return rc;
}

/* Journal scan fallback */
typedef struct {
    const uint8_t *target_sha;
    gm_edge_t *edges;
    size_t capacity;
    size_t count;
    int check_source; /* 1 for fanout (check src), 0 for fanin (check tgt) */
} journal_scan_state_t;

static int journal_scan_callback_generic(const gm_edge_t *edge,
                                         void *userdata) {
    journal_scan_state_t *state = (journal_scan_state_t *)userdata;
    const uint8_t *edge_sha =
        state->check_source ? edge->src_sha : edge->tgt_sha;

    /* Check if this edge matches our query */
    if (memcmp(edge_sha, state->target_sha, GM_SHA1_SIZE) == 0) {
        /* Grow array if needed */
        if (state->count >= state->capacity) {
            size_t new_capacity = state->capacity * 2;
            if (new_capacity > MAX_EDGE_IDS) {
                return GM_ERROR; /* Too many edges */
            }

            gm_edge_t *new_edges =
                realloc(state->edges, new_capacity * sizeof(gm_edge_t));
            if (!new_edges)
                return GM_NO_MEMORY;

            state->edges = new_edges;
            state->capacity = new_capacity;
        }

        /* Copy edge */
        memcpy(&state->edges[state->count], edge, sizeof(gm_edge_t));
        state->count++;
    }

    return GM_OK;
}

/* Try to query from cache */
static int try_cache_query(git_repository *repo, const char *branch,
                           const uint8_t *sha, const char *index_type,
                           gm_cache_result_t *result) {
    git_commit *cache_commit = NULL;
    git_tree *cache_tree = NULL;
    roaring_bitmap_t *bitmap = NULL;
    git_oid cache_oid;
    char ref_name[REF_NAME_BUFFER_SIZE];
    int rc;

    /* Try to load cache metadata */
    gm_cache_meta_t meta;
    rc = gm_cache_load_meta(repo, branch, &meta);
    if (rc != GM_OK) {
        return rc;
    }

    /* Get cache commit */
    snprintf(ref_name, sizeof(ref_name), "%s%s", GM_CACHE_REF_PREFIX, branch);
    rc = git_reference_name_to_id(&cache_oid, repo, ref_name);
    if (rc < 0) {
        return GM_NOT_FOUND;
    }

    rc = git_commit_lookup(&cache_commit, repo, &cache_oid);
    if (rc < 0) {
        return GM_ERROR;
    }

    /* Get cache tree */
    rc = git_commit_tree(&cache_tree, cache_commit);
    if (rc < 0) {
        git_commit_free(cache_commit);
        return GM_ERROR;
    }

    /* Load bitmap for this SHA */
    rc = load_bitmap_from_cache(repo, cache_tree, sha, index_type, &bitmap);
    git_tree_free(cache_tree);
    git_commit_free(cache_commit);

    if (rc != GM_OK) {
        return rc;
    }

    /* Extract edge IDs from bitmap */
    result->edge_ids = gm_bitmap_to_array(bitmap, &result->count);
    gm_bitmap_free(bitmap);

    if (!result->edge_ids && result->count > 0) {
        return GM_NO_MEMORY;
    }

    result->from_cache = true;
    return GM_OK;
}

/* Fallback to journal scan */
static int fallback_journal_scan(gm_context_t *ctx, const char *branch,
                                 const uint8_t *sha, int check_source,
                                 gm_cache_result_t *result) {
    journal_scan_state_t state = {
        .target_sha = sha,
        .edges = malloc(INITIAL_EDGE_CAPACITY * sizeof(gm_edge_t)),
        .capacity = INITIAL_EDGE_CAPACITY,
        .count = 0,
        .check_source = check_source};

    if (!state.edges) {
        return GM_NO_MEMORY;
    }

    int rc =
        gm_journal_read(ctx, branch, journal_scan_callback_generic, &state);
    if (rc != GM_OK) {
        free(state.edges);
        return rc;
    }

    /* Convert edges to edge IDs (just use indices for now) */
    result->edge_ids = malloc(state.count * sizeof(uint32_t));
    if (!result->edge_ids) {
        free(state.edges);
        return GM_NO_MEMORY;
    }

    for (size_t i = 0; i < state.count; i++) {
        result->edge_ids[i] = (uint32_t)i;
    }

    result->count = state.count;
    result->from_cache = false;

    free(state.edges);
    return GM_OK;
}

/* Generic cache query function */
static int cache_query_generic(git_repository *repo, const char *branch,
                               const uint8_t *sha, const char *index_type,
                               int check_source, gm_cache_result_t *result) {
    gm_context_t ctx = {0};
    int rc;

    /* Initialize result */
    memset(result, 0, sizeof(gm_cache_result_t));
    ctx.git_repo = repo;

    /* Try cache first */
    rc = try_cache_query(repo, branch, sha, index_type, result);
    if (rc == GM_OK) {
        return GM_OK;
    }

    /* Fall back to journal scan */
    return fallback_journal_scan(&ctx, branch, sha, check_source, result);
}

/* Query edges by source SHA (forward index) */
int gm_cache_query_fanout(git_repository *repo, const char *branch,
                          const uint8_t *src_sha, gm_cache_result_t *result) {
    return cache_query_generic(repo, branch, src_sha, "forward", 1, result);
}

/* Query edges by target SHA (reverse index) */
int gm_cache_query_fanin(git_repository *repo, const char *branch,
                         const uint8_t *tgt_sha, gm_cache_result_t *result) {
    return cache_query_generic(repo, branch, tgt_sha, "reverse", 0, result);
}

/* Free cache result */
void gm_cache_result_free(gm_cache_result_t *result) {
    if (result && result->edge_ids) {
        free(result->edge_ids);
        result->edge_ids = NULL;
        result->count = 0;
    }
}

/* Forward declaration */
int gm_cache_calculate_size(git_repository *repo, const git_oid *tree_oid,
                            uint64_t *size_bytes);

/* Get cache statistics */
int gm_cache_stats(git_repository *repo, const char *branch,
                   uint64_t *edge_count, uint64_t *cache_size_bytes) {
    gm_cache_meta_t meta;
    int rc = gm_cache_load_meta(repo, branch, &meta);
    if (rc != GM_OK) {
        return rc;
    }

    if (edge_count) {
        *edge_count = meta.edge_count;
    }

    if (cache_size_bytes) {
        /* Calculate actual cache size from tree */
        char ref_name[REF_NAME_BUFFER_SIZE];
        snprintf(ref_name, sizeof(ref_name), "%s%s", GM_CACHE_REF_PREFIX,
                 branch);

        git_oid cache_oid;
        rc = git_reference_name_to_id(&cache_oid, repo, ref_name);
        if (rc == 0) {
            git_commit *commit = NULL;
            rc = git_commit_lookup(&commit, repo, &cache_oid);
            if (rc == 0) {
                const git_oid *tree_oid = git_commit_tree_id(commit);
                gm_cache_calculate_size(repo, tree_oid, cache_size_bytes);
                git_commit_free(commit);
            }
        }

        /* Fall back to estimate if calculation fails */
        if (rc < 0) {
            *cache_size_bytes =
                meta.edge_count *
                CACHE_SIZE_ESTIMATE_PER_EDGE; /* Rough estimate */
        }
    }

    return GM_OK;
}