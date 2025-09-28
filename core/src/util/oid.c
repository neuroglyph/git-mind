/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/util/oid.h"

#include "gitmind/error.h"
#include "gitmind/security/memory.h"

static const char kHexDigits[] = "0123456789abcdef";

GM_NODISCARD int gm_bytes_to_hex(const uint8_t *bytes, size_t len,
                                 char *out, size_t out_size) {
    if (bytes == NULL || out == NULL) {
        if (out != NULL && out_size > 0U) {
            gm_memset_safe(out, out_size, 0, out_size);
        }
        return GM_ERR_INVALID_ARGUMENT;
    }

    const size_t required = (len * 2U) + 1U;
    if (out_size < required) {
        gm_memset_safe(out, out_size, 0, out_size);
        return GM_ERR_BUFFER_TOO_SMALL;
    }

    for (size_t i = 0; i < len; ++i) {
        unsigned byte = bytes[i];
        out[i * 2U] = kHexDigits[(byte >> 4U) & 0x0FU];
        out[(i * 2U) + 1U] = kHexDigits[byte & 0x0FU];
    }
    out[len * 2U] = '\0';
    return GM_OK;
}

GM_NODISCARD int gm_oid_to_hex(const gm_oid_t *oid, char *out, size_t out_size) {
    if (oid == NULL) {
        if (out != NULL && out_size > 0U) {
            gm_memset_safe(out, out_size, 0, out_size);
        }
        return GM_ERR_INVALID_ARGUMENT;
    }
    return gm_bytes_to_hex(oid->id, GM_OID_RAWSZ, out, out_size);
}
