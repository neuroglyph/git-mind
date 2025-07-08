/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/crypto/random.h"

#include "gitmind/crypto/backend.h"
#include "gitmind/error.h"
#include "gitmind/result.h"

#include <stddef.h>
#include <stdint.h>

gm_result_void_t gm_random_bytes_with_context(const gm_crypto_context_t *ctx, void *buf, size_t size) {
    if (!ctx || !ctx->backend) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "Invalid crypto context"));
    }
    if (!buf) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "nullptr buffer"));
    }
    if (size == 0) {
        return gm_ok_void();
    }

    if (ctx->backend->random_bytes) {
        ctx->backend->random_bytes(buf, size);
        return gm_ok_void();
    }
    
    return gm_err_void(GM_ERROR(GM_ERR_UNKNOWN, "Backend missing random_bytes function"));
}

gm_result_u32_t gm_random_u32_with_context(const gm_crypto_context_t *ctx) {
    if (!ctx || !ctx->backend) {
        return (gm_result_u32_t){.ok = false, 
                                .u.err = GM_ERROR(GM_ERR_INVALID_ARGUMENT, "Invalid crypto context")};
    }

    if (ctx->backend->random_u32) {
        return (gm_result_u32_t){.ok = true, .u.val = ctx->backend->random_u32()};
    }
    
    return (gm_result_u32_t){.ok = false, 
                            .u.err = GM_ERROR(GM_ERR_UNKNOWN, "Backend missing random_u32 function")};
}

gm_result_u64_t gm_random_u64_with_context(const gm_crypto_context_t *ctx) {
    if (!ctx || !ctx->backend) {
        return (gm_result_u64_t){.ok = false, 
                                .u.err = GM_ERROR(GM_ERR_INVALID_ARGUMENT, "Invalid crypto context")};
    }

    if (ctx->backend->random_u64) {
        return (gm_result_u64_t){.ok = true, .u.val = ctx->backend->random_u64()};
    }
    
    return (gm_result_u64_t){.ok = false, 
                            .u.err = GM_ERROR(GM_ERR_UNKNOWN, "Backend missing random_u64 function")};
}