/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "fake_metrics_port.h"

#include <stdlib.h>

#include "gitmind/error.h"
#include "gitmind/security/string.h"
#include "gitmind/util/memory.h"

static gm_result_void_t counter_add(void *self, const char *name, uint64_t value,
                                    const char *tags) {
    gm_fake_metrics_state_t *st = (gm_fake_metrics_state_t *)self;
    if (st == NULL) return gm_ok_void();
    if (st->counter_count >= 32) return gm_ok_void();
    size_t i = st->counter_count++;
    (void)gm_strcpy_safe(st->counters[i].name, sizeof(st->counters[i].name),
                         name ? name : "");
    st->counters[i].value = value;
    (void)gm_strcpy_safe(st->counters[i].tags, sizeof(st->counters[i].tags),
                         tags ? tags : "");
    return gm_ok_void();
}

static gm_result_void_t gauge_set(void *self, const char *name, double value,
                                  const char *tags) {
    gm_fake_metrics_state_t *st = (gm_fake_metrics_state_t *)self;
    if (st == NULL) return gm_ok_void();
    if (st->gauge_count >= 32) return gm_ok_void();
    size_t i = st->gauge_count++;
    (void)gm_strcpy_safe(st->gauges[i].name, sizeof(st->gauges[i].name),
                         name ? name : "");
    st->gauges[i].value = value;
    (void)gm_strcpy_safe(st->gauges[i].tags, sizeof(st->gauges[i].tags),
                         tags ? tags : "");
    return gm_ok_void();
}

static gm_result_void_t timing_ms(void *self, const char *name, uint64_t millis,
                                  const char *tags) {
    gm_fake_metrics_state_t *st = (gm_fake_metrics_state_t *)self;
    if (st == NULL) return gm_ok_void();
    if (st->timing_count >= 32) return gm_ok_void();
    size_t i = st->timing_count++;
    (void)gm_strcpy_safe(st->timings[i].name, sizeof(st->timings[i].name),
                         name ? name : "");
    st->timings[i].millis = millis;
    (void)gm_strcpy_safe(st->timings[i].tags, sizeof(st->timings[i].tags),
                         tags ? tags : "");
    return gm_ok_void();
}

static const gm_metrics_port_vtbl_t VTBL = {
    .counter_add = counter_add,
    .gauge_set = gauge_set,
    .timing_ms = timing_ms,
};

gm_result_void_t gm_fake_metrics_port_init(gm_metrics_port_t *out,
                                           gm_fake_metrics_state_t **out_state) {
    if (out == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "fake metrics requires out"));
    }
    gm_fake_metrics_state_t *st = calloc(1, sizeof(*st));
    if (st == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_OUT_OF_MEMORY, "alloc metrics state"));
    }
    out->vtbl = &VTBL;
    out->self = st;
    if (out_state) *out_state = st;
    return gm_ok_void();
}

void gm_fake_metrics_port_dispose(gm_metrics_port_t *port) {
    if (port == NULL) return;
    free(port->self);
    port->self = NULL;
    port->vtbl = NULL;
}
