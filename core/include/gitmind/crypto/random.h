/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_CRYPTO_RANDOM_H
#define GITMIND_CRYPTO_RANDOM_H

#include "gitmind/result.h"

#include <stddef.h>
#include <stdint.h>

/* Forward declaration */
typedef struct gm_crypto_context gm_crypto_context_t;

/**
 * @brief Fill buffer with cryptographically secure random bytes
 *
 * Uses the crypto context's backend to provide CSPRNG data
 * suitable for cryptographic keys, IDs, and nonces.
 *
 * @param ctx Crypto context with backend
 * @param buf Output buffer
 * @param size Number of random bytes to generate
 * @return gm_result_void Success or error
 */
gm_result_void_t gm_random_bytes_with_context(const gm_crypto_context_t *ctx, void *buf, size_t size);

/**
 * @brief Generate a random 32-bit unsigned integer
 *
 * @param ctx Crypto context with backend
 * @return gm_result_u32 containing random value or error
 */
gm_result_u32_t gm_random_u32_with_context(const gm_crypto_context_t *ctx);

/**
 * @brief Generate a random 64-bit unsigned integer
 *
 * @param ctx Crypto context with backend
 * @return gm_result_u64 containing random value or error
 */
gm_result_u64_t gm_random_u64_with_context(const gm_crypto_context_t *ctx);

#endif /* GITMIND_CRYPTO_RANDOM_H */