/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_CRYPTO_BACKEND_H
#define GITMIND_CRYPTO_BACKEND_H

#include "gitmind/result.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* SHA-256 constants */
#define GM_SHA256_DIGEST_SIZE 32
#define GM_SHA256_BLOCK_SIZE 64

/* Forward declaration */
typedef struct gm_sha256_ctx gm_sha256_ctx_t;

/**
 * @brief Crypto backend interface for dependency injection
 *
 * This allows tests to inject deterministic implementations
 * while production code uses real crypto libraries.
 */
typedef struct gm_crypto_backend {
    /* Backend name for identification */
    const char *name;

    /* SHA-256 functions */
    int (*sha256_init)(gm_sha256_ctx_t *ctx);
    int (*sha256_update)(gm_sha256_ctx_t *ctx, const void *data, size_t len);
    int (*sha256_final)(gm_sha256_ctx_t *ctx,
                        uint8_t out[GM_SHA256_DIGEST_SIZE]);
    int (*sha256)(const void *data, size_t len,
                  uint8_t out[GM_SHA256_DIGEST_SIZE]);

    /* Random number generation */
    int (*random_bytes)(void *buf, size_t size);
    uint32_t (*random_u32)(void);
    uint64_t (*random_u64)(void);

    /* Optional: backend-specific context */
    void *context;
} gm_crypto_backend_t;

/* Result type for backend operations */
GM_RESULT_DEF(gm_result_backend, gm_crypto_backend_t *);

/* Global backend management */
gm_result_backend gm_crypto_set_backend(gm_crypto_backend_t *backend);
gm_crypto_backend_t *gm_crypto_get_backend(void);

/* Default backends */
gm_crypto_backend_t *gm_crypto_backend_libsodium(void);
gm_crypto_backend_t *gm_crypto_backend_test(void);

/* Backend initialization */
gm_result_void gm_crypto_init(void);
gm_result_void gm_crypto_cleanup(void);

#endif /* GITMIND_CRYPTO_BACKEND_H */