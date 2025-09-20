/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/cache.h"
#include "gitmind/cache/internal/rebuild_service.h"
#include "gitmind/context.h"
#include "gitmind/error.h"

int gm_cache_rebuild(gm_context_t *ctx, const char *branch, bool force_full) {
    if (ctx == NULL || ctx->git_repo == NULL || branch == NULL) {
        return GM_ERR_INVALID_ARGUMENT;
    }

    return gm_cache_rebuild_execute(ctx, branch, force_full);
}
