/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_ATTRIBUTION_INTERNAL_ENV_LOADER_H
#define GITMIND_ATTRIBUTION_INTERNAL_ENV_LOADER_H

#include "gitmind/attribution.h"
#include "gitmind/ports/env_port.h"
#include "gitmind/result.h"

GM_NODISCARD gm_result_void_t gm_attribution_from_env_with_port(
    gm_attribution_t *attr, const gm_env_port_t *port);

#endif /* GITMIND_ATTRIBUTION_INTERNAL_ENV_LOADER_H */
