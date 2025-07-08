/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_CBOR_CBOR_H
#define GITMIND_CBOR_CBOR_H

#include <gitmind/result.h>
#include <stddef.h>
#include <stdint.h>

/* Result types for CBOR operations */
GM_RESULT_DEF(gm_result_uint64, uint64_t);

/**
 * CBOR read operations with Result types.
 * All operations return Result types for proper error handling.
 */

/**
 * Read a CBOR unsigned integer from buffer.
 * @param buf Buffer to read from
 * @param offset Pointer to current offset (updated on success)
 * @param max_size Maximum buffer size to prevent overruns
 * @return Result containing the uint64_t value or error
 */
gm_result_uint64_t gm_cbor_read_uint(const uint8_t *buf, size_t *offset, size_t max_size);

/**
 * Read a CBOR byte string from buffer.
 * @param buf Buffer to read from
 * @param offset Pointer to current offset (updated on success)
 * @param max_size Maximum buffer size to prevent overruns
 * @param data Buffer to write bytes to
 * @param expected_len Expected length of byte string
 * @return Result indicating success or error
 */
gm_result_void_t gm_cbor_read_bytes(const uint8_t *buf, size_t *offset, size_t max_size,
                                    uint8_t *data, size_t expected_len);

/**
 * Read a CBOR text string from buffer.
 * @param buf Buffer to read from
 * @param offset Pointer to current offset (updated on success)
 * @param max_size Maximum buffer size to prevent overruns
 * @param text Buffer to write text to (null-terminated)
 * @param max_text_len Maximum text buffer size
 * @return Result indicating success or error
 */
gm_result_void_t gm_cbor_read_text(const uint8_t *buf, size_t *offset, size_t max_size,
                                   char *text, size_t max_text_len);

/**
 * CBOR write operations with Result types.
 */

/**
 * Write a CBOR unsigned integer to buffer.
 * @param value Value to write
 * @param buf Buffer to write to
 * @param buf_size Size of output buffer
 * @return Result containing number of bytes written or error
 */
gm_result_size_t gm_cbor_write_uint(uint64_t value, uint8_t *buf, size_t buf_size);

/**
 * Write a CBOR byte string to buffer.
 * @param buf Buffer to write to
 * @param buf_size Size of output buffer
 * @param data Bytes to write
 * @param data_len Length of bytes to write
 * @return Result containing number of bytes written or error
 */
gm_result_size_t gm_cbor_write_bytes(uint8_t *buf, size_t buf_size,
                                     const uint8_t *data, size_t data_len);

/**
 * Write a CBOR text string to buffer.
 * @param buf Buffer to write to
 * @param buf_size Size of output buffer
 * @param text Null-terminated text to write
 * @return Result containing number of bytes written or error
 */
gm_result_size_t gm_cbor_write_text(uint8_t *buf, size_t buf_size, const char *text);

#endif /* GITMIND_CBOR_CBOR_H */
