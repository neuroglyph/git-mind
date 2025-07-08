/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/types/id.h"

#include "gitmind/crypto/backend.h"
#include "gitmind/crypto/random.h"
#include "gitmind/crypto/sha256.h"
#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/security/memory.h"

#include <stdatomic.h>
#include <stddef.h>
#include <stdlib.h>
#include <sodium/crypto_shorthash_siphash24.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

/* Compare two IDs */
bool gm_id_equal(gm_id_t new_id_a, gm_id_t new_id_b) {
    return memcmp(new_id_a.bytes, new_id_b.bytes, GM_ID_SIZE) == 0;
}

/* Compare IDs for ordering */
int gm_id_compare(gm_id_t new_id_a, gm_id_t new_id_b) {
    return memcmp(new_id_a.bytes, new_id_b.bytes, GM_ID_SIZE);
}

/* Constants for deterministic SipHash key derivation */
#define SIPHASH_KEY_SALT "gitmind_id_hash_salt_v1"
#define SIPHASH_KEY_SALT_LEN 22

/* Helper to create key derivation input */
static gm_result_void_t create_key_input(const char *backend_name, char **input, size_t *total_len) {
    if (!backend_name) {
        backend_name = "unknown";
    }
    
    size_t backend_len = strlen(backend_name);
    *total_len = backend_len + SIPHASH_KEY_SALT_LEN;
    *input = malloc(*total_len);
    if (!*input) {
        return gm_err_void(GM_ERROR(GM_ERR_OUT_OF_MEMORY, "Failed to allocate derivation input"));
    }
    
    GM_MEMCPY_SAFE(*input, *total_len, backend_name, backend_len);
    GM_MEMCPY_SAFE(*input + backend_len, *total_len - backend_len, SIPHASH_KEY_SALT, SIPHASH_KEY_SALT_LEN);
    return gm_ok_void();
}

/* Derive deterministic SipHash key from crypto context */
static gm_result_void_t derive_siphash_key(const gm_crypto_context_t *ctx, uint8_t key[crypto_shorthash_siphash24_KEYBYTES]) {
    if (!ctx) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "nullptr crypto context"));
    }
    
    char *input;
    size_t total_len;
    gm_result_void_t input_result = create_key_input(ctx->backend->name, &input, &total_len);
    if (GM_IS_ERR(input_result)) {
        return input_result;
    }
    
    uint8_t hash[GM_SHA256_DIGEST_SIZE];
    gm_result_void_t result = gm_sha256_with_context(ctx, input, total_len, hash);
    free(input);
    
    if (GM_IS_ERR(result)) {
        return result;
    }
    
    GM_MEMCPY_SAFE(key, crypto_shorthash_siphash24_KEYBYTES, hash, crypto_shorthash_siphash24_KEYBYTES);
    return gm_ok_void();
}

/* Compute SipHash and mix for better distribution */
static uint32_t compute_siphash(const uint8_t *siphash_key, gm_id_t new_identifier) {
    uint8_t hash_output[crypto_shorthash_siphash24_BYTES];
    
    crypto_shorthash_siphash24(hash_output, new_identifier.bytes, GM_ID_SIZE, siphash_key);
    
    uint64_t hash64;
    GM_MEMCPY_SAFE(&hash64, sizeof(hash64), hash_output, sizeof(hash64));
    
    return (uint32_t)(hash64 ^ (hash64 >> 32));
}

/* Hash function for hash tables using SipHash-2-4 with injected context */
gm_result_u32_t gm_id_hash_with_context(const gm_crypto_context_t *ctx, gm_id_t new_identifier) {
    if (!ctx) {
        return (gm_result_u32_t){.ok = false, 
                                .u.err = GM_ERROR(GM_ERR_INVALID_ARGUMENT, "nullptr crypto context")};
    }
    
    uint8_t siphash_key[crypto_shorthash_siphash24_KEYBYTES];
    gm_result_void_t key_result = derive_siphash_key(ctx, siphash_key);
    if (GM_IS_ERR(key_result)) {
        return (gm_result_u32_t){.ok = false, .u.err = GM_UNWRAP_ERR(key_result)};
    }

    uint32_t hash_value = compute_siphash(siphash_key, new_identifier);
    return (gm_result_u32_t){.ok = true, .u.val = hash_value};
}

/* Helper to create error result for ID */
static inline gm_result_id_t gm_err_id(gm_error_t *err) {
    return (gm_result_id_t){.ok = false, .u.err = err};
}

/* Create ID from data */
gm_result_id_t gm_id_from_data_with_context(const gm_crypto_context_t *ctx, const void *data, size_t len) {
    if (!ctx) {
        return gm_err_id(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "nullptr crypto context"));
    }
    if (!data) {
        return gm_err_id(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "nullptr data"));
    }

    gm_id_t new_id;
    gm_result_void_t result = gm_sha256_with_context(ctx, data, len, new_id.bytes);
    if (GM_IS_ERR(result)) {
        return gm_err_id(GM_UNWRAP_ERR(result));
    }

    return (gm_result_id_t){.ok = true, .u.val = new_id};
}

