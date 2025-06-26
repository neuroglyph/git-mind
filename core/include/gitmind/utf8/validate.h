/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief UTF-8 validation error codes
 *
 * Specific error types allow better error messages and security responses
 */
typedef enum {
    GM_UTF8_OK = 0,            /* Valid UTF-8 */
    GM_UTF8_ERR_OVERLONG,      /* Overlong encoding detected */
    GM_UTF8_ERR_INVALID_START, /* Invalid start byte */
    GM_UTF8_ERR_TRUNCATED,     /* Truncated sequence */
    GM_UTF8_ERR_SURROGATE,     /* UTF-16 surrogate (D800-DFFF) */
    GM_UTF8_ERR_OUT_OF_RANGE   /* Codepoint > U+10FFFF */
} gm_utf8_error_t;

/**
 * @brief Validate complete UTF-8 buffer
 *
 * Fast-fails on first error. Uses DFA-based validation for high performance.
 *
 * @param buf Buffer to validate
 * @param len Length of buffer
 * @return GM_UTF8_OK or specific error code
 */
gm_utf8_error_t gm_utf8_validate(const char *buf, size_t len);

/**
 * @brief UTF-8 validation state for streaming
 *
 * Allows validation of large inputs in chunks without buffering
 */
typedef struct {
    uint32_t state; /* DFA state */
    uint32_t codep; /* Current codepoint being decoded */
} gm_utf8_state_t;

/**
 * @brief Initialize streaming validation state
 *
 * @param state State to initialize (sets to ACCEPT state)
 */
void gm_utf8_state_init(gm_utf8_state_t *state);

/**
 * @brief Validate a chunk of UTF-8 data
 *
 * Can be called multiple times to validate large inputs.
 * Final chunk should end with state in ACCEPT for valid UTF-8.
 *
 * @param state Validation state (modified)
 * @param buf Buffer chunk to validate
 * @param len Length of chunk
 * @return GM_UTF8_OK if chunk is valid so far, or specific error
 */
gm_utf8_error_t gm_utf8_validate_chunk(gm_utf8_state_t *state, const char *buf,
                                       size_t len);

/**
 * @brief Check if streaming validation is complete
 *
 * @param state Validation state
 * @return true if in ACCEPT state (valid complete UTF-8)
 */
bool gm_utf8_state_is_complete(const gm_utf8_state_t *state);

#ifdef __cplusplus
}
#endif