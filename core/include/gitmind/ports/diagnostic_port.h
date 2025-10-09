/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_PORTS_DIAGNOSTIC_PORT_H
#define GITMIND_PORTS_DIAGNOSTIC_PORT_H

#include <stddef.h>

#include "gitmind/result.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Key/value for diagnostic events */
typedef struct gm_diag_kv {
    const char *key;
    const char *value;
} gm_diag_kv_t;

typedef struct gm_diagnostics_port_vtbl gm_diagnostics_port_vtbl_t;

typedef struct gm_diagnostics_port {
    const gm_diagnostics_port_vtbl_t *vtbl; /* non-owning */
    void *self;                              /* opaque */
} gm_diagnostics_port_t;

struct gm_diagnostics_port_vtbl {
    gm_result_void_t (*emit)(void *self,
                             const char *component,
                             const char *event,
                             const gm_diag_kv_t *kvs,
                             size_t kv_count);
};

/* Safe wrapper: no-op when unset */
static inline gm_result_void_t gm_diag_emit(const gm_diagnostics_port_t *port,
                                            const char *component,
                                            const char *event,
                                            const gm_diag_kv_t *kvs,
                                            size_t kv_count) {
    if (port == NULL || port->vtbl == NULL || port->vtbl->emit == NULL) {
        return gm_ok_void();
    }
    return port->vtbl->emit(port->self, component, event, kvs, kv_count);
}

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_PORTS_DIAGNOSTIC_PORT_H */