/* Create ID from string */
gm_result_id_t gm_id_from_string_with_context(const gm_crypto_context_t *ctx, const char *str) {
    if (!str) {
        return gm_err_id(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "nullptr string"));
    }
    return gm_id_from_data_with_context(ctx, str, strlen(str));
}

/* Generate random ID */
gm_result_id_t gm_id_generate_with_context(const gm_crypto_context_t *ctx) {
    if (!ctx) {
        return gm_err_id(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "nullptr crypto context"));
    }
    
    gm_id_t new_id;
    gm_result_void_t result = gm_random_bytes_with_context(ctx, new_id.bytes, GM_ID_SIZE);
    if (GM_IS_ERR(result)) {
        return gm_err_id(GM_UNWRAP_ERR(result));
    }

    return (gm_result_id_t){.ok = true, .u.val = new_id};
}

/* Hex conversion constants */
#define HEX_CHARS_PER_BYTE 2
#define HIGH_NIBBLE_SHIFT 4
#define LOW_NIBBLE_MASK 0x0F
#define HEX_DIGIT_STRING "0123456789abcdef"
#define HEX_FORMAT_2X "%2x"

/* Error messages */
static const char *const GM_ERR_MSG_INVALID_HEX_LEN = "Invalid hex ID: must be %d characters";
static const char *const GM_ERR_MSG_INVALID_HEX_CHAR = "Invalid hex character at position %d";

/* Convert ID to hex string (safe version with buffer size check) */
gm_result_void_t gm_id_to_hex(gm_id_t new_identifier, char *out, size_t out_size) {
    if (!out) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, "nullptr output buffer"));
    }

    if (out_size < GM_ID_HEX_SIZE) {
        return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL,
                                    "Buffer too small: need %zu bytes, got %zu",
                                    (size_t)GM_ID_HEX_SIZE, out_size));
    }

    static const char hex[] = HEX_DIGIT_STRING;

    for (int i = 0; i < GM_ID_SIZE; i++) {
        size_t hex_offset = (size_t)i * HEX_CHARS_PER_BYTE;
        out[hex_offset] = hex[new_identifier.bytes[i] >> HIGH_NIBBLE_SHIFT];
        out[hex_offset + 1] = hex[new_identifier.bytes[i] & LOW_NIBBLE_MASK];
    }
    out[GM_ID_HEX_CHARS] = '\0';

    return gm_ok_void();
}

/* Validate hex string length */
static gm_error_t *validate_hex_length(const char *hex) {
    if (!hex || strlen(hex) != GM_ID_HEX_CHARS) {
        return GM_ERROR(GM_ERR_INVALID_FORMAT, GM_ERR_MSG_INVALID_HEX_LEN, GM_ID_HEX_CHARS);
    }
    return nullptr;
}

/* Parse single hex byte */
static gm_error_t *parse_hex_byte(const char *hex, int pos, uint8_t *out) {
    char hex_pair[3];
    hex_pair[0] = hex[(size_t)pos * HEX_CHARS_PER_BYTE];
    hex_pair[1] = hex[((size_t)pos * HEX_CHARS_PER_BYTE) + 1];
    hex_pair[2] = '\0';
    
    char *endptr = nullptr;
    errno = 0;
    unsigned long value = strtoul(hex_pair, &endptr, 16);
    
    /* Check for conversion errors */
    if (errno != 0 || endptr != hex_pair + 2 || value > 0xFF) {
        return GM_ERROR(GM_ERR_INVALID_FORMAT, GM_ERR_MSG_INVALID_HEX_CHAR, 
                       pos * HEX_CHARS_PER_BYTE);
    }
    
    *out = (uint8_t)value;
    return nullptr;
}

/* Parse hex string to ID */
gm_result_id_t gm_id_from_hex(const char *hex) {
    gm_error_t *err = validate_hex_length(hex);
    if (err) {
        return (gm_result_id_t){.ok = false, .u.err = err};
    }

    gm_id_t new_id;
    GM_MEMSET_SAFE(&new_id, sizeof(new_id), 0, sizeof(new_id));

    for (int i = 0; i < GM_ID_SIZE; i++) {
        err = parse_hex_byte(hex, i, &new_id.bytes[i]);
        if (err) {
            return (gm_result_id_t){.ok = false, .u.err = err};
        }
    }

    return (gm_result_id_t){.ok = true, .u.val = new_id};
}

/* Helper to create error result for session ID */
static inline gm_result_session_id_t gm_err_session_id(gm_error_t *err) {
    return (gm_result_session_id_t){.ok = false, .u.err = err};
}

/* Generate session ID */
gm_result_session_id_t gm_session_id_new_with_context(const gm_crypto_context_t *ctx) {
    gm_result_id_t result = gm_id_generate_with_context(ctx);
    if (GM_IS_ERR(result)) {
        return gm_err_session_id(GM_UNWRAP_ERR(result));
    }

    gm_session_id_t sid;
    sid.base = GM_UNWRAP(result);
    return (gm_result_session_id_t){.ok = true, .u.val = sid};
}