/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_PORTS_METRICS_PORT_H
#define GITMIND_PORTS_METRICS_PORT_H

#include <stdint.h>

#include "gitmind/result.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file metrics_port.h
 * @brief Outbound port for metrics emission (counters, gauges, timings).
 *
 * Semantics:
 * - Names: ASCII identifiers using segment.case (e.g., "cache.edges_total").
 * - Tags: Optional key=value pairs separated by commas (e.g., "branch=main").
 * - Units: timing_ms() expects milliseconds; gauges are unit-less doubles;
 *   counters are monotonically increasing deltas.
 *
 * Defaults:
 * - gm_metrics_* wrappers no-op when the port is unset to keep callers simple.
 *
 * Thread-safety: Adapter implementations should be thread-safe.
 */

typedef struct gm_metrics_port_vtbl gm_metrics_port_vtbl_t;

typedef struct gm_metrics_port {
    const gm_metrics_port_vtbl_t *vtbl; /* non-owning */
    void *self;                          /* opaque adapter state */
} gm_metrics_port_t;

struct gm_metrics_port_vtbl {
    /** Add a delta to a counter metric. */
    gm_result_void_t (*counter_add)(void *self, const char *name,
                                    uint64_t value, const char *tags);
    /** Set an absolute value for a gauge metric. */
    gm_result_void_t (*gauge_set)(void *self, const char *name,
                                  double value, const char *tags);
    /** Record a timing metric in milliseconds. */
    gm_result_void_t (*timing_ms)(void *self, const char *name,
                                  uint64_t millis, const char *tags);
};

/* Convenience inline wrappers default to no-op success when unset */
/** No-op when unset; otherwise calls adapter. */
static inline gm_result_void_t gm_metrics_counter_add(
    const gm_metrics_port_t *port, const char *name, uint64_t value,
    const char *tags) {
    if (port == NULL || port->vtbl == NULL || port->vtbl->counter_add == NULL) {
        return gm_ok_void();
    }
    return port->vtbl->counter_add(port->self, name, value, tags);
}

/** No-op when unset; otherwise calls adapter. */
static inline gm_result_void_t gm_metrics_gauge_set(
    const gm_metrics_port_t *port, const char *name, double value,
    const char *tags) {
    if (port == NULL || port->vtbl == NULL || port->vtbl->gauge_set == NULL) {
        return gm_ok_void();
    }
    return port->vtbl->gauge_set(port->self, name, value, tags);
}

/** No-op when unset; otherwise calls adapter. */
static inline gm_result_void_t gm_metrics_timing_ms(
    const gm_metrics_port_t *port, const char *name, uint64_t millis,
    const char *tags) {
    if (port == NULL || port->vtbl == NULL || port->vtbl->timing_ms == NULL) {
        return gm_ok_void();
    }
    return port->vtbl->timing_ms(port->self, name, millis, tags);
}

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_PORTS_METRICS_PORT_H */
