/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/ports/cache_query_port.h"
#include "gitmind/cache.h"
#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/context.h"
#include <stdint.h>
#include <stdlib.h>

typedef struct {
    gm_context_t *ctx; /* not owned */
} cache_query_state_t;

static gm_result_void_t query_fanout_impl(gm_qry_cache_port_t *self,
                                          const char *branch,
                                          const gm_oid_t *src_oid,
                                          gm_cache_result_t *out) {
    if (self == NULL || self->state == NULL || branch == NULL ||
        src_oid == NULL || out == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "cache query fanout requires inputs"));
    }
    cache_query_state_t *state = (cache_query_state_t *)self->state;
    int result_code = gm_cache_query_fanout(state->ctx, branch, src_oid, out);
    if (result_code != GM_OK) {
        return gm_err_void(GM_ERROR(result_code, "fanout query failed"));
    }
    return gm_ok_void();
}

static gm_result_void_t query_fanin_impl(gm_qry_cache_port_t *self,
                                         const char *branch,
                                         const gm_oid_t *tgt_oid,
                                         gm_cache_result_t *out) {
    if (self == NULL || self->state == NULL || branch == NULL ||
        tgt_oid == NULL || out == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "cache query fanin requires inputs"));
    }
    cache_query_state_t *state = (cache_query_state_t *)self->state;
    int result_code = gm_cache_query_fanin(state->ctx, branch, tgt_oid, out);
    if (result_code != GM_OK) {
        return gm_err_void(GM_ERROR(result_code, "fanin query failed"));
    }
    return gm_ok_void();
}

static gm_result_void_t stats_impl(gm_qry_cache_port_t *self,
                                   const char *branch,
                                   uint64_t *edge_count,
                                   uint64_t *cache_size_bytes) {
    if (self == NULL || self->state == NULL || branch == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "cache stats requires branch"));
    }
    cache_query_state_t *state = (cache_query_state_t *)self->state;
    int result_code = gm_cache_stats(state->ctx, branch, edge_count, cache_size_bytes);
    if (result_code != GM_OK) {
        return gm_err_void(GM_ERROR(result_code, "cache stats failed"));
    }
    return gm_ok_void();
}

static const gm_qry_cache_port_vtbl_t VTBL = {
    .query_fanout = query_fanout_impl,
    .query_fanin  = query_fanin_impl,
    .stats        = stats_impl,
};

GM_NODISCARD gm_result_void_t gm_qry_cache_port_init(
    gm_qry_cache_port_t *port, gm_context_t *ctx) {
    if (port == NULL || ctx == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "cache query port requires ctx"));
    }
    cache_query_state_t *state = (cache_query_state_t *)calloc(1, sizeof(*state));
    if (state == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_OUT_OF_MEMORY,
                                    "allocating cache query state"));
    }
    state->ctx = ctx;
    port->vtbl = &VTBL;
    port->state = state;
    return gm_ok_void();
}

void gm_qry_cache_port_dispose(gm_qry_cache_port_t *port) {
    if (port == NULL) {
        return;
    }
    free(port->state);
    port->state = NULL;
    port->vtbl = NULL;
}
