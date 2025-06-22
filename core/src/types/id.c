/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/types/id.h"
#include "gitmind/error.h"
#include "gitmind/crypto/sha256.h"
#include "gitmind/crypto/random.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <sodium.h>

/* Compare two IDs */
bool gm_id_equal(gm_id_t a, gm_id_t b) {
    return memcmp(a.bytes, b.bytes, GM_ID_SIZE) == 0;
}

/* Compare IDs for ordering */
int gm_id_compare(gm_id_t a, gm_id_t b) {
    return memcmp(a.bytes, b.bytes, GM_ID_SIZE);
}

/* SipHash key - initialized once at startup */
static uint8_t g_siphash_key[crypto_shorthash_siphash24_KEYBYTES];
static bool g_siphash_key_initialized = false;

/* Initialize SipHash key with random data */
static void ensure_siphash_key_initialized(void) {
    if (!g_siphash_key_initialized) {
        /* Generate random key - this is best effort, if it fails we use zeros */
        gm_result_void result = gm_random_bytes(g_siphash_key, sizeof(g_siphash_key));
        if (GM_IS_ERR(result)) {
            /* If random generation fails, use a deterministic fallback */
            /* This is still better than hardcoded constant */
            for (size_t i = 0; i < sizeof(g_siphash_key); i++) {
                g_siphash_key[i] = (uint8_t)(i ^ 0xAA);
            }
        }
        g_siphash_key_initialized = true;
    }
}

/* Hash function for hash tables using SipHash-2-4 */
uint32_t gm_id_hash(gm_id_t id) {
    /* Ensure key is initialized */
    ensure_siphash_key_initialized();
    
    /* SipHash produces 8-byte output */
    uint8_t hash_output[crypto_shorthash_siphash24_BYTES];
    
    /* Compute SipHash-2-4 of the ID bytes */
    crypto_shorthash_siphash24(
        hash_output,           /* output buffer */
        id.bytes,             /* input data */
        GM_ID_SIZE,           /* input length */
        g_siphash_key         /* 128-bit key */
    );
    
    /* Convert 8-byte output to uint64_t */
    uint64_t hash64;
    memcpy(&hash64, hash_output, sizeof(hash64));
    
    /* Mix upper and lower halves for better distribution */
    return (uint32_t)(hash64 ^ (hash64 >> 32));
}

/* Helper to create error result for ID */
static inline gm_result_id gm_err_id(gm_error_t* e) {
    return (gm_result_id){ .ok = false, .u.err = e };
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

/* Convert ID to hex string */
void gm_id_to_hex(gm_id_t id, char out[GM_ID_HEX_SIZE]) {
    static const char hex[] = "0123456789abcdef";
    
    for (int i = 0; i < GM_ID_SIZE; i++) {
        out[i * 2]     = hex[id.bytes[i] >> 4];
        out[i * 2 + 1] = hex[id.bytes[i] & 0x0F];
    }
    out[GM_ID_HEX_CHARS] = '\0';
}

/* Parse hex string to ID */
gm_result_id gm_id_from_hex(const char* hex) {
    gm_id_t id;
    memset(&id, 0, sizeof(id));
    
    if (!hex || strlen(hex) != GM_ID_HEX_CHARS) {
        return (gm_result_id){
            .ok = false,
            .u.err = GM_ERROR(GM_ERR_INVALID_FORMAT, 
                             "Invalid hex ID: must be %d characters", GM_ID_HEX_CHARS)
        };
    }
    
    for (int i = 0; i < GM_ID_SIZE; i++) {
        unsigned int byte;
        if (sscanf(hex + i * 2, "%2x", &byte) != 1) {
            return (gm_result_id){
                .ok = false,
                .u.err = GM_ERROR(GM_ERR_INVALID_FORMAT,
                                 "Invalid hex character at position %d", i * 2)
            };
        }
        id.bytes[i] = (uint8_t)byte;
    }
    
    return (gm_result_id){ .ok = true, .u.val = id };
}

/* Helper to create error result for session ID */
static inline gm_result_session_id gm_err_session_id(gm_error_t* e) {
    return (gm_result_session_id){ .ok = false, .u.err = e };
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