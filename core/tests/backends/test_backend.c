/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

/*
 * Test-only crypto backend with deterministic behavior
 * This file is NEVER linked into production builds
 */

#include "gitmind/crypto/backend.h"
#include "gitmind/crypto/sha256.h"
#include "gitmind/security/memory.h"

#include <stddef.h>
#include <stdint.h>

/* Test backend constants */
#define TEST_HASH_LENGTH_HEADER_SIZE 4
#define TEST_HASH_MAX_DATA_BYTES (GM_SHA256_DIGEST_SIZE - TEST_HASH_LENGTH_HEADER_SIZE)
#define BITS_PER_BYTE 8
#define BYTE_MASK 0xFF
#define U32_HIGH_SHIFT 32

/* Test-only mutable state for deterministic behavior */
static uint32_t test_counter = 0;

/* Test SHA-256 implementation (deterministic) */
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
    uint64_t *total = &ctx->u.align[0];
    *total += len;
    return 0;
}

static int test_sha256_final(gm_sha256_ctx_t *ctx,
                             uint8_t out[GM_SHA256_DIGEST_SIZE]) {
    /* Output based on total length */
    uint64_t *total = &ctx->u.align[0];
    GM_MEMSET_SAFE(out, GM_SHA256_DIGEST_SIZE, 0, GM_SHA256_DIGEST_SIZE);
    GM_MEMCPY_SAFE(out, GM_SHA256_DIGEST_SIZE, total, sizeof(*total));
    return 0;
}

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
static const gm_crypto_backend_t GM_TEST_BACKEND = {
    .name = "test",
    .sha256_init = test_sha256_init,
    .sha256_update = test_sha256_update,
    .sha256_final = test_sha256_final,
    .sha256 = test_sha256,
    .random_bytes = test_random_bytes,
    .random_u32 = test_random_u32,
    .random_u64 = test_random_u64,
    .context = &test_counter
};

/* Get test backend - only available when GITMIND_ENABLE_TEST_BACKEND is defined */
const gm_crypto_backend_t *gm_crypto_backend_test(void) {
    /* Reset counter for reproducibility */
    test_counter = 0;
    return &GM_TEST_BACKEND;
}
