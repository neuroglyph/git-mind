/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_PORTS_LOGGER_PORT_H
#define GITMIND_PORTS_LOGGER_PORT_H

#include <stddef.h>

#include "gitmind/result.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file logger_port.h
 * @brief Outbound port for structured logging.
 *
 * Design goals:
 * - Umbrella-safe public header with C++ guards handled by the umbrella.
 * - Minimal vtable surface (single log() entry point) that adapters can
 *   implement using stdio, syslog, or external backends.
 * - Defaults to a safe no-op via gm_logger_log() when the port is unset,
 *   allowing callers to instrument without conditional compilation.
 *
 * Thread-safety and lifetime:
 * - Implementations should be reentrant and thread-safe; the port carries
 *   an opaque state pointer managed by the adapter.
 * - The runtime owns adapter state; dispose functions are provided by the
 *   adapter and should free any allocated resources.
 */

typedef enum {
    GM_LOG_DEBUG = 10,
    GM_LOG_INFO  = 20,
    GM_LOG_WARN  = 30,
    GM_LOG_ERROR = 40,
} gm_log_level_t;

typedef struct gm_logger_port_vtbl gm_logger_port_vtbl_t;

typedef struct gm_logger_port {
    const gm_logger_port_vtbl_t *vtbl; /* non-owning */
    void *self;                         /* opaque adapter state */
} gm_logger_port_t;

struct gm_logger_port_vtbl {
    /**
     * Write a single structured log entry.
     *
     * @param self      Adapter state (opaque to callers).
     * @param level     Severity level (DEBUG..ERROR).
     * @param component Subsystem/component name (e.g., "cache").
     * @param message   Message payload (UTF-8, NUL-terminated).
     * @return gm_result_void_t indicating success or failure.
     */
    gm_result_void_t (*log)(void *self,
                            gm_log_level_t level,
                            const char *component,
                            const char *message);
};

/* Convenience inline wrappers */
/**
 * Convenience wrapper that logs when the port is available, otherwise no-ops.
 * Safe to call unconditionally by application services.
 */
static inline gm_result_void_t gm_logger_log(const gm_logger_port_t *port,
                                             gm_log_level_t level,
                                             const char *component,
                                             const char *message) {
    if (port == NULL || port->vtbl == NULL || port->vtbl->log == NULL) {
        return gm_ok_void(); /* Default to no-op logging */
    }
    return port->vtbl->log(port->self, level, component, message);
}

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_PORTS_LOGGER_PORT_H */
