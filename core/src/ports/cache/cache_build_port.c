/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/ports/cache_build_port.h"
#include "gitmind/cache.h"
#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/context.h"
#include <stdlib.h>

typedef struct {
    gm_context_t *ctx; /* not owned */
} cache_build_state_t;

static gm_result_void_t request_build_impl(gm_cmd_cache_build_port_t *self,
                                           const char *branch, bool force_full) {
    if (self == NULL || self->state == NULL || branch == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "cache build requires branch"));
    }
    cache_build_state_t *state = (cache_build_state_t *)self->state;
    int result_code = gm_cache_rebuild(state->ctx, branch, force_full);
    if (result_code != GM_OK) {
        return gm_err_void(GM_ERROR(result_code, "cache rebuild failed"));
    }
    return gm_ok_void();
}

static gm_result_void_t invalidate_impl(gm_cmd_cache_build_port_t *self,
                                        const char *branch) {
    (void)self;
    (void)branch;
    /* No dedicated invalidation yet; treat as best-effort no-op */
    return gm_ok_void();
}

static const gm_cmd_cache_build_port_vtbl_t VTBL = {
    .request_build = request_build_impl,
    .invalidate = invalidate_impl,
};

GM_NODISCARD gm_result_void_t gm_cmd_cache_build_port_init(
    gm_cmd_cache_build_port_t *port, gm_context_t *ctx) {
    if (port == NULL || ctx == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "cache build port requires ctx"));
    }
    cache_build_state_t *st = (cache_build_state_t *)calloc(1, sizeof(*st));
    if (st == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_OUT_OF_MEMORY,
                                    "allocating cache build state"));
    }
    st->ctx = ctx;
    port->vtbl = &VTBL;
    port->state = st;
    return gm_ok_void();
}

void gm_cmd_cache_build_port_dispose(gm_cmd_cache_build_port_t *port) {
    if (port == NULL) return;
    free(port->state);
    port->state = NULL;
    port->vtbl = NULL;
}
