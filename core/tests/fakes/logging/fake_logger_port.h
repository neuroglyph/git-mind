/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_TESTS_FAKES_LOGGING_FAKE_LOGGER_PORT_H
#define GITMIND_TESTS_FAKES_LOGGING_FAKE_LOGGER_PORT_H

#include <stddef.h>
#include <stdint.h>

#include "gitmind/ports/logger_port.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    gm_log_level_t level[32];
    char component[32][32];
    char message[32][256];
    size_t count;
} gm_fake_logger_state_t;

gm_result_void_t gm_fake_logger_port_init(gm_logger_port_t *out,
                                          gm_fake_logger_state_t **out_state);
void gm_fake_logger_port_dispose(gm_logger_port_t *port);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_TESTS_FAKES_LOGGING_FAKE_LOGGER_PORT_H */

