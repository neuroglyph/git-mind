/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <roaring/roaring.h>

#include "gitmind/cache.h"
#include "gitmind/cache/bitmap.h"
#include "gitmind/constants.h"
#include "gitmind/context.h"
#include "gitmind/error.h"
#include "gitmind/journal.h"
#include "gitmind/security/memory.h"
#include "gitmind/security/string.h"
#include "gitmind/util/ref.h"
#include "gitmind/util/memory.h"
#include "gitmind/constants_internal.h"

/* Constants */
#define CACHE_MAX_AGE_SECONDS 3600 /* 1 hour */
#define MAX_EDGE_IDS 100000        /* Safety limit */
#define SHA_PREFIX_BUFFER_SIZE 16  /* Buffer for SHA prefix */
#define CACHE_PATH_BUFFER_SIZE 128 /* Buffer for cache paths */
#define INITIAL_EDGE_CAPACITY 100  /* Initial allocation for edge arrays */

static int gm_hex_to_raw(const char *hex, uint8_t *out_raw, size_t raw_size);

static int gm_hex_value(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    c = (char)(c | 0x20); /* lowercase */
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    return -1;
}

static void gm_raw_to_hex(const uint8_t *raw,
                          char out_hex[GM_OID_HEX_CHARS + 1]) {
    static const char kHexDigits[] = "0123456789abcdef";
    for (size_t i = 0; i < GM_OID_RAWSZ; ++i) {
        unsigned byte = raw[i];
        out_hex[i * 2] = kHexDigits[(byte >> 4) & 0xF];
        out_hex[i * 2 + 1] = kHexDigits[byte & 0xF];
    }
    out_hex[GM_OID_HEX_CHARS] = '\0';
}

static int gm_hex_to_raw(const char *hex, uint8_t *out_raw, size_t raw_size) {
    if (hex == NULL || out_raw == NULL) {
        return GM_ERR_INVALID_ARGUMENT;
    }
    size_t len = strlen(hex);
    if (len != raw_size * 2) {
        return GM_ERR_INVALID_ARGUMENT;
    }
    for (size_t i = 0; i < raw_size; ++i) {
        int hi = gm_hex_value(hex[i * 2]);
        int lo = gm_hex_value(hex[i * 2 + 1]);
        if (hi < 0 || lo < 0) {
            return GM_ERR_INVALID_ARGUMENT;
        }
        out_raw[i] = (uint8_t)((hi << 4) | lo);
    }
    return GM_OK;
}

static int gm_hex_to_oid(const char *hex, gm_oid_t *oid) {
    if (oid == NULL) {
        return GM_ERR_INVALID_ARGUMENT;
    }
    return gm_hex_to_raw(hex, oid->id, GM_OID_RAWSZ);
}

static bool gm_oid_equal(const gm_oid_t *a, const gm_oid_t *b) {
    return a != NULL && b != NULL && memcmp(a->id, b->id, GM_OID_RAWSZ) == 0;
}

/* Get SHA prefix for sharding */
static void get_sha_prefix(const uint8_t *sha, char *prefix, int bits) {
    int chars = (bits + 3) / BITS_PER_HEX_CHAR; /* Round up to hex chars */
    if (chars <= 0) { prefix[0] = '\0'; return; }
    git_oid tmp;
    git_oid_fromraw(&tmp, sha);
    char hex[GM_OID_HEX_CHARS];
    git_oid_fmt(hex, &tmp); /* not null-terminated */
    if (chars > GM_OID_HEX_CHARS) chars = GM_OID_HEX_CHARS;
    /* Copy requested prefix and terminate */
    for (int i = 0; i < chars; i++) prefix[i] = hex[i];
    prefix[chars] = '\0';
}

static int unwrap_result_code(gm_result_void_t result) {
    if (result.ok) {
        return GM_OK;
    }
    int code = GM_ERR_UNKNOWN;
    if (result.u.err != NULL) {
        code = result.u.err->code;
        gm_error_free(result.u.err);
    }
    return code;
}

