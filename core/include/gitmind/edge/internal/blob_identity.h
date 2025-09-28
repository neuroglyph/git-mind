/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_EDGE_INTERNAL_BLOB_IDENTITY_H
#define GITMIND_EDGE_INTERNAL_BLOB_IDENTITY_H

#include "gitmind/context.h"
#include "gitmind/result.h"
#include "gitmind/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Resolve the blob identity for a file path using the current repository head.
 *
 * On success, both the preferred OID (`out_oid`) and the legacy SHA-1 buffer
 * (`legacy_sha`, sized to `GM_SHA1_SIZE`) are populated. When `GM_OID_RAWSZ`
 * exceeds `GM_SHA1_SIZE`, the legacy buffer is zero-padded to maintain
 * deterministic contents. Callers must provide non-null pointers.
 */
GM_NODISCARD gm_result_void_t gm_edge_resolve_blob_identity(
    gm_context_t *ctx,
    const char *path,
    gm_oid_t *out_oid,
    uint8_t *legacy_sha);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_EDGE_INTERNAL_BLOB_IDENTITY_H */
