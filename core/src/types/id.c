/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/types/id.h"
#include "gitmind/types/id_context.h"
#include "gitmind/error.h"
#include "gitmind/crypto/sha256.h"
#include "gitmind/crypto/random.h"
#include "gitmind/security/memory.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <sodium.h>

/* Compare two IDs */
bool gm_id_equal(gm_id_t id_a, gm_id_t id_b) {
    return memcmp(id_a.bytes, id_b.bytes, GM_ID_SIZE) == 0;
}

/* Compare IDs for ordering */
int gm_id_compare(gm_id_t id_a, gm_id_t id_b) {
    return memcmp(id_a.bytes, id_b.bytes, GM_ID_SIZE);
}

/* 
 * Thread-safe ID operations now use context-based approach.
 * The global state has been moved to gm_id_context_t.
 * See id_context.h for the new thread-safe implementation.
 */

/* Helper to create error result for u32 */
static inline gm_result_u32 gm_err_u32(gm_error_t* err) {
    return (gm_result_u32){ .ok = false, .u.err = err };
}

/* Hash function for hash tables using SipHash-2-4 */
gm_result_u32 gm_id_hash(gm_id_t identifier) {
    /* Get the default context (thread-safe) */
    gm_id_context_t* ctx = gm_id_context_get_default();
    if (!ctx) {
        return gm_err_u32(GM_ERROR(GM_ERR_INVALID_STATE, 
                                  "Failed to get ID context"));
    }
    
    /* Use context-based hash function */
    return gm_id_hash_with_context(ctx, identifier);
}

/* Helper to create error result for ID */
static inline gm_result_id gm_err_id(gm_error_t* err) {
    return (gm_result_id){ .ok = false, .u.err = err };
}

/* Create ID from data */
gm_result_id gm_id_from_data(const void* data, size_t len) {
    if (!data) {
        return gm_err_id(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "NULL data"));
    }
    
    gm_id_t id;
    gm_result_void result = gm_sha256(data, len, id.bytes);
    if (GM_IS_ERR(result)) {
        return gm_err_id(GM_UNWRAP_ERR(result));
    }
    
    return (gm_result_id){ .ok = true, .u.val = id };
}

/* Create ID from string */
gm_result_id gm_id_from_string(const char* str) {
    if (!str) {
        return gm_err_id(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "NULL string"));
    }
    return gm_id_from_data(str, strlen(str));
}

/* Generate random ID */
gm_result_id gm_id_generate(void) {
    gm_id_t id;
    gm_result_void result = gm_random_bytes(id.bytes, GM_ID_SIZE);
    if (GM_IS_ERR(result)) {
        return gm_err_id(GM_UNWRAP_ERR(result));
    }
    
    return (gm_result_id){ .ok = true, .u.val = id };
}

/* Hex conversion constants */
#define HEX_CHARS_PER_BYTE 2
#define HIGH_NIBBLE_SHIFT 4
#define LOW_NIBBLE_MASK 0x0F
#define HEX_DIGIT_STRING "0123456789abcdef"
#define HEX_FORMAT_2X "%2x"

/* Convert ID to hex string (safe version with buffer size check) */
gm_result_void gm_id_to_hex(gm_id_t identifier, char* out, size_t out_size) {
    if (!out) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "NULL output buffer"));
    }
    
    if (out_size < GM_ID_HEX_SIZE) {
        return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL, 
                                   "Buffer too small: need %zu bytes, got %zu",
                                   (size_t)GM_ID_HEX_SIZE, out_size));
    }
    
    static const char hex[] = HEX_DIGIT_STRING;
    
    for (int i = 0; i < GM_ID_SIZE; i++) {
        size_t hex_offset = (size_t)i * HEX_CHARS_PER_BYTE;
        out[hex_offset]     = hex[identifier.bytes[i] >> HIGH_NIBBLE_SHIFT];
        out[hex_offset + 1] = hex[identifier.bytes[i] & LOW_NIBBLE_MASK];
    }
    out[GM_ID_HEX_CHARS] = '\0';
    
    return gm_ok_void();
}

/* Parse hex string to ID */
gm_result_id gm_id_from_hex(const char* hex) {
    gm_id_t id;
    GM_MEMSET_SAFE(&id, sizeof(id), 0, sizeof(id));
    
    if (!hex || strlen(hex) != GM_ID_HEX_CHARS) {
        return (gm_result_id){
            .ok = false,
            .u.err = GM_ERROR(GM_ERR_INVALID_FORMAT, 
                             "Invalid hex ID: must be %d characters", GM_ID_HEX_CHARS)
        };
    }
    
    for (int i = 0; i < GM_ID_SIZE; i++) {
        unsigned int byte;
        if (sscanf(hex + (size_t)i * HEX_CHARS_PER_BYTE, HEX_FORMAT_2X, &byte) != 1) {
            return (gm_result_id){
                .ok = false,
                .u.err = GM_ERROR(GM_ERR_INVALID_FORMAT,
                                 "Invalid hex character at position %d", (int)(i * HEX_CHARS_PER_BYTE))
            };
        }
        id.bytes[i] = (uint8_t)byte;
    }
    
    return (gm_result_id){ .ok = true, .u.val = id };
}

/* Helper to create error result for session ID */
static inline gm_result_session_id gm_err_session_id(gm_error_t* err) {
    return (gm_result_session_id){ .ok = false, .u.err = err };
}

/* Generate session ID */
gm_result_session_id gm_session_id_new(void) {
    gm_result_id result = gm_id_generate();
    if (GM_IS_ERR(result)) {
        return gm_err_session_id(GM_UNWRAP_ERR(result));
    }
    
    gm_session_id_t sid;
    sid.base = GM_UNWRAP(result);
    return (gm_result_session_id){ .ok = true, .u.val = sid };
}