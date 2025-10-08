/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_JOURNAL_INTERNAL_APPEND_PLAN_H
#define GITMIND_JOURNAL_INTERNAL_APPEND_PLAN_H

#include <stddef.h>

#include "gitmind/util/oid.h"
#include "gitmind/result.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gm_journal_commit_plan {
    const gm_oid_t *tree_oid;           /* required */
    const char *message;                /* required, base64-encoded CBOR */
    const gm_oid_t *parents;            /* optional pointer to single parent */
    size_t parent_count;                /* 0 or 1 for now */
} gm_journal_commit_plan_t;

/* Build a commit plan from inputs without performing IO. */
GM_NODISCARD gm_result_void_t gm_journal_build_commit_plan(
    const gm_oid_t *empty_tree_oid,
    const gm_oid_t *parent_oid_opt,
    const char *message,
    gm_journal_commit_plan_t *out_plan);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_JOURNAL_INTERNAL_APPEND_PLAN_H */

