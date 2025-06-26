/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_CRYPTO_RANDOM_H
#define GITMIND_CRYPTO_RANDOM_H

#include "gitmind/result.h"

#include <stddef.h>
#include <stdint.h>

/* Result types for random operations (u32/u64) are now defined in result.h */

/**
 * @brief Fill buffer with cryptographically secure random bytes
 *
 * Uses libsodium's randombytes_buf() which provides CSPRNG data
 * suitable for cryptographic keys, IDs, and nonces.
 *
 * @param buf Output buffer
 * @param size Number of random bytes to generate
 * @return gm_result_void Success or error
 */
gm_result_void_t gm_random_bytes(void *buf, size_t size);

/**
 * @brief Generate a random 32-bit unsigned integer
 *
 * @return gm_result_u32 containing random value or error
 */
gm_result_u32_t gm_random_u32(void);

/**
 * @brief Generate a random 64-bit unsigned integer
 *
 * @return gm_result_u64 containing random value or error
 */
gm_result_u64_t gm_random_u64(void);

#endif /* GITMIND_CRYPTO_RANDOM_H */