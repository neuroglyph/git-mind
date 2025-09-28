/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_UTIL_OID_H
#define GITMIND_UTIL_OID_H

#include <stddef.h>
#include <stdint.h>

#include "gitmind/result.h"
#include "gitmind/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Convert raw bytes to a lowercase hexadecimal string.
 *
 * @param bytes     Input byte array
 * @param len       Number of bytes to convert
 * @param out       Output buffer for the hex string
 * @param out_size  Size of the output buffer in bytes
 * @return GM_OK on success, GM_ERR_INVALID_ARGUMENT or GM_ERR_BUFFER_TOO_SMALL otherwise
 */
GM_NODISCARD int gm_bytes_to_hex(const uint8_t *bytes, size_t len,
                                 char *out, size_t out_size);

/**
 * Convert a gm_oid_t into its hexadecimal string form.
 *
 * @param oid       Pointer to the OID to encode
 * @param out       Output buffer for the hex string (including null terminator)
 * @param out_size  Size of the output buffer in bytes
 * @return GM_OK on success, GM_ERR_INVALID_ARGUMENT or GM_ERR_BUFFER_TOO_SMALL otherwise
 */
GM_NODISCARD int gm_oid_to_hex(const gm_oid_t *oid, char *out, size_t out_size);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_UTIL_OID_H */
