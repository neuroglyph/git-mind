/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_ADAPTERS_DIAGNOSTICS_STDERR_DIAGNOSTICS_ADAPTER_H
#define GITMIND_ADAPTERS_DIAGNOSTICS_STDERR_DIAGNOSTICS_ADAPTER_H

#include "gitmind/ports/diagnostic_port.h"
#include "gitmind/result.h"

#ifdef __cplusplus
extern "C" {
#endif

GM_NODISCARD gm_result_void_t gm_stderr_diagnostics_port_init(
    gm_diagnostics_port_t *port);
void gm_stderr_diagnostics_port_dispose(gm_diagnostics_port_t *port);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_ADAPTERS_DIAGNOSTICS_STDERR_DIAGNOSTICS_ADAPTER_H */
