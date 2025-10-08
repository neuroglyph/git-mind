/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_PORTS_CACHE_BUILD_PORT_H
#define GITMIND_PORTS_CACHE_BUILD_PORT_H

#include <stdbool.h>
#include <stddef.h>

#include "gitmind/result.h"
#include "gitmind/context.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file cache_build_port.h
 * @brief Inbound driving port for cache rebuild and invalidation requests.
 *
 * This port represents the application-facing surface for triggering cache
 * operations. Implementations should orchestrate domain services and use
 * outbound ports for all IO. Methods return gm_result_* variants and MUST NOT
 * perform direct IO in inline code.
 */

typedef struct gm_cmd_cache_build_port_vtbl gm_cmd_cache_build_port_vtbl_t;

typedef struct gm_cmd_cache_build_port {
    const gm_cmd_cache_build_port_vtbl_t *vtbl; /* non-owning */
    void *state;                                /* opaque implementation state */
} gm_cmd_cache_build_port_t;

struct gm_cmd_cache_build_port_vtbl {
    /** Request a cache rebuild for a branch. */
    gm_result_void_t (*request_build)(gm_cmd_cache_build_port_t *self,
                                      const char *branch,
                                      bool force_full);

    /** Invalidate cache for a branch (adapter may no-op). */
    gm_result_void_t (*invalidate)(gm_cmd_cache_build_port_t *self,
                                   const char *branch);
};

/* Default implementation factory (thin coordinator over existing APIs). */
GM_NODISCARD gm_result_void_t gm_cmd_cache_build_port_init(
    gm_cmd_cache_build_port_t *port, gm_context_t *ctx);
void gm_cmd_cache_build_port_dispose(gm_cmd_cache_build_port_t *port);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_PORTS_CACHE_BUILD_PORT_H */
