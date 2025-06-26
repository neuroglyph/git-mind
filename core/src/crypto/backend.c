/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/crypto/backend.h"

#include "gitmind/crypto/sha256.h"
#include "gitmind/error.h"
#include "gitmind/security/memory.h"

#include <sodium/crypto_hash_sha256.h>
#include <sodium/randombytes.h>
#include <sodium/core.h>
#include <stdint.h>
#include <string.h>

/* Test backend constants */
#define TEST_HASH_LENGTH_HEADER_SIZE                                           \
    4 /* Bytes used to store length in test hash */
#define TEST_HASH_MAX_DATA_BYTES                                               \
    (GM_SHA256_DIGEST_SIZE - TEST_HASH_LENGTH_HEADER_SIZE)
#define BITS_PER_BYTE 8
#define BYTE_MASK 0xFF
#define U32_HIGH_SHIFT 32

/* Global backend instance */
static gm_crypto_backend_t *g_backend = NULL;

/* Libsodium backend implementation */
static int libsodium_sha256_init(gm_sha256_ctx_t *ctx) {
    crypto_hash_sha256_init((crypto_hash_sha256_state *)ctx);
    return 0;
}

static int libsodium_sha256_update(gm_sha256_ctx_t *ctx, const void *data,
                                   size_t len) {
    crypto_hash_sha256_update((crypto_hash_sha256_state *)ctx, data, len);
    return 0;
}

static int libsodium_sha256_final(gm_sha256_ctx_t *ctx,
                                  uint8_t out[GM_SHA256_DIGEST_SIZE]) {
    crypto_hash_sha256_final((crypto_hash_sha256_state *)ctx, out);
    return 0;
}

static int libsodium_sha256(const void *data, size_t len,
                            uint8_t out[GM_SHA256_DIGEST_SIZE]) {
    crypto_hash_sha256(out, data, len);
    return 0;
}

static int libsodium_random_bytes(void *buf, size_t size) {
    randombytes_buf(buf, size);
    return 0;
}

static uint32_t libsodium_random_u32(void) {
    return randombytes_random();
}

static uint64_t libsodium_random_u64(void) {
    return ((uint64_t)randombytes_random() << U32_HIGH_SHIFT) |
           randombytes_random();
}

/* Libsodium backend instance */
static gm_crypto_backend_t libsodium_backend = {
    .name = "libsodium",
    .sha256_init = libsodium_sha256_init,
    .sha256_update = libsodium_sha256_update,
    .sha256_final = libsodium_sha256_final,
    .sha256 = libsodium_sha256,
    .random_bytes = libsodium_random_bytes,
    .random_u32 = libsodium_random_u32,
    .random_u64 = libsodium_random_u64,
    .context = NULL};

/* Test backend implementation (deterministic) */
static int test_sha256(const void *data, size_t len,
                       uint8_t out[GM_SHA256_DIGEST_SIZE]) {
    /* Simple deterministic "hash" for testing */
    GM_MEMSET_SAFE(out, GM_SHA256_DIGEST_SIZE, 0, GM_SHA256_DIGEST_SIZE);

    /* Mix in length */
    out[0] = (uint8_t)(len & BYTE_MASK);
    out[1] = (uint8_t)((len >> BITS_PER_BYTE) & BYTE_MASK);
    out[2] = (uint8_t)((len >> (2 * BITS_PER_BYTE)) & BYTE_MASK);
    out[3] = (uint8_t)((len >> (3 * BITS_PER_BYTE)) & BYTE_MASK);

    /* Mix in first few bytes of data */
    if (data && len > 0) {
        const uint8_t *bytes = (const uint8_t *)data;
        size_t to_copy =
            len < TEST_HASH_MAX_DATA_BYTES ? len : TEST_HASH_MAX_DATA_BYTES;
        GM_MEMCPY_SAFE(out + TEST_HASH_LENGTH_HEADER_SIZE,
                       GM_SHA256_DIGEST_SIZE - TEST_HASH_LENGTH_HEADER_SIZE,
                       bytes, to_copy);
    }

    return 0;
}

