/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/edge/internal/blob_identity.h"

#include "gitmind/error.h"
#include "gitmind/ports/git_repository_port.h"
#include "gitmind/security/memory.h"
#include "gitmind/util/memory.h"

#include <string.h>

GM_NODISCARD gm_result_void_t gm_edge_resolve_blob_identity(
    gm_context_t *ctx,
    const char *path,
    gm_oid_t *out_oid,
    uint8_t *legacy_sha) {
    if (ctx == NULL || path == NULL || out_oid == NULL || legacy_sha == NULL) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, "blob resolve requires inputs"));
    }
    if (ctx->git_repo_port.vtbl == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_STATE,
                                    "git repository port unavailable"));
    }

    gm_result_void_t port_result = gm_git_repository_port_resolve_blob_at_head(
        &ctx->git_repo_port, path, out_oid);
    if (!port_result.ok) {
        return port_result;
    }

    const size_t copy_len = (GM_OID_RAWSZ < GM_SHA1_SIZE) ? GM_OID_RAWSZ : GM_SHA1_SIZE;
    if (gm_memcpy_span(legacy_sha, GM_SHA1_SIZE, out_oid->id, copy_len) != 0) {
        gm_memset_safe(legacy_sha, GM_SHA1_SIZE, 0, GM_SHA1_SIZE);
        return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL,
                                    "legacy SHA buffer too small"));
    }

    if (copy_len < GM_SHA1_SIZE) {
        memset(legacy_sha + copy_len, 0, GM_SHA1_SIZE - copy_len);
    }

    return gm_ok_void();
}
