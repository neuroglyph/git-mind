/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "null_metrics_adapter.h"
#include "gitmind/error.h"
#include "gitmind/result.h"
#include <stdlib.h>

typedef struct {
    int _unused;
} null_metrics_state_t;

static gm_result_void_t counter_add_impl(void *self, const char *name,
                                         uint64_t value, const char *tags) {
    (void)self; (void)name; (void)value; (void)tags; return gm_ok_void();
}

static gm_result_void_t gauge_set_impl(void *self, const char *name,
                                       double value, const char *tags) {
    (void)self; (void)name; (void)value; (void)tags; return gm_ok_void();
}

static gm_result_void_t timing_ms_impl(void *self, const char *name,
                                       uint64_t millis, const char *tags) {
    (void)self; (void)name; (void)millis; (void)tags; return gm_ok_void();
}

static const gm_metrics_port_vtbl_t NULL_VTBL = {
    .counter_add = counter_add_impl,
    .gauge_set   = gauge_set_impl,
    .timing_ms   = timing_ms_impl,
};

GM_NODISCARD gm_result_void_t gm_null_metrics_port_init(gm_metrics_port_t *port) {
    if (port == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "metrics port out is null"));
    }
    null_metrics_state_t *state = (null_metrics_state_t *)calloc(1, sizeof(*state));
    if (state == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_OUT_OF_MEMORY, "allocating metrics state"));
    }
    port->vtbl = &NULL_VTBL;
    port->self = state;
    return gm_ok_void();
}

void gm_null_metrics_port_dispose(gm_metrics_port_t *port) {
    if (port == NULL) return;
    free(port->self);
    port->self = NULL;
    port->vtbl = NULL;
}
