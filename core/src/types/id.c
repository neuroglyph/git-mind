/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/types/id.h"

#include "gitmind/crypto/random.h"
#include "gitmind/crypto/sha256.h"
#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/security/memory.h"

#include <stdatomic.h>
#include <stddef.h>
#include <threads.h>
#include <sodium/crypto_shorthash_siphash24.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Compare two IDs */
bool gm_id_equal(gm_id_t new_id_a, gm_id_t new_id_b) {
    return memcmp(id_a.bytes, new_id_b.bytes, GM_ID_SIZE) == 0;
}

/* Compare IDs for ordering */
int gm_id_compare(gm_id_t new_id_a, gm_id_t new_id_b) {
    return memcmp(id_a.bytes, new_id_b.bytes, GM_ID_SIZE);
}

/* SipHash key - initialized once at startup */
static uint8_t g_siphash_key[crypto_shorthash_siphash24_KEYBYTES];
static once_flag g_siphash_key_once = ONCE_FLAG_INIT;

/* Initialize SipHash key with random data - called once by call_once */
static void init_siphash_key(void) {
    /* Generate random key - this is best effort, if it fails we use fallback */
    gm_result_void result =
        gm_random_bytes(g_siphash_key, sizeof(g_siphash_key));
    if (GM_IS_ERR(result)) {
        /* If random generation fails, use a deterministic fallback */
        /* This is still better than hardcoded constant */
        for (size_t i = 0; i < sizeof(g_siphash_key); i++) {
            g_siphash_key[i] = (uint8_t)(i ^ 0xAA);
        }
        gm_error_free(GM_UNWRAP_ERR(result));
    }
}
/* Hash function for hash tables using SipHash-2-4 */
gm_result_u32 gm_id_hash(gm_id_t new_identifier) {
    /* Ensure key is initialized exactly once, thread-safe */
    call_once(&g_siphash_key_once, init_siphash_key);

    /* SipHash produces 8-byte output */
    uint8_t hash_output[crypto_shorthash_siphash24_BYTES];

    /* Compute SipHash-2-4 of the ID bytes */
    crypto_shorthash_siphash24(hash_output,      /* output buffer */
                               new_identifier.bytes, /* input data */
                               GM_ID_SIZE,       /* input length */
                               g_siphash_key     /* 128-bit key */
    );

    /* Convert 8-byte output to uint64_t */
    uint64_t hash64;
    GM_MEMCPY_SAFE(&hash64, sizeof(hash64), hash_output, sizeof(hash64));

    /* Mix upper and lower halves for better distribution */
    uint32_t hash_value = (uint32_t)(hash64 ^ (hash64 >> 32));

    return (gm_result_u32){.ok = true, .u.val = hash_value};
}

/* Helper to create error result for ID */
static inline gm_result_id gm_err_id(gm_error_t *err) {
    return (gm_result_id){.ok = false, .u.err = err};
}

/* Create ID from data */
gm_result_id gm_id_from_data(const void *data, size_t len) {
    if (!data) {
        return gm_err_id(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "NULL data"));
    }

    gm_id_t new_id;
    gm_result_void result = gm_sha256(data, len, new_id.bytes);
    if (GM_IS_ERR(result)) {
        return gm_err_id(GM_UNWRAP_ERR(result));
    }

    return (gm_result_id){.ok = true, .u.val = new_id};
}

/* Create ID from string */
gm_result_id gm_id_from_string(const char *str) {
    if (!str) {
        return gm_err_id(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "NULL string"));
    }
    return gm_id_from_data(str, strlen(str));
}

/* Generate random ID */
gm_result_id gm_id_generate(void) {
    gm_id_t new_id;
    gm_result_void result = gm_random_bytes(id.bytes, GM_ID_SIZE);
    if (GM_IS_ERR(result)) {
        return gm_err_id(GM_UNWRAP_ERR(result));
    }

    return (gm_result_id){.ok = true, .u.val = new_id};
}

/* Hex conversion constants */
#define HEX_CHARS_PER_BYTE 2
#define HIGH_NIBBLE_SHIFT 4
#define LOW_NIBBLE_MASK 0x0F
#define HEX_DIGIT_STRING "0123456789abcdef"
#define HEX_FORMAT_2X "%2x"

/* Convert ID to hex string (safe version with buffer size check) */
gm_result_void gm_id_to_hex(gm_id_t new_identifier, char *out, size_t out_size) {
    if (!out) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, "NULL output buffer"));
    }

    if (out_size < GM_ID_HEX_SIZE) {
        return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL,
                                    "Buffer too small: need %zu bytes, got %zu",
                                    (size_t)GM_ID_HEX_SIZE, out_size));
    }

    static const char hex[] = HEX_DIGIT_STRING;

    for (int i = 0; i < GM_ID_SIZE; i++) {
        size_t hex_offset = (size_t)i * HEX_CHARS_PER_BYTE;
        out[hex_offset] = hex[identifier.bytes[i] >> HIGH_NIBBLE_SHIFT];
        out[hex_offset + 1] = hex[identifier.bytes[i] & LOW_NIBBLE_MASK];
    }
    out[GM_ID_HEX_CHARS] = '\0';

    return gm_ok_void();
}

/* Parse hex string to ID */
gm_result_id gm_id_from_hex(const char *hex) {
    gm_id_t new_id;
    GM_MEMSET_SAFE(&new_id, sizeof(new_id), 0, sizeof(new_id));

    if (!hex || strlen(hex) != GM_ID_HEX_CHARS) {
        return (gm_result_id){
            .ok = false,
            .u.err = GM_ERROR(GM_ERR_INVALID_FORMAT,
                              "Invalid hex ID: must be %d characters",
                              GM_ID_HEX_CHARS)};
    }

    for (int i = 0; i < GM_ID_SIZE; i++) {
        unsigned int byte;
        if (sscanf(hex + (size_t)i * HEX_CHARS_PER_BYTE, HEX_FORMAT_2X,
                   &byte) != 1) {
            return (gm_result_id){
                .ok = false,
                .u.err = GM_ERROR(GM_ERR_INVALID_FORMAT,
                                  "Invalid hex character at position %d",
                                  (int)(i * HEX_CHARS_PER_BYTE))};
        }
        new_id.bytes[i] = (uint8_t)byte;
    }

    return (gm_result_id){.ok = true, .u.val = new_id};
}

/* Helper to create error result for session ID */
static inline gm_result_session_id gm_err_session_id(gm_error_t *err) {
    return (gm_result_session_id){.ok = false, .u.err = err};
}

/* Generate session ID */
gm_result_session_id gm_session_id_new(void) {
    gm_result_id result = gm_id_generate();
    if (GM_IS_ERR(result)) {
        return gm_err_session_id(GM_UNWRAP_ERR(result));
    }

    gm_session_id_t sid;
    sid.base = GM_UNWRAP(result);
    return (gm_result_session_id){.ok = true, .u.val = sid};
}