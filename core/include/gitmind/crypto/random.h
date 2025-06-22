/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_CRYPTO_RANDOM_H
#define GITMIND_CRYPTO_RANDOM_H

#include <stdint.h>
#include <stddef.h>
#include "gitmind/result.h"

/* Define result types for random operations */
GM_RESULT_DEF(gm_result_u32, uint32_t);
GM_RESULT_DEF(gm_result_u64, uint64_t);

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
gm_result_void gm_random_bytes(void* buf, size_t size);

/**
 * @brief Generate a random 32-bit unsigned integer
 * 
 * @return gm_result_u32 containing random value or error
 */
gm_result_u32 gm_random_u32(void);

/**
 * @brief Generate a random 64-bit unsigned integer
 * 
 * @return gm_result_u64 containing random value or error
 */
gm_result_u64 gm_random_u64(void);

#endif /* GITMIND_CRYPTO_RANDOM_H */