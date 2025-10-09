/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "fake_metrics_port.h"

#include <stdlib.h>
#include <string.h>

#include "gitmind/error.h"
#include "gitmind/security/string.h"
#include "gitmind/util/memory.h"

static gm_result_void_t counter_add(void *self, const char *name, uint64_t value,
                                    const char *tags) {
    gm_fake_metrics_state_t *st = (gm_fake_metrics_state_t *)self;
    if (st == NULL) return gm_ok_void();
    if (st->counter_count >= 32) return gm_ok_void();
    size_t slot = st->counter_count;
    if (gm_strcpy_safe(st->counters[slot].name,
                       sizeof(st->counters[slot].name),
                       name ? name : "") != GM_OK) {
        memset(&st->counters[slot], 0, sizeof(st->counters[slot]));
        return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL,
                                    "fake metrics counter name truncated"));
    }
    st->counters[slot].value = value;
    if (gm_strcpy_safe(st->counters[slot].tags,
                       sizeof(st->counters[slot].tags),
                       tags ? tags : "") != GM_OK) {
        memset(&st->counters[slot], 0, sizeof(st->counters[slot]));
        return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL,
                                    "fake metrics counter tags truncated"));
    }
    st->counter_count = slot + 1;
    return gm_ok_void();
}

static gm_result_void_t gauge_set(void *self, const char *name, double value,
                                  const char *tags) {
    gm_fake_metrics_state_t *st = (gm_fake_metrics_state_t *)self;
    if (st == NULL) return gm_ok_void();
    if (st->gauge_count >= 32) return gm_ok_void();
    size_t slot = st->gauge_count;
    if (gm_strcpy_safe(st->gauges[slot].name,
                       sizeof(st->gauges[slot].name),
                       name ? name : "") != GM_OK) {
        memset(&st->gauges[slot], 0, sizeof(st->gauges[slot]));
        return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL,
                                    "fake metrics gauge name truncated"));
    }
    st->gauges[slot].value = value;
    if (gm_strcpy_safe(st->gauges[slot].tags,
                       sizeof(st->gauges[slot].tags),
                       tags ? tags : "") != GM_OK) {
        memset(&st->gauges[slot], 0, sizeof(st->gauges[slot]));
        return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL,
                                    "fake metrics gauge tags truncated"));
    }
    st->gauge_count = slot + 1;
    return gm_ok_void();
}

static gm_result_void_t timing_ms(void *self, const char *name, uint64_t millis,
                                  const char *tags) {
    gm_fake_metrics_state_t *st = (gm_fake_metrics_state_t *)self;
    if (st == NULL) return gm_ok_void();
    if (st->timing_count >= 32) return gm_ok_void();
    size_t slot = st->timing_count;
    if (gm_strcpy_safe(st->timings[slot].name,
                       sizeof(st->timings[slot].name),
                       name ? name : "") != GM_OK) {
        memset(&st->timings[slot], 0, sizeof(st->timings[slot]));
        return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL,
                                    "fake metrics timing name truncated"));
    }
    st->timings[slot].millis = millis;
    if (gm_strcpy_safe(st->timings[slot].tags,
                       sizeof(st->timings[slot].tags),
                       tags ? tags : "") != GM_OK) {
        memset(&st->timings[slot], 0, sizeof(st->timings[slot]));
        return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL,
                                    "fake metrics timing tags truncated"));
    }
    st->timing_count = slot + 1;
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
