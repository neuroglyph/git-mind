/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_TESTS_FAKES_DIAGNOSTICS_FAKE_DIAGNOSTICS_PORT_H
#define GITMIND_TESTS_FAKES_DIAGNOSTICS_FAKE_DIAGNOSTICS_PORT_H

#include <stddef.h>

#include "gitmind/ports/diagnostic_port.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    struct { char component[32]; char event[64]; } meta[64];
    struct { char key[32]; char value[64]; } kvs[64][8];
    size_t kv_counts[64];
    size_t count;
} gm_fake_diag_state_t;

gm_result_void_t gm_fake_diag_port_init(gm_diagnostics_port_t *out,
                                        gm_fake_diag_state_t **out_state);
void gm_fake_diag_port_dispose(gm_diagnostics_port_t *port);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_TESTS_FAKES_DIAGNOSTICS_FAKE_DIAGNOSTICS_PORT_H */

