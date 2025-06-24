/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_CRYPTO_SHA256_H
#define GITMIND_CRYPTO_SHA256_H

#include "gitmind/result.h"

#include <stddef.h>
#include <stdint.h>

/* SHA256 digest size */
#define GM_SHA256_DIGEST_SIZE 32
#define GM_SHA256_BLOCK_SIZE 64

/* SHA256 context for streaming operations */
typedef struct gm_sha256_ctx {
    /* Implementation-specific; users should treat as opaque */
    uint8_t opaque[256]; /* Enough for any backend */
} gm_sha256_ctx_t;

/**
 * @brief Compute SHA256 hash of data in one shot
 *
 * @param data Input data
 * @param len Length of input data
 * @param out Output buffer for 32-byte digest (big-endian/network order)
 * @return gm_result_void Success or error
 */
gm_result_void gm_sha256(const void *data, size_t len,
                         uint8_t out[GM_SHA256_DIGEST_SIZE]);

/**
 * @brief Initialize SHA256 streaming context
 *
 * @param ctx Context to initialize
 * @return gm_result_void Success or error
 */
gm_result_void gm_sha256_init(gm_sha256_ctx_t *ctx);

/**
 * @brief Update SHA256 context with more data
 *
 * @param ctx Context to update
 * @param data Input data chunk
 * @param len Length of input data
 * @return gm_result_void Success or error
 */
gm_result_void gm_sha256_update(gm_sha256_ctx_t *ctx, const void *data,
                                size_t len);

/**
 * @brief Finalize SHA256 computation and get digest
 *
 * @param ctx Context to finalize (becomes invalid after this call)
 * @param out Output buffer for 32-byte digest (big-endian/network order)
 * @return gm_result_void Success or error
 */
gm_result_void gm_sha256_final(gm_sha256_ctx_t *ctx,
                               uint8_t out[GM_SHA256_DIGEST_SIZE]);

#endif /* GITMIND_CRYPTO_SHA256_H */