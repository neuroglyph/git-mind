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
 * Purpose:
 * - Provide a stable application entry point (from CLI/API) to request cache
 *   operations without exposing implementation details.
 * - Keep domain logic pure; coordinators call into services which consume
 *   outbound ports (git repo, fs, logger, metrics).
 *
 * Lifecycle:
 * - Initialize via gm_cmd_cache_build_port_init(), passing a gm_context_t that
 *   holds outbound ports.
 * - Dispose with gm_cmd_cache_build_port_dispose() when done.
 */

typedef struct gm_cmd_cache_build_port_vtbl gm_cmd_cache_build_port_vtbl_t;

typedef struct gm_cmd_cache_build_port {
    const gm_cmd_cache_build_port_vtbl_t *vtbl; /* non-owning */
    void *state;                                /* opaque implementation state */
} gm_cmd_cache_build_port_t;

struct gm_cmd_cache_build_port_vtbl {
    /**
     * Request a cache rebuild for a branch.
     *
     * @param self       Port instance.
     * @param branch     Branch name (without "refs/").
     * @param force_full If true, ignore incremental hints and rebuild fully.
     * @return gm_result_void_t indicating success or error code.
     */
    gm_result_void_t (*request_build)(gm_cmd_cache_build_port_t *self,
                                      const char *branch,
                                      bool force_full);

    /**
     * Invalidate cache for a branch (optional; may be a no-op depending on
     * implementation).
     */
    gm_result_void_t (*invalidate)(gm_cmd_cache_build_port_t *self,
                                   const char *branch);
};

/* Default implementation factory (thin coordinator over existing APIs). */
/** Initialize a cache build port using dependencies from the provided context. */
GM_NODISCARD gm_result_void_t gm_cmd_cache_build_port_init(
    gm_cmd_cache_build_port_t *port, gm_context_t *ctx);
/** Release any resources held by a cache build port. */
void gm_cmd_cache_build_port_dispose(gm_cmd_cache_build_port_t *port);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_PORTS_CACHE_BUILD_PORT_H */
