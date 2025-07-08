/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/crypto/backend.h"
#include "gitmind/crypto/random.h"
#include "gitmind/crypto/sha256.h"
#include "gitmind/error.h"
#include "gitmind/result.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Test backend switching */
static void test_backend_switch(void) {
    /* Create libsodium context */
    gm_result_crypto_context_t ctx1_result = gm_crypto_context_create(gm_crypto_backend_libsodium());
    assert(GM_IS_OK(ctx1_result));
    gm_crypto_context_t ctx1 = GM_UNWRAP(ctx1_result);

    /* Verify backend name */
    const gm_crypto_backend_t *backend = gm_crypto_context_get_backend(&ctx1);
    assert(backend != nullptr);
    assert(strcmp(backend->name, "libsodium") == 0);

    /* Test with libsodium backend */
    uint8_t hash1[GM_SHA256_DIGEST_SIZE];
    gm_result_void_t sha_result = gm_sha256_with_context(&ctx1, "test", 4, hash1);
    assert(GM_IS_OK(sha_result));

    /* Create test backend context */
    gm_result_crypto_context_t ctx2_result = gm_crypto_context_create(gm_crypto_backend_test());
    assert(GM_IS_OK(ctx2_result));
    gm_crypto_context_t ctx2 = GM_UNWRAP(ctx2_result);

    backend = gm_crypto_context_get_backend(&ctx2);
    assert(strcmp(backend->name, "test") == 0);

    /* Test with test backend (should be deterministic) */
    uint8_t hash2[GM_SHA256_DIGEST_SIZE];
    sha_result = gm_sha256_with_context(&ctx2, "test", 4, hash2);
    assert(GM_IS_OK(sha_result));

    /* Test backend should produce predictable output */
    assert(hash2[0] == 4); /* Length & 0xFF */
    assert(hash2[1] == 0); /* (Length >> 8) & 0xFF */
    assert(hash2[2] == 0); /* (Length >> 16) & 0xFF */
    assert(hash2[3] == 0); /* (Length >> 24) & 0xFF */
    assert(memcmp(hash2 + 4, "test", 4) == 0);

    /* Hashes should be different */
    assert(memcmp(hash1, hash2, GM_SHA256_DIGEST_SIZE) != 0);

    /* Test libsodium again with new context */
    gm_result_crypto_context_t ctx3_result = gm_crypto_context_create(gm_crypto_backend_libsodium());
    assert(GM_IS_OK(ctx3_result));
    gm_crypto_context_t ctx3 = GM_UNWRAP(ctx3_result);

    uint8_t hash3[GM_SHA256_DIGEST_SIZE];
    sha_result = gm_sha256_with_context(&ctx3, "test", 4, hash3);
    assert(GM_IS_OK(sha_result));

    /* Should match first hash */
    assert(memcmp(hash1, hash3, GM_SHA256_DIGEST_SIZE) == 0);

    printf("✓ test_backend_switch\n");
}

/* Test deterministic random in test backend */
static void test_deterministic_random(void) {
    /* Create test backend context */
    gm_result_crypto_context_t ctx_result = gm_crypto_context_create(gm_crypto_backend_test());
    assert(GM_IS_OK(ctx_result));
    gm_crypto_context_t ctx = GM_UNWRAP(ctx_result);

    /* Random should be deterministic */
    gm_result_u32_t r1_result = gm_random_u32_with_context(&ctx);
    assert(GM_IS_OK(r1_result));
    uint32_t rand1 = GM_UNWRAP(r1_result);

    gm_result_u32_t r2_result = gm_random_u32_with_context(&ctx);
    assert(GM_IS_OK(r2_result));
    uint32_t rand2 = GM_UNWRAP(r2_result);

    gm_result_u32_t r3_result = gm_random_u32_with_context(&ctx);
    assert(GM_IS_OK(r3_result));
    uint32_t rand3 = GM_UNWRAP(r3_result);

    assert(rand1 == 0);
    assert(rand2 == 1);
    assert(rand3 == 2);

    /* Test random bytes */
    uint8_t buf[10];
    gm_result_void_t rand_result = gm_random_bytes_with_context(&ctx, buf, sizeof(buf));
    assert(GM_IS_OK(rand_result));

    for (int i = 0; i < 10; i++) {
        assert(buf[i] ==
               (uint8_t)(3 + i)); /* Continues from where u32 left off */
    }

    /* Create new test backend context (resets counter) */
    gm_result_crypto_context_t ctx2_result = gm_crypto_context_create(gm_crypto_backend_test());
    assert(GM_IS_OK(ctx2_result));
    gm_crypto_context_t ctx2 = GM_UNWRAP(ctx2_result);

    /* Should restart from 0 */
    r1_result = gm_random_u32_with_context(&ctx2);
    assert(GM_IS_OK(r1_result));
    uint32_t rand_reset = GM_UNWRAP(r1_result);
    assert(rand_reset == 0);

    printf("✓ test_deterministic_random\n");
}

