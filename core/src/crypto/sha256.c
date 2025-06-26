/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/crypto/sha256.h"

#include "gitmind/crypto/backend.h"
#include "gitmind/error.h"
#include "gitmind/result.h"

#include <assert.h>
#include <sodium/crypto_hash_sha256.h>
#include <stdint.h>
#include <string.h>

/* Ensure our context size is sufficient */
_Static_assert(sizeof(crypto_hash_sha256_state) <= sizeof(gm_sha256_ctx_t),
               "gm_sha256_ctx_t too small for libsodium state");

gm_result_void gm_sha256(const void *data, size_t len,
                         uint8_t out[GM_SHA256_DIGEST_SIZE]) {
    if (!data && len > 0) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "NULL data with non-zero length"));
    }
    if (!out) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, "NULL output buffer"));
    }

    const gm_crypto_backend_t *backend = gm_crypto_get_backend();

    /* Use backend if available */
    if (backend && backend->sha256) {
        backend->sha256(data, len, out);
        return gm_ok_void();
    }

    /* Fallback to direct libsodium */
    crypto_hash_sha256(out, data, len);
    return gm_ok_void();
}

gm_result_void gm_sha256_init(gm_sha256_ctx_t *ctx) {
    if (!ctx) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "NULL context"));
    }

    const gm_crypto_backend_t *backend = gm_crypto_get_backend();

    /* Use backend if available */
    if (backend && backend->sha256_init) {
        backend->sha256_init(ctx);
        return gm_ok_void();
    }

    /* Fallback to direct libsodium */
    crypto_hash_sha256_state *state = (crypto_hash_sha256_state *)ctx;
    crypto_hash_sha256_init(state);
    return gm_ok_void();
}

gm_result_void gm_sha256_update(gm_sha256_ctx_t *ctx, const void *data,
                                size_t len) {
    if (!ctx) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "NULL context"));
    }
    if (!data && len > 0) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "NULL data with non-zero length"));
    }

    const gm_crypto_backend_t *backend = gm_crypto_get_backend();

    /* Use backend if available */
    if (backend && backend->sha256_update) {
        backend->sha256_update(ctx, data, len);
        return gm_ok_void();
    }

    /* Fallback to direct libsodium */
    crypto_hash_sha256_state *state = (crypto_hash_sha256_state *)ctx;
    crypto_hash_sha256_update(state, data, len);
    return gm_ok_void();
}

gm_result_void gm_sha256_final(gm_sha256_ctx_t *ctx,
                               uint8_t out[GM_SHA256_DIGEST_SIZE]) {
    if (!ctx) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "NULL context"));
    }
    if (!out) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, "NULL output buffer"));
    }

    const gm_crypto_backend_t *backend = gm_crypto_get_backend();

    /* Use backend if available */
    if (backend && backend->sha256_final) {
        backend->sha256_final(ctx, out);
        return gm_ok_void();
    }

    /* Fallback to direct libsodium */
    crypto_hash_sha256_state *state = (crypto_hash_sha256_state *)ctx;
    crypto_hash_sha256_final(state, out);
    return gm_ok_void();
}