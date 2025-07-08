/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/crypto/backend.h"

#include "gitmind/crypto/sha256.h"
#include "gitmind/error.h"

#include <sodium/crypto_hash_sha256.h>
#include <sodium/randombytes.h>
#include <stdint.h>
#include <string.h>
#include <stddef.h>

#define BITS_PER_BYTE 8
#define BYTE_MASK 0xFF
#define U32_HIGH_SHIFT 32

/* Forward declaration for default backend */
static const gm_crypto_backend_t GM_LIBSODIUM_BACKEND;

/* No global state - everything is context-based */

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
static const gm_crypto_backend_t GM_LIBSODIUM_BACKEND = {
    .name = "libsodium",
    .sha256_init = libsodium_sha256_init,
    .sha256_update = libsodium_sha256_update,
    .sha256_final = libsodium_sha256_final,
    .sha256 = libsodium_sha256,
    .random_bytes = libsodium_random_bytes,
    .random_u32 = libsodium_random_u32,
    .random_u64 = libsodium_random_u64,
    .context = nullptr};

/* Get libsodium backend */
const gm_crypto_backend_t *gm_crypto_backend_libsodium(void) {
    return &GM_LIBSODIUM_BACKEND;
}


/* Context-based crypto management (preferred) */
gm_result_crypto_context_t gm_crypto_context_create(const gm_crypto_backend_t *backend) {
    if (!backend) {
        return (gm_result_crypto_context_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERR_INVALID_ARGUMENT, "Backend cannot be null")};
    }
    
    /* Validate backend has all required functions */
    if (!backend->sha256_init || !backend->sha256_update ||
        !backend->sha256_final || !backend->sha256 || !backend->random_bytes ||
        !backend->random_u32 || !backend->random_u64) {
        return (gm_result_crypto_context_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERR_INVALID_ARGUMENT, "Backend missing required functions")};
    }
    
    gm_crypto_context_t ctx = { .backend = backend };
    return (gm_result_crypto_context_t){.ok = true, .u.val = ctx};
}

const gm_crypto_backend_t *gm_crypto_context_get_backend(const gm_crypto_context_t *ctx) {
    return ctx ? ctx->backend : NULL;
}



/* Legacy initialization removed - use gm_crypto_context_create() instead */
