/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/util/oid.h"

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/security/memory.h"
#include "gitmind/types.h"
#include "gitmind/util/memory.h"

static const char KHexDigits[] = "0123456789abcdef";
static const uint8_t KNibbleMask = 0x0FU;
static const unsigned KNibbleBits = 4U;
static const int KHexAlphaOffset = 10;

static int gm_hex_value(char hex_char) {
    if (hex_char >= '0' && hex_char <= '9') {
        return hex_char - '0';
    }
    if (hex_char >= 'a' && hex_char <= 'f') {
        return hex_char - 'a' + KHexAlphaOffset;
    }
    if (hex_char >= 'A' && hex_char <= 'F') {
        return hex_char - 'A' + KHexAlphaOffset;
    }
    return -1;
}

GM_NODISCARD int gm_bytes_to_hex(const uint8_t *bytes, size_t len,
                                 char *out, size_t out_size) {
    if (bytes == NULL || out == NULL) {
        if (out != NULL && out_size > 0U) {
            gm_memset_safe(out, out_size, 0, out_size);
        }
        return GM_ERR_INVALID_ARGUMENT;
    }

    if (len > (SIZE_MAX - 1U) / 2U) {
        gm_memset_safe(out, out_size, 0, out_size);
        return GM_ERR_INVALID_ARGUMENT;
    }

    const size_t required = (len * 2U) + 1U;
    if (out_size < required) {
        gm_memset_safe(out, out_size, 0, out_size);
        return GM_ERR_BUFFER_TOO_SMALL;
    }

    for (size_t i = 0; i < len; ++i) {
        unsigned byte = bytes[i];
        out[i * 2U] = KHexDigits[(byte >> KNibbleBits) & KNibbleMask];
        out[(i * 2U) + 1U] = KHexDigits[byte & KNibbleMask];
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

GM_NODISCARD bool gm_oid_equal(const gm_oid_t *lhs, const gm_oid_t *rhs) {
    if (lhs == NULL || rhs == NULL) {
        return false;
    }
    return memcmp(lhs->id, rhs->id, GM_OID_RAWSZ) == 0;
}

GM_NODISCARD bool gm_oid_is_zero(const gm_oid_t *oid) {
    if (oid == NULL) {
        return false;
    }
    for (size_t index = 0; index < GM_OID_RAWSZ; ++index) {
        if (oid->id[index] != 0U) {
            return false;
        }
    }
    return true;
}

GM_NODISCARD int gm_oid_from_raw(gm_oid_t *oid, const uint8_t *raw,
                                 size_t raw_len) {
    if (oid == NULL) {
        return GM_ERR_INVALID_ARGUMENT;
    }

    gm_memset_safe(oid, sizeof(*oid), 0, sizeof(*oid));

    if (raw == NULL || raw_len != GM_OID_RAWSZ) {
        return GM_ERR_INVALID_ARGUMENT;
    }

    if (gm_memcpy_span(oid->id, GM_OID_RAWSZ, raw, raw_len) != 0) {
        gm_memset_safe(oid, sizeof(*oid), 0, sizeof(*oid));
        return GM_ERR_BUFFER_TOO_SMALL;
    }

    return GM_OK;
}

GM_NODISCARD int gm_oid_from_hex(gm_oid_t *oid, const char *hex) {
    if (oid == NULL) {
        return GM_ERR_INVALID_ARGUMENT;
    }

    gm_memset_safe(oid, sizeof(*oid), 0, sizeof(*oid));

    if (hex == NULL) {
        return GM_ERR_INVALID_ARGUMENT;
    }

    size_t len = strlen(hex);
    if (len != GM_OID_HEX_CHARS) {
        return GM_ERR_INVALID_ARGUMENT;
    }

    for (size_t index = 0; index < GM_OID_RAWSZ; ++index) {
        int high_nibble_value = gm_hex_value(hex[index * 2U]);
        int low_nibble_value = gm_hex_value(hex[(index * 2U) + 1U]);
        if (high_nibble_value < 0 || low_nibble_value < 0) {
            gm_memset_safe(oid, sizeof(*oid), 0, sizeof(*oid));
            return GM_ERR_INVALID_ARGUMENT;
        }
        uint8_t hi_component =
            (uint8_t)(((uint32_t)high_nibble_value) << KNibbleBits);
        uint8_t lo_component = (uint8_t)low_nibble_value;
        oid->id[index] = (uint8_t)(hi_component | lo_component);
    }

    return GM_OK;
}