int gm_cache_load_meta(gm_context_t *ctx, const char *branch, gm_cache_meta_t *meta) {
    if (ctx == NULL || branch == NULL || meta == NULL) {
        return GM_ERR_INVALID_ARGUMENT;
    }

    gm_memset_safe(meta, sizeof(*meta), 0, sizeof(*meta));

    if (ctx->git_repo_port.vtbl == NULL) {
        return GM_ERR_INVALID_STATE;
    }

    char ref_name[REF_NAME_BUFFER_SIZE];
    {
        int brc = gm_build_ref(ref_name, sizeof(ref_name), GM_CACHE_REF_PREFIX, branch);
        if (brc != GM_OK) {
            return brc;
        }
    }

    gm_git_reference_tip_t cache_tip = {0};
    int rc = unwrap_result_code(gm_git_repository_port_reference_tip(
        &ctx->git_repo_port, ref_name, &cache_tip));
    if (rc != GM_OK) {
        return rc;
    }

    if (!cache_tip.has_target) {
        char pattern[REF_NAME_BUFFER_SIZE + 8];
        int rn = gm_snprintf(pattern, sizeof(pattern), "%s%s/*", GM_CACHE_REF_PREFIX,
                              branch);
        if (rn < 0 || (size_t)rn >= sizeof(pattern)) {
            return GM_ERR_NOT_FOUND;
        }

        rc = unwrap_result_code(gm_git_repository_port_reference_glob_latest(
            &ctx->git_repo_port, pattern, &cache_tip));
        if (rc != GM_OK) {
            return rc;
        }
        if (!cache_tip.has_target) {
            return GM_ERR_NOT_FOUND;
        }
    }

    meta->version = GM_CACHE_VERSION;
    meta->shard_bits = GM_CACHE_SHARD_BITS;
    if (gm_strcpy_safe(meta->branch, GM_CACHE_BRANCH_NAME_SIZE, branch) != GM_OK) {
        return GM_ERR_BUFFER_TOO_SMALL;
    }
    meta->journal_tip_time = cache_tip.commit_time;
    meta->cache_tip_oid = cache_tip.oid;

    char journal_ref_name[REF_NAME_BUFFER_SIZE];
    {
        int brc = gm_build_ref(journal_ref_name, sizeof(journal_ref_name),
                               GITMIND_EDGES_REF_PREFIX, branch);
        if (brc != GM_OK) {
            gm_memset_safe(&meta->journal_tip_oid_bin,
                           sizeof(meta->journal_tip_oid_bin), 0,
                           sizeof(meta->journal_tip_oid_bin));
            meta->journal_tip_oid[0] = '\0';
            meta->edge_count = 0;
            meta->build_time_ms = 0;
            return GM_OK;
        }
    }

    gm_git_reference_tip_t journal_tip = {0};
    rc = unwrap_result_code(gm_git_repository_port_reference_tip(
        &ctx->git_repo_port, journal_ref_name, &journal_tip));
    if (rc != GM_OK || !journal_tip.has_target) {
        gm_memset_safe(&meta->journal_tip_oid_bin, sizeof(meta->journal_tip_oid_bin), 0,
                       sizeof(meta->journal_tip_oid_bin));
        meta->journal_tip_oid[0] = '\0';
    } else {
        meta->journal_tip_oid_bin = journal_tip.oid;
        if (gm_strcpy_safe(meta->journal_tip_oid, sizeof(meta->journal_tip_oid),
                           journal_tip.oid_hex) != GM_OK) {
            meta->journal_tip_oid[0] = '\0';
            return GM_ERR_BUFFER_TOO_SMALL;
        }
    }

    meta->edge_count = 0;
    meta->build_time_ms = 0;
    return GM_OK;
}

