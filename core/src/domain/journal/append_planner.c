/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/journal/internal/append_plan.h"

#include <string.h>

#include "gitmind/error.h"
#include "gitmind/result.h"

GM_NODISCARD gm_result_void_t gm_journal_build_commit_plan(
    const gm_oid_t *empty_tree_oid,
    const gm_oid_t *parent_oid_opt,
    const char *message,
    gm_journal_commit_plan_t *out_plan) {
    if (empty_tree_oid == NULL || message == NULL || out_plan == NULL) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, "append plan requires inputs"));
    }
    out_plan->tree_oid = empty_tree_oid;
    out_plan->message = message;
    out_plan->parents = parent_oid_opt;
    out_plan->parent_count = (parent_oid_opt != NULL) ? 1U : 0U;
    return gm_ok_void();
}

