/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_CACHE_H
#define GITMIND_CACHE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <git2/types.h>
#include <git2/oid.h>
#include "gitmind/types.h"
#include "gitmind/result.h"

#include "gitmind/context.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file cache.h
 * @brief High-performance query cache for git-mind edge data
 *
 * The cache system provides fast lookups for edge relationships by maintaining
 * indexed Git objects. It supports incremental updates and sharded storage.
 */

/* Cache configuration constants */
#define GM_CACHE_VERSION 1
#define GM_CACHE_SHARD_BITS 8 /* 2 hex chars = 256 shards */
#define GM_CACHE_REF_PREFIX "refs/gitmind/cache/"
#define GM_CACHE_BRANCH_NAME_SIZE 64  /* Maximum branch name length */
#define GM_CACHE_OID_STRING_SIZE (GM_OID_HEX_CHARS + 1) /* OID hex + NUL */

/* Cache result containing edge IDs matching a query */
typedef struct {
    uint32_t *edge_ids; /**< Array of matching edge IDs */
    size_t count;       /**< Number of edges in result */
    bool from_cache;    /**< True if served from cache, false if live query */
} gm_cache_result_t;

/* Cache metadata and statistics */
typedef struct {
    uint64_t journal_tip_time; /**< Timestamp of last processed journal commit */
    char journal_tip_oid[GM_CACHE_OID_STRING_SIZE];  /**< @deprecated OID hex string of last processed journal commit */
    uint64_t edge_count;       /**< Total edges in cache */
    uint64_t build_time_ms;    /**< Time to build cache (milliseconds) */
    uint32_t shard_bits;       /**< Number of bits for sharding */
    uint32_t version;          /**< Cache format version */
    char branch[GM_CACHE_BRANCH_NAME_SIZE];           /**< Branch name (may be truncated if too long) */
    gm_oid_t journal_tip_oid_bin; /**< OID of last processed journal commit (preferred). Added at struct tail to preserve ABI layout. */
} gm_cache_meta_t;

/**
 * Initialize the cache subsystem
 * @return GM_OK on success, error code on failure
 */
GM_NODISCARD int gm_cache_init(void);

/**
 * Rebuild cache from journal data
 * @param ctx Git-mind context containing repository
 * @param force_full Force full rebuild instead of incremental
 * @return GM_OK on success, error code on failure
 */
GM_NODISCARD int gm_cache_rebuild(gm_context_t *ctx, const char *branch, bool force_full);

/**
 * Query edges by source OID (forward traversal)
 * @param ctx Git-mind context containing repository
 * @param src_oid Source object ID
 * @param result Output result structure
 * @return GM_OK on success, error code on failure
 */
GM_NODISCARD int gm_cache_query_fanout(gm_context_t *ctx, const char *branch, const gm_oid_t *src_oid,
                          gm_cache_result_t *result);

/**
 * Query edges by target OID (reverse traversal)
 * @param ctx Git-mind context containing repository
 * @param branch Branch name to query
 * @param tgt_oid Target object ID
 * @param result Output result structure
 * @return GM_OK on success, error code on failure
 */
GM_NODISCARD int gm_cache_query_fanin(gm_context_t *ctx, const char *branch, const gm_oid_t *tgt_oid,
                         gm_cache_result_t *result);

/**
 * Load cache metadata for specified branch
 * @param ctx Git-mind context containing repository
 * @param branch Branch name to load metadata for
 * @param meta Output metadata structure
 * @return GM_OK on success, error code on failure
 */
GM_NODISCARD int gm_cache_load_meta(gm_context_t *ctx, const char *branch, gm_cache_meta_t *meta);

/**
 * Check if cache needs rebuilding
 * @param ctx Git-mind context containing repository
 * @param branch Branch name to check cache for
 * @return true if cache is stale and needs rebuild
 */
GM_NODISCARD bool gm_cache_is_stale(gm_context_t *ctx, const char *branch);

/**
 * Get cache statistics
 * @param ctx Git-mind context containing repository
 * @param edge_count Output total number of edges
 * @param cache_size_bytes Output cache size in bytes
 * @return GM_OK on success, error code on failure
 */
GM_NODISCARD int gm_cache_stats(gm_context_t *ctx, const char *branch, uint64_t *edge_count, 
                   uint64_t *cache_size_bytes);

/**
 * Free cache result memory
 * @param result Result structure to free
 */
void gm_cache_result_free(gm_cache_result_t *result);

/* Internal functions have been moved to a private header to avoid exposing
 * non-public API via umbrella headers. See core/src/cache/cache_internal.h. */

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_CACHE_H */