/* Check if cache is stale */
bool gm_cache_is_stale(gm_context_t *ctx, const char *branch) {
    if (ctx == NULL || branch == NULL || ctx->git_repo_port.vtbl == NULL) {
        return true;
    }

    gm_cache_meta_t meta;
    if (gm_cache_load_meta(ctx, branch, &meta) != GM_OK) {
        return true;
    }

    /* Check age */
    time_t now = time(NULL);
    if ((uint64_t)now - meta.journal_tip_time > CACHE_MAX_AGE_SECONDS) {
        return true;
    }

    /* Check if journal has new commits since cache was built */
    char journal_ref_name[REF_NAME_BUFFER_SIZE];
    {
        int brc = gm_build_ref(journal_ref_name, sizeof(journal_ref_name),
                               GITMIND_EDGES_REF_PREFIX, branch);
        if (brc != GM_OK) {
            return true; /* treat as stale if we can't build ref safely */
        }
    }
    gm_git_reference_tip_t current_tip = {0};
    int rc = unwrap_result_code(gm_git_repository_port_reference_tip(
        &ctx->git_repo_port, journal_ref_name, &current_tip));
    if (rc != GM_OK || !current_tip.has_target) {
        return true;
    }

    bool have_binary = (memcmp(&meta.journal_tip_oid_bin, &(const gm_oid_t){0},
                               sizeof(gm_oid_t)) != 0);
    if (have_binary) {
        return !gm_oid_equal(&current_tip.oid, &meta.journal_tip_oid_bin);
    }

    if (meta.journal_tip_oid[0] != '\0') {
        gm_oid_t cached_tip;
        if (gm_hex_to_oid(meta.journal_tip_oid, &cached_tip) != GM_OK) {
            return true;
        }
        return !gm_oid_equal(&current_tip.oid, &cached_tip);
    }

    return true; /* No previous tip known */
}

/* Load bitmap from cache tree */
static int load_bitmap_from_cache(const gm_git_repository_port_t *port,
                                  const gm_oid_t *cache_commit_oid,
                                  const uint8_t *sha, const char *suffix,
                                  roaring_bitmap_t **bitmap) {
    char prefix[SHA_PREFIX_BUFFER_SIZE];
    char sha_hex[SHA_HEX_SIZE];
    char path[CACHE_PATH_BUFFER_SIZE];
    int rc;

    /* Get shard prefix */
    get_sha_prefix(sha, prefix, GM_CACHE_SHARD_BITS);

    /* Convert SHA to hex */
    gm_raw_to_hex(sha, sha_hex);

    /* Build path: prefix/sha.suffix */
    {
        int rn = gm_snprintf(path, sizeof(path), "%s/%s.%s", prefix, sha_hex, suffix);
        if (rn < 0 || (size_t)rn >= sizeof(path)) {
            return GM_ERR_BUFFER_TOO_SMALL;
        }
    }

    /* Look up tree entry */
    uint8_t *blob_data = NULL;
    size_t blob_size = 0;
    rc = unwrap_result_code(gm_git_repository_port_commit_read_blob(
        port, cache_commit_oid, path, &blob_data, &blob_size));
    if (rc != GM_OK) {
        return rc;
    }

    rc = gm_bitmap_deserialize(blob_data, blob_size, bitmap);
    free(blob_data);

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
        ? (const uint8_t *)edge->src_oid.id
        : (const uint8_t *)edge->tgt_oid.id;

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
                return GM_ERR_OUT_OF_MEMORY;

            state->edges = new_edges;
            state->capacity = new_capacity;
        }

        /* Copy edge */
        (void)gm_memcpy_span(&state->edges[state->count], sizeof(gm_edge_t), edge, sizeof(gm_edge_t));
        state->count++;
    }

    return GM_OK;
}

