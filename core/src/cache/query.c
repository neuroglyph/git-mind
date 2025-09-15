/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <roaring/roaring.h>

#include <git2/repository.h>
#include <git2/refs.h>
#include <git2/commit.h>
#include <git2/oid.h>
#include <git2/tree.h>
#include <git2/blob.h>

#include "gitmind/cache.h"
#include "gitmind/cache/bitmap.h"
#include "gitmind/constants.h"
#include "gitmind/context.h"
#include "gitmind/error.h"
#include "gitmind/journal.h"
#include "gitmind/security/memory.h"
#include "gitmind/security/string.h"

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
        size_t off = (size_t)i * (size_t)HEX_CHARS_PER_BYTE;
        (void)snprintf(prefix + off, (size_t)HEX_CHARS_PER_BYTE + 1, "%02x",
                       sha[i]);
    }
    prefix[(size_t)chars * (size_t)HEX_CHARS_PER_BYTE] = '\0';
}

/* Load cache metadata from commit message */
/* Helper to find legacy timestamped cache refs: refs/gitmind/cache/<branch>/<ts> */
static int find_legacy_cache_ref(git_repository *repo, const char *branch, git_oid *out_oid, uint64_t *out_time) {
    char pattern[REF_NAME_BUFFER_SIZE + 8];
    (void)snprintf(pattern, sizeof(pattern), "%s%s/*", GM_CACHE_REF_PREFIX, branch);
    git_reference *best_ref = NULL;
    git_reference *ref = NULL;
    git_reference_iterator *iter = NULL;
    int rc = git_reference_iterator_glob_new(&iter, repo, pattern);
    if (rc < 0) return GM_NOT_FOUND;

    uint64_t best_time = 0;
    while (git_reference_next(&ref, iter) == 0) {
        const char *name = git_reference_name(ref);
        git_oid oid;
        if (git_reference_name_to_id(&oid, repo, name) == 0) {
            git_commit *c = NULL;
            if (git_commit_lookup(&c, repo, &oid) == 0) {
                git_time_t t = git_commit_time(c);
                if ((uint64_t)t > best_time) {
                    best_time = (uint64_t)t;
                    if (best_ref) git_reference_free(best_ref);
                    best_ref = ref; /* take ownership */
                    ref = NULL;     /* prevent double free */
                    *out_oid = oid;
                }
                git_commit_free(c);
            }
        }
        if (ref) git_reference_free(ref);
        ref = NULL;
    }
    git_reference_iterator_free(iter);
    if (ref) git_reference_free(ref);
    if (!best_ref) return GM_NOT_FOUND;
    if (out_time) *out_time = best_time;
    git_reference_free(best_ref);
    return GM_OK;
}

int gm_cache_load_meta(gm_context_t *ctx, const char *branch, gm_cache_meta_t *meta) {
    if (!ctx || !ctx->git_repo || !branch || !meta) return GM_INVALID_ARG;
    git_repository *repo = (git_repository *)ctx->git_repo;
    git_reference *ref = NULL;
    git_commit *commit = NULL;
    char ref_name[REF_NAME_BUFFER_SIZE];
    int rc;

    /* Build cache ref name and resolve */
    {
        int rn = gm_snprintf(ref_name, sizeof(ref_name), "%s%s", GM_CACHE_REF_PREFIX, branch);
        if (rn < 0 || (size_t)rn >= sizeof(ref_name)) {
            return GM_ERR_BUFFER_TOO_SMALL;
        }
    }
    rc = git_reference_lookup(&ref, repo, ref_name);
    git_oid oid;
    if (rc == 0) {
        rc = git_reference_name_to_id(&oid, repo, ref_name);
        git_reference_free(ref);
        if (rc < 0) return GM_NOT_FOUND;
    } else {
        /* Fallback: look for legacy timestamped refs */
        uint64_t ts = 0;
        rc = find_legacy_cache_ref(repo, branch, &oid, &ts);
        if (rc != GM_OK) return GM_NOT_FOUND;
    }

    rc = git_commit_lookup(&commit, repo, &oid);
    if (rc < 0) {
        return GM_ERR_UNKNOWN;
    }

    /* Synthesize metadata from repo state (no binary commit message parsing) */
    memset(meta, 0, sizeof *meta);
    meta->version = GM_CACHE_VERSION;
    meta->shard_bits = GM_CACHE_SHARD_BITS;
    strncpy(meta->branch, branch, GM_CACHE_BRANCH_NAME_SIZE - 1);
    meta->journal_tip_time = (uint64_t)git_commit_time(commit);
    git_commit_free(commit);

    /* Resolve current journal tip OID for branch */
    git_reference *journal_ref = NULL;
    char journal_ref_name[REF_NAME_BUFFER_SIZE];
    (void)snprintf(journal_ref_name, sizeof(journal_ref_name),
                   "refs/gitmind/edges/%s", branch);
    if (git_reference_lookup(&journal_ref, repo, journal_ref_name) == 0) {
        const git_oid *tip_oid = git_reference_target(journal_ref);
        if (tip_oid) {
            git_oid_tostr(meta->journal_tip_oid, sizeof(meta->journal_tip_oid), tip_oid);
        } else {
            strcpy(meta->journal_tip_oid, ZERO_SHA_STRING);
        }
        git_reference_free(journal_ref);
    } else {
        strcpy(meta->journal_tip_oid, ZERO_SHA_STRING);
    }

    /* edge_count and build_time_ms unavailable without dedicated storage */
    meta->edge_count = 0;
    meta->build_time_ms = 0;
    return GM_OK;
}

