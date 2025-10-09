/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_ADAPTERS_METRICS_NULL_H
#define GITMIND_ADAPTERS_METRICS_NULL_H

#include "gitmind/ports/metrics_port.h"
#include "gitmind/result.h"

#ifdef __cplusplus
extern "C" {
#endif

GM_NODISCARD gm_result_void_t gm_null_metrics_port_init(gm_metrics_port_t *port);
void gm_null_metrics_port_dispose(gm_metrics_port_t *port);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_ADAPTERS_METRICS_NULL_H */
