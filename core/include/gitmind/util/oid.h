/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_UTIL_OID_H
#define GITMIND_UTIL_OID_H

#include <stdbool.h>
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

/**
 * Test whether two OIDs refer to the same object.
 *
 * @param lhs  Left-hand OID operand
 * @param rhs  Right-hand OID operand
 * @return true when both pointers are non-null and contain identical raw bytes
 */
GM_NODISCARD bool gm_oid_equal(const gm_oid_t *lhs, const gm_oid_t *rhs);

/**
 * Determine whether an OID is the all-zero sentinel.
 *
 * @param oid  OID to inspect; NULL is treated as invalid / not zero
 * @return true when every byte is zero; false otherwise (including NULL)
 */
GM_NODISCARD bool gm_oid_is_zero(const gm_oid_t *oid);

/**
 * Populate an OID from raw bytes.
 *
 * @param oid      Destination OID (must not be NULL)
 * @param raw      Source byte buffer
 * @param raw_len  Number of bytes provided in `raw`
 * @return GM_OK on success, GM_ERR_INVALID_ARGUMENT when parameters are invalid
 */
GM_NODISCARD int gm_oid_from_raw(gm_oid_t *oid, const uint8_t *raw,
                                 size_t raw_len);

/**
 * Parse a hexadecimal string into an OID.
 *
 * @param oid  Destination OID (cleared on failure)
 * @param hex  Null-terminated hex string (must be GM_OID_HEX_CHARS long)
 * @return GM_OK on success, GM_ERR_INVALID_ARGUMENT otherwise
 */
GM_NODISCARD int gm_oid_from_hex(gm_oid_t *oid, const char *hex);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_UTIL_OID_H */
