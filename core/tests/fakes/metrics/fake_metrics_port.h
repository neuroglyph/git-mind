/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_TESTS_FAKES_METRICS_FAKE_METRICS_PORT_H
#define GITMIND_TESTS_FAKES_METRICS_FAKE_METRICS_PORT_H

#include <stddef.h>
#include <stdint.h>

#include "gitmind/ports/metrics_port.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    struct { char name[64]; uint64_t value; char tags[256]; } counters[32];
    struct { char name[64]; double value; char tags[256]; } gauges[32];
    struct { char name[64]; uint64_t millis; char tags[256]; } timings[32];
    size_t counter_count;
    size_t gauge_count;
    size_t timing_count;
} gm_fake_metrics_state_t;

gm_result_void_t gm_fake_metrics_port_init(gm_metrics_port_t *out,
                                           gm_fake_metrics_state_t **out_state);
void gm_fake_metrics_port_dispose(gm_metrics_port_t *port);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_TESTS_FAKES_METRICS_FAKE_METRICS_PORT_H */

