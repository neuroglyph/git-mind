/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/crypto/backend.h"

#include "gitmind/crypto/sha256.h"
#include "gitmind/error.h"
#include "gitmind/result.h"

#include <sodium/crypto_hash_sha256.h>
#include <sodium/randombytes.h>
#include <sodium/core.h>
#include <stdatomic.h>
#include <stdint.h>
#include <string.h>

#define BITS_PER_BYTE 8
#define BYTE_MASK 0xFF
#define U32_HIGH_SHIFT 32

/* Forward declaration for default backend */
static const gm_crypto_backend_t GM_LIBSODIUM_BACKEND;

/* Global backend instance - uses atomic operations for thread-safe switching */
static _Atomic(const gm_crypto_backend_t *) g_backend = &GM_LIBSODIUM_BACKEND;

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
        return gm_err_crypto_context(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, "Backend cannot be null"));
    }
    
    /* Validate backend has all required functions */
    if (!backend->sha256_init || !backend->sha256_update ||
        !backend->sha256_final || !backend->sha256 || !backend->random_bytes ||
        !backend->random_u32 || !backend->random_u64) {
        return gm_err_crypto_context(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, "Backend missing required functions"));
    }
    
    gm_crypto_context_t ctx = { .backend = backend };
    return gm_ok_crypto_context(ctx);
}

const gm_crypto_backend_t *gm_crypto_context_get_backend(const gm_crypto_context_t *ctx) {
    return ctx ? ctx->backend : nullptr;
}

/* Legacy global backend management (deprecated) */
gm_result_backend_t gm_crypto_set_backend(const gm_crypto_backend_t *backend) {
    if (!backend) {
        return (gm_result_backend_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERR_INVALID_ARGUMENT, "nullptr backend")};
    }

    /* Validate backend has all required functions */
    if (!backend->sha256_init || !backend->sha256_update ||
        !backend->sha256_final || !backend->sha256 || !backend->random_bytes ||
        !backend->random_u32 || !backend->random_u64) {
        return (gm_result_backend_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                              "Backend missing required functions")};
    }

    atomic_store(&g_backend, backend);
    return (gm_result_backend_t){.ok = true, .u.val = backend};
}

/* Get current backend */
const gm_crypto_backend_t *gm_crypto_get_backend(void) {
    return atomic_load(&g_backend);
}

/* Initialize crypto subsystem */
gm_result_void_t gm_crypto_init(void) {
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
    if (!atomic_load(&g_backend)) {
        gm_result_backend_t result = gm_crypto_set_backend(&GM_LIBSODIUM_BACKEND);
        if (GM_IS_ERR(result)) {
            return gm_err_void(GM_UNWRAP_ERR(result));
        }
    }

    return gm_ok_void();
}

/* Cleanup crypto subsystem */
gm_result_void_t gm_crypto_cleanup(void) {
    atomic_store(&g_backend, nullptr);
    return gm_ok_void();
}