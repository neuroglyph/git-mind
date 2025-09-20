/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_CACHE_INTERNAL_REBUILD_SERVICE_H
#define GITMIND_CACHE_INTERNAL_REBUILD_SERVICE_H

#include <stdbool.h>

#include "gitmind/context.h"

#ifdef __cplusplus
extern "C" {
#endif

int gm_cache_rebuild_execute(gm_context_t *ctx, const char *branch,
                             bool force_full);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_CACHE_INTERNAL_REBUILD_SERVICE_H */
