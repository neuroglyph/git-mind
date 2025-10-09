/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_ADAPTERS_LOGGING_STDIO_H
#define GITMIND_ADAPTERS_LOGGING_STDIO_H

#include <stdio.h>
#include "gitmind/ports/logger_port.h"
#include "gitmind/result.h"

#ifdef __cplusplus
extern "C" {
#endif

GM_NODISCARD gm_result_void_t gm_stdio_logger_port_init(
    gm_logger_port_t *port, FILE *stream, gm_log_level_t min_level);
void gm_stdio_logger_port_dispose(gm_logger_port_t *port);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_ADAPTERS_LOGGING_STDIO_H */
