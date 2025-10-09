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
    const gm_oid_t *tree_oid;           /* required, borrowed; must outlive plan use */
    const char *message;                /* required, borrowed; base64-encoded CBOR; non-NULL */
    const gm_oid_t *parents;            /* optional, borrowed; single parent when parent_count == 1 */
    size_t parent_count;                /* 0 or 1 for now */
} gm_journal_commit_plan_t;

/* Build a commit plan from inputs without performing IO.
 * Parameters:
 *  - tree_oid: required, non-NULL borrowed pointer to the commit tree OID.
 *  - parent_oid_opt: optional parent OID (borrowed). NULL when parent_count == 0.
 *  - message: required, non-NULL borrowed pointer to base64-encoded CBOR payload.
 *  - out_plan: required, non-NULL destination for the resulting plan.
 */
GM_NODISCARD gm_result_void_t gm_journal_build_commit_plan(
    const gm_oid_t *tree_oid,
    const gm_oid_t *parent_oid_opt,
    const char *message,
    gm_journal_commit_plan_t *out_plan);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_JOURNAL_INTERNAL_APPEND_PLAN_H */