/* Try to query from cache */
static int try_cache_query(gm_context_t *ctx, const char *branch,
                           const uint8_t *sha, const char *index_type,
                           gm_cache_result_t *result) {
    roaring_bitmap_t *bitmap = NULL;
    int rc;

    /* Try to load cache metadata */
    gm_cache_meta_t meta;
    rc = gm_cache_load_meta(ctx, branch, &meta);
    if (rc != GM_OK) {
        return rc;
    }

    if (memcmp(&meta.cache_tip_oid, &(const gm_oid_t){0}, sizeof(gm_oid_t)) == 0) {
        return GM_ERR_NOT_FOUND;
    }

    /* Load bitmap for this SHA */
    rc = load_bitmap_from_cache(&ctx->git_repo_port, &meta.cache_tip_oid, sha,
                                index_type, &bitmap);

    if (rc != GM_OK) {
        return rc;
    }

    /* Extract edge IDs from bitmap */
    result->edge_ids = gm_bitmap_to_array(bitmap, &result->count);
    gm_bitmap_free(bitmap);

    if (!result->edge_ids && result->count > 0) {
        return GM_ERR_OUT_OF_MEMORY;
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
        return GM_ERR_OUT_OF_MEMORY;
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
        return GM_ERR_OUT_OF_MEMORY;
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
static int cache_query_generic(gm_context_t *ctx, const char *branch,
                               const uint8_t *sha, const char *index_type,
                               int check_source, gm_cache_result_t *result) {
    int rc;

    /* Initialize result */
    gm_memset_safe(result, sizeof(gm_cache_result_t), 0, sizeof(gm_cache_result_t));

    /* Try cache first */
    rc = try_cache_query(ctx, branch, sha, index_type, result);
    if (rc == GM_OK) {
        return GM_OK;
    }

    /* Fall back to journal scan */
    return fallback_journal_scan(ctx, branch, sha, check_source, result);
}

/* Query edges by source SHA (forward index) */
int gm_cache_query_fanout(gm_context_t *ctx, const char *branch,
                          const gm_oid_t *src_oid, gm_cache_result_t *result) {
    return cache_query_generic(ctx, branch, (const uint8_t *)src_oid->id,
                               "forward", 1, result);
}

/* Query edges by target SHA (reverse index) */
int gm_cache_query_fanin(gm_context_t *ctx, const char *branch,
                         const gm_oid_t *tgt_oid, gm_cache_result_t *result) {
    return cache_query_generic(ctx, branch, (const uint8_t *)tgt_oid->id,
                               "reverse", 0, result);
}

/* Free cache result */
void gm_cache_result_free(gm_cache_result_t *result) {
    if (result && result->edge_ids) {
        free(result->edge_ids);
        result->edge_ids = NULL;
        result->count = 0;
    }
}

/* Get cache statistics */
int gm_cache_stats(gm_context_t *ctx, const char *branch,
                   uint64_t *edge_count, uint64_t *cache_size_bytes) {
    if (ctx == NULL || branch == NULL) {
        return GM_ERR_INVALID_ARGUMENT;
    }
    if (ctx->git_repo_port.vtbl == NULL) {
        return GM_ERR_INVALID_STATE;
    }

    gm_cache_meta_t meta;
    int rc = gm_cache_load_meta(ctx, branch, &meta);
    if (rc != GM_OK) {
        return rc;
    }

    if (edge_count) {
        *edge_count = meta.edge_count;
    }

    if (cache_size_bytes) {
        if (memcmp(&meta.cache_tip_oid, &(const gm_oid_t){0}, sizeof(gm_oid_t)) != 0) {
            rc = unwrap_result_code(gm_git_repository_port_commit_tree_size(
                &ctx->git_repo_port, &meta.cache_tip_oid, cache_size_bytes));
            if (rc != GM_OK) {
                *cache_size_bytes =
                    meta.edge_count * CACHE_SIZE_ESTIMATE_PER_EDGE;
            }
        } else {
            *cache_size_bytes = meta.edge_count * CACHE_SIZE_ESTIMATE_PER_EDGE;
        }
    }

    return GM_OK;
}