/* Check if cache is stale */
bool gm_cache_is_stale(gm_context_t *ctx, const char *branch) {
    gm_cache_meta_t meta;
    git_repository *repo = (git_repository *)ctx->git_repo;

    /* Try to load cache metadata */
    if (gm_cache_load_meta(ctx, branch, &meta) != GM_OK) {
        return true; /* No cache = stale */
    }

    /* Check age */
    time_t now = time(NULL);
    if ((uint64_t)now - meta.journal_tip_time > CACHE_MAX_AGE_SECONDS) {
        return true;
    }

    /* Check if journal has new commits since cache was built */
    git_reference *journal_ref = NULL;
    char journal_ref_name[REF_NAME_BUFFER_SIZE];
    {
        int rn = gm_snprintf(journal_ref_name, sizeof(journal_ref_name),
                              "refs/gitmind/edges/%s", branch);
        if (rn < 0 || (size_t)rn >= sizeof(journal_ref_name)) {
            return true; /* treat as stale if we can't build ref safely */
        }
    }

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
    for (int i = 0; i < GM_OID_RAWSZ; i++) {
        size_t off = (size_t)i * (size_t)HEX_CHARS_PER_BYTE;
        (void)snprintf(sha_hex + off, (size_t)HEX_CHARS_PER_BYTE + 1, "%02x",
                       sha[i]);
    }
    sha_hex[(size_t)GM_OID_RAWSZ * (size_t)HEX_CHARS_PER_BYTE] = '\0';

    /* Build path: prefix/sha.suffix */
    {
        int rn = gm_snprintf(path, sizeof(path), "%s/%s.%s", prefix, sha_hex, suffix);
        if (rn < 0 || (size_t)rn >= sizeof(path)) {
            return GM_ERR_BUFFER_TOO_SMALL;
        }
    }

    /* Look up tree entry */
    rc = git_tree_entry_bypath(&entry, tree, path);
    if (rc < 0)
        return GM_NOT_FOUND;

    /* Get blob */
    rc = git_blob_lookup(&blob, repo, git_tree_entry_id(entry));
    git_tree_entry_free(entry);
    if (rc < 0)
        return GM_ERR_UNKNOWN;

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
    const uint8_t *edge_sha = state->check_source
        ? git_oid_raw(&edge->src_oid)
        : git_oid_raw(&edge->tgt_oid);

    /* Check if this edge matches our query */
    if (memcmp(edge_sha, state->target_sha, GM_OID_RAWSZ) == 0) {
        /* Grow array if needed */
        if (state->count >= state->capacity) {
            size_t new_capacity = state->capacity * 2;
            if (new_capacity > MAX_EDGE_IDS) {
                return GM_ERR_UNKNOWN; /* Too many edges */
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
    gm_context_t ctx = {0};
    ctx.git_repo = repo;
    rc = gm_cache_load_meta(&ctx, branch, &meta);
    if (rc != GM_OK) {
        return rc;
    }

    /* Get cache commit */
    {
        int rn = gm_snprintf(ref_name, sizeof(ref_name), "%s%s", GM_CACHE_REF_PREFIX, branch);
        if (rn < 0 || (size_t)rn >= sizeof(ref_name)) {
            return GM_ERR_BUFFER_TOO_SMALL;
        }
    }
    rc = git_reference_name_to_id(&cache_oid, repo, ref_name);
    if (rc < 0) {
        return GM_NOT_FOUND;
    }

    rc = git_commit_lookup(&cache_commit, repo, &cache_oid);
    if (rc < 0) {
        return GM_ERR_UNKNOWN;
    }

    /* Get cache tree */
    rc = git_commit_tree(&cache_tree, cache_commit);
    if (rc < 0) {
        git_commit_free(cache_commit);
        return GM_ERR_UNKNOWN;
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
int gm_cache_query_fanout(gm_context_t *ctx, const char *branch,
                          const gm_oid_t *src_oid, gm_cache_result_t *result) {
    git_repository *repo = (git_repository *)ctx->git_repo;
    return cache_query_generic(repo, branch, git_oid_raw(src_oid), "forward", 1, result);
}

/* Query edges by target SHA (reverse index) */
int gm_cache_query_fanin(gm_context_t *ctx, const char *branch,
                         const gm_oid_t *tgt_oid, gm_cache_result_t *result) {
    git_repository *repo = (git_repository *)ctx->git_repo;
    return cache_query_generic(repo, branch, git_oid_raw(tgt_oid), "reverse", 0, result);
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
int gm_cache_stats(gm_context_t *ctx, const char *branch,
                   uint64_t *edge_count, uint64_t *cache_size_bytes) {
    gm_cache_meta_t meta;
    git_repository *repo = (git_repository *)ctx->git_repo;
    int rc = gm_cache_load_meta(ctx, branch, &meta);
    if (rc != GM_OK) {
        return rc;
    }

    if (edge_count) {
        *edge_count = meta.edge_count;
    }

    if (cache_size_bytes) {
        /* Calculate actual cache size from tree */
        char ref_name[REF_NAME_BUFFER_SIZE];
        {
            int rn = gm_snprintf(ref_name, sizeof(ref_name), "%s%s", GM_CACHE_REF_PREFIX, branch);
            if (rn < 0 || (size_t)rn >= sizeof(ref_name)) {
                /* fall back to estimate below */
                rc = -1;
            }
        }

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
