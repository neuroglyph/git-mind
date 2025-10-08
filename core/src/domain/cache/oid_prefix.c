/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/cache/internal/oid_prefix.h"

#include <stddef.h>

#include "gitmind/util/oid.h"
#include "gitmind/error.h"

#define BITS_PER_HEX_CHAR 4

GM_NODISCARD int gm_cache_oid_prefix(const gm_oid_t *oid, int bits,
                                     char *out, size_t out_size) {
    if (out == NULL || out_size == 0) {
        return GM_ERR_INVALID_ARGUMENT;
    }
    out[0] = '\0';
    if (oid == NULL || bits <= 0) {
        return GM_OK;
    }
    int chars = (bits + (BITS_PER_HEX_CHAR - 1)) / BITS_PER_HEX_CHAR;
    if (chars > GM_OID_HEX_CHARS) chars = GM_OID_HEX_CHARS;
    if ((size_t)chars >= out_size) {
        /* clamp to buffer, still try to fill */
        chars = (int)out_size - 1;
        if (chars < 0) chars = 0;
    }

    char hex[GM_OID_HEX_CHARS + 1] = {0};
    int rc = gm_oid_to_hex(oid, hex, sizeof(hex));
    if (rc != GM_OK) return rc;

    /* Enforce a conservative maximum prefix for directory names */
    if (chars > (GM_CACHE_MAX_SHARD_PATH - 1)) {
        chars = GM_CACHE_MAX_SHARD_PATH - 1;
    }

    for (int i = 0; i < chars; ++i) {
        out[i] = hex[i];
    }
    out[chars] = '\0';
    return GM_OK;
}
