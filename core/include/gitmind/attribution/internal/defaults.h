/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_ATTRIBUTION_INTERNAL_DEFAULTS_H
#define GITMIND_ATTRIBUTION_INTERNAL_DEFAULTS_H

#include "gitmind/attribution.h"
#include "gitmind/result.h"

GM_NODISCARD gm_result_void_t
    gm_attribution_defaults_apply(gm_attribution_t *attr, gm_source_type_t source);

#endif /* GITMIND_ATTRIBUTION_INTERNAL_DEFAULTS_H */
