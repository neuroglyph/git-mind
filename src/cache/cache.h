/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GM_CACHE_H
#define GM_CACHE_H

#include <git2.h>

#include <stdbool.h>
#include <stdint.h>

#include "../../include/gitmind.h"

/* Cache constants */
#define GM_CACHE_MAGIC "GMCACHE\0"
#define GM_CACHE_MAGIC_LEN 8
#define GM_CACHE_VERSION 1
#define GM_CACHE_SHARD_BITS 8 /* 2 hex chars = 256 shards */
#define GM_CACHE_REF_PREFIX "refs/gitmind/cache/"

/* Cache flags */
#define GM_CACHE_FLAG_NONE 0
#define GM_CACHE_FLAG_COMPRESSED (1 << 0)
#define GM_CACHE_FLAG_INCREMENTAL (1 << 1)

/* Cache metadata stored in commit message */
typedef struct {
    uint64_t journal_tip_time; /* Timestamp of last processed journal commit */
    char journal_tip_oid[41];  /* SHA of last processed journal commit */
    uint64_t edge_count;       /* Total edges in cache */
    uint64_t build_time_ms;    /* Time to build cache */
    uint32_t shard_bits;       /* Number of bits for sharding (8 = 2 chars) */
    uint32_t version;          /* Cache format version */
    char branch[64];           /* Branch name */
} gm_cache_meta_t;

/* Cache query result */
typedef struct {
    uint32_t *edge_ids; /* Array of edge IDs */
    size_t count;       /* Number of edges */
    bool from_cache;    /* True if from cache, false if journal scan */
} gm_cache_result_t;

/* Initialize cache subsystem */
int gm_cache_init(void);

/* Rebuild cache from journal (incremental if possible) - internal version */
int gm_cache_rebuild_internal(git_repository *repo, const char *branch,
                              bool force_full);

/* Query edges by source SHA (forward index) */
int gm_cache_query_fanout(git_repository *repo, const char *branch,
                          const uint8_t *src_sha, gm_cache_result_t *result);

/* Query edges by target SHA (reverse index) */
int gm_cache_query_fanin(git_repository *repo, const char *branch,
                         const uint8_t *tgt_sha, gm_cache_result_t *result);

/* Load cache metadata */
int gm_cache_load_meta(git_repository *repo, const char *branch,
                       gm_cache_meta_t *meta);

/* Check if cache needs rebuild */
bool gm_cache_is_stale(git_repository *repo, const char *branch);

/* Free cache result */
void gm_cache_result_free(gm_cache_result_t *result);

/* Get cache statistics */
int gm_cache_stats(git_repository *repo, const char *branch,
                   uint64_t *edge_count, uint64_t *cache_size_bytes);

#endif /* GM_CACHE_H */