/* Test streaming hash with backend */
static void test_streaming_hash(void) {
    /* Test with both backends */
    const gm_crypto_backend_t *backends[] = {gm_crypto_backend_libsodium(),
                                       gm_crypto_backend_test()};

    for (int i = 0; i < 2; i++) {
        /* Create context for this backend */
        gm_result_crypto_context_t ctx_result = gm_crypto_context_create(backends[i]);
        assert(GM_IS_OK(ctx_result));
        gm_crypto_context_t ctx = GM_UNWRAP(ctx_result);

        /* Streaming hash */
        gm_sha256_ctx_t sha_ctx;
        gm_result_void_t hash_result = gm_sha256_init_with_context(&ctx, &sha_ctx);
        assert(GM_IS_OK(hash_result));

        hash_result = gm_sha256_update_with_context(&ctx, &sha_ctx, "hello", 5);
        assert(GM_IS_OK(hash_result));

        hash_result = gm_sha256_update_with_context(&ctx, &sha_ctx, " ", 1);
        assert(GM_IS_OK(hash_result));

        hash_result = gm_sha256_update_with_context(&ctx, &sha_ctx, "world", 5);
        assert(GM_IS_OK(hash_result));

        uint8_t hash1[GM_SHA256_DIGEST_SIZE];
        hash_result = gm_sha256_final_with_context(&ctx, &sha_ctx, hash1);
        assert(GM_IS_OK(hash_result));

        /* One-shot hash */
        uint8_t hash2[GM_SHA256_DIGEST_SIZE];
        hash_result = gm_sha256_with_context(&ctx, "hello world", 11, hash2);
        assert(GM_IS_OK(hash_result));

        if (i == 0) {
            /* Libsodium backend should produce matching hashes */
            assert(memcmp(hash1, hash2, GM_SHA256_DIGEST_SIZE) == 0);
        } else {
            /* Test backend uses different algorithm for streaming */
            /* Just verify it produces something */
            assert(hash1[0] != 0 || hash1[1] != 0);
        }
    }

    printf("✓ test_streaming_hash\n");
}

/* Test invalid backend */
static void test_invalid_backend(void) {
    /* nullptr backend */
    gm_result_crypto_context_t result = gm_crypto_context_create(nullptr);
    assert(GM_IS_ERR(result));
    gm_error_free(GM_UNWRAP_ERR(result));

    /* Backend with missing functions */
    gm_crypto_backend_t incomplete = {.name = "incomplete",
                                      .sha256 = nullptr, /* Missing function */
                                      .sha256_init = nullptr,
                                      .sha256_update = nullptr,
                                      .sha256_final = nullptr,
                                      .random_bytes = nullptr,
                                      .random_u32 = nullptr,
                                      .random_u64 = nullptr,
                                      .context = nullptr};

    result = gm_crypto_context_create(&incomplete);
    assert(GM_IS_ERR(result));
    gm_error_free(GM_UNWRAP_ERR(result));

    printf("✓ test_invalid_backend\n");
}

int main(void) {
    printf("Running crypto backend tests...\n\n");

    test_backend_switch();
    test_deterministic_random();
    test_streaming_hash();
    test_invalid_backend();

    printf("\n✅ All crypto backend tests passed!\n");
    return 0;
}