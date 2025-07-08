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

gm_result_void_t gm_sha256_with_context(const gm_crypto_context_t *ctx, const void *data, size_t len,
                                        uint8_t out[GM_SHA256_DIGEST_SIZE]) {
    if (!ctx || !ctx->backend) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "Invalid crypto context"));
    }
    if (!data && len > 0) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "nullptr data with non-zero length"));
    }
    if (!out) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "nullptr output buffer"));
    }

    if (ctx->backend->sha256) {
        ctx->backend->sha256(data, len, out);
        return gm_ok_void();
    }
    
    return gm_err_void(GM_ERROR(GM_ERR_UNKNOWN, "Backend missing sha256 function"));
}

gm_result_void_t gm_sha256_init_with_context(const gm_crypto_context_t *ctx, gm_sha256_ctx_t *sha_ctx) {
    if (!ctx || !ctx->backend) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "Invalid crypto context"));
    }
    if (!sha_ctx) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "nullptr SHA256 context"));
    }

    if (ctx->backend->sha256_init) {
        ctx->backend->sha256_init(sha_ctx);
        return gm_ok_void();
    }
    
    return gm_err_void(GM_ERROR(GM_ERR_UNKNOWN, "Backend missing sha256_init function"));
}

gm_result_void_t gm_sha256_update_with_context(const gm_crypto_context_t *ctx, gm_sha256_ctx_t *sha_ctx,
                                               const void *data, size_t len) {
    if (!ctx || !ctx->backend) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "Invalid crypto context"));
    }
    if (!sha_ctx) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "nullptr SHA256 context"));
    }
    if (!data && len > 0) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "nullptr data with non-zero length"));
    }

    if (ctx->backend->sha256_update) {
        ctx->backend->sha256_update(sha_ctx, data, len);
        return gm_ok_void();
    }
    
    return gm_err_void(GM_ERROR(GM_ERR_UNKNOWN, "Backend missing sha256_update function"));
}

gm_result_void_t gm_sha256_final_with_context(const gm_crypto_context_t *ctx, gm_sha256_ctx_t *sha_ctx,
                                              uint8_t out[GM_SHA256_DIGEST_SIZE]) {
    if (!ctx || !ctx->backend) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "Invalid crypto context"));
    }
    if (!sha_ctx) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "nullptr SHA256 context"));
    }
    if (!out) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "nullptr output buffer"));
    }

    if (ctx->backend->sha256_final) {
        ctx->backend->sha256_final(sha_ctx, out);
        return gm_ok_void();
    }
    
    return gm_err_void(GM_ERROR(GM_ERR_UNKNOWN, "Backend missing sha256_final function"));
}
