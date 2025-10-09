/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_TESTS_FAKES_DIAGNOSTICS_FAKE_DIAGNOSTICS_PORT_H
#define GITMIND_TESTS_FAKES_DIAGNOSTICS_FAKE_DIAGNOSTICS_PORT_H

#include <stddef.h>

#include "gitmind/ports/diagnostic_port.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GM_FAKE_DIAG_MAX_EVENTS 64
#define GM_FAKE_DIAG_MAX_KVS_PER_EVENT 8
#define GM_FAKE_DIAG_MAX_COMPONENT 32
#define GM_FAKE_DIAG_MAX_EVENT 64
#define GM_FAKE_DIAG_MAX_KEY 32
#define GM_FAKE_DIAG_MAX_VALUE 64

typedef struct {
    struct { char component[GM_FAKE_DIAG_MAX_COMPONENT];
             char event[GM_FAKE_DIAG_MAX_EVENT]; } meta[GM_FAKE_DIAG_MAX_EVENTS];
    struct { char key[GM_FAKE_DIAG_MAX_KEY];
             char value[GM_FAKE_DIAG_MAX_VALUE]; }
        kvs[GM_FAKE_DIAG_MAX_EVENTS][GM_FAKE_DIAG_MAX_KVS_PER_EVENT];
    size_t kv_counts[GM_FAKE_DIAG_MAX_EVENTS];
    size_t count;
} gm_fake_diag_state_t;

gm_result_void_t gm_fake_diag_port_init(gm_diagnostics_port_t *out,
                                        gm_fake_diag_state_t **out_state);
void gm_fake_diag_port_dispose(gm_diagnostics_port_t *port);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_TESTS_FAKES_DIAGNOSTICS_FAKE_DIAGNOSTICS_PORT_H */
