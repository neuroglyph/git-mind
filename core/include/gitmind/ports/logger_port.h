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
    gm_result_void_t (*log)(void *self,
                            gm_log_level_t level,
                            const char *component,
                            const char *message);
};

/* Convenience inline wrappers */
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

