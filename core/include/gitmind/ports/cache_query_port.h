/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_PORTS_CACHE_QUERY_PORT_H
#define GITMIND_PORTS_CACHE_QUERY_PORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "gitmind/cache.h"    /* gm_cache_result_t */
#include "gitmind/result.h"   /* gm_result_void_t */
#include "gitmind/util/oid.h" /* gm_oid_t */
#include "gitmind/context.h"  /* gm_context_t */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file cache_query_port.h
 * @brief Inbound driving port for cache queries and stats.
 *
 * Memory ownership:
 * - query_* functions populate gm_cache_result_t with a dynamically allocated
 *   edge_ids array when count > 0. Callers must free via gm_cache_result_free().
 *
 * Error handling:
 * - Returns gm_result_void_t wrapping error codes from underlying services.
 */

typedef struct gm_qry_cache_port_vtbl gm_qry_cache_port_vtbl_t;

typedef struct gm_qry_cache_port {
    const gm_qry_cache_port_vtbl_t *vtbl; /* non-owning */
    void *state;                           /* opaque implementation state */
} gm_qry_cache_port_t;

struct gm_qry_cache_port_vtbl {
    /**
     * Query edges by source OID (fanout).
     * @param self    Port instance.
     * @param branch  Branch name.
     * @param src_oid Source object id.
     * @param out     Result (caller frees edge_ids via gm_cache_result_free()).
     */
    gm_result_void_t (*query_fanout)(gm_qry_cache_port_t *self,
                                     const char *branch,
                                     const gm_oid_t *src_oid,
                                     gm_cache_result_t *out);

    /**
     * Query edges by target OID (fanin).
     * Same memory ownership rules as query_fanout.
     */
    gm_result_void_t (*query_fanin)(gm_qry_cache_port_t *self,
                                    const char *branch,
                                    const gm_oid_t *tgt_oid,
                                    gm_cache_result_t *out);

    /** Retrieve cache statistics for a branch. */
    gm_result_void_t (*stats)(gm_qry_cache_port_t *self,
                              const char *branch,
                              uint64_t *edge_count,
                              uint64_t *cache_size_bytes);
};

/* Default implementation factory (thin coordinator over existing APIs). */
/** Initialize a cache query port using dependencies from the provided context. */
GM_NODISCARD gm_result_void_t gm_qry_cache_port_init(
    gm_qry_cache_port_t *port, gm_context_t *ctx);
/** Release any resources held by a cache query port. */
void gm_qry_cache_port_dispose(gm_qry_cache_port_t *port);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_PORTS_CACHE_QUERY_PORT_H */