static int test_sha256_init(gm_sha256_ctx_t *ctx) {
    /* Clear context */
    GM_MEMSET_SAFE(ctx, sizeof(*ctx), 0, sizeof(*ctx));
    return 0;
}

static int test_sha256_update(gm_sha256_ctx_t *ctx, const void *data,
                              size_t len) {
    /* For test backend, just track total length */
    (void)data; /* Unused in test implementation */
    uint64_t *total = (uint64_t *)ctx;
    *total += len;
    return 0;
}

static int test_sha256_final(gm_sha256_ctx_t *ctx,
                             uint8_t out[GM_SHA256_DIGEST_SIZE]) {
    /* Output based on total length */
    uint64_t *total = (uint64_t *)ctx;
    GM_MEMSET_SAFE(out, GM_SHA256_DIGEST_SIZE, 0, GM_SHA256_DIGEST_SIZE);
    GM_MEMCPY_SAFE(out, GM_SHA256_DIGEST_SIZE, total, sizeof(*total));
    return 0;
}

static uint32_t test_counter = 0;

static int test_random_bytes(void *buf, size_t size) {
    /* Fill with incrementing pattern */
    uint8_t *bytes = (uint8_t *)buf;
    for (size_t i = 0; i < size; i++) {
        bytes[i] = (uint8_t)(test_counter++ & BYTE_MASK);
    }
    return 0;
}

static uint32_t test_random_u32(void) {
    return test_counter++;
}

static uint64_t test_random_u64(void) {
    uint64_t result = test_counter;
    test_counter += 2;
    return result;
}

/* Test backend instance */
static gm_crypto_backend_t test_backend = {.name = "test",
                                           .sha256_init = test_sha256_init,
                                           .sha256_update = test_sha256_update,
                                           .sha256_final = test_sha256_final,
                                           .sha256 = test_sha256,
                                           .random_bytes = test_random_bytes,
                                           .random_u32 = test_random_u32,
                                           .random_u64 = test_random_u64,
                                           .context = &test_counter};

/* Get libsodium backend */
gm_crypto_backend_t *gm_crypto_backend_libsodium(void) {
    return &libsodium_backend;
}

/* Get test backend */
gm_crypto_backend_t *gm_crypto_backend_test(void) {
    /* Reset counter for reproducibility */
    test_counter = 0;
    return &test_backend;
}

/* Set current backend */
gm_result_backend gm_crypto_set_backend(gm_crypto_backend_t *backend) {
    if (!backend) {
        return (gm_result_backend){
            .ok = false,
            .u.err = GM_ERROR(GM_ERR_INVALID_ARGUMENT, "NULL backend")};
    }

    /* Validate backend has all required functions */
    if (!backend->sha256_init || !backend->sha256_update ||
        !backend->sha256_final || !backend->sha256 || !backend->random_bytes ||
        !backend->random_u32 || !backend->random_u64) {
        return (gm_result_backend){
            .ok = false,
            .u.err = GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                              "Backend missing required functions")};
    }

    g_backend = backend;
    return (gm_result_backend){.ok = true, .u.val = backend};
}

/* Get current backend */
gm_crypto_backend_t *gm_crypto_get_backend(void) {
    return g_backend;
}

/* Initialize crypto subsystem */
gm_result_void gm_crypto_init(void) {
    /* Initialize libsodium if not already done */
    static bool sodium_initialized = false;
    if (!sodium_initialized) {
        if (sodium_init() < 0) {
            return gm_err_void(
                GM_ERROR(GM_ERR_UNKNOWN, "Failed to initialize libsodium"));
        }
        sodium_initialized = true;
    }

    /* Set default backend if none set */
    if (!g_backend) {
        gm_result_backend result = gm_crypto_set_backend(&libsodium_backend);
        if (GM_IS_ERR(result)) {
            return gm_err_void(GM_UNWRAP_ERR(result));
        }
    }

    return gm_ok_void();
}

/* Cleanup crypto subsystem */
gm_result_void gm_crypto_cleanup(void) {
    g_backend = NULL;
    return gm_ok_void();
}