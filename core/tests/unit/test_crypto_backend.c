/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/crypto/backend.h"
#include "gitmind/crypto/random.h"
#include "gitmind/crypto/sha256.h"
#include "gitmind/error.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Test backend switching */
static void test_backend_switch(void) {
    /* Initialize crypto system */
    gm_result_void init_result = gm_crypto_init();
    assert(GM_IS_OK(init_result));

    /* Default should be libsodium */
    gm_crypto_backend_t *backend = gm_crypto_get_backend();
    assert(backend != NULL);
    assert(strcmp(backend->name, "libsodium") == 0);

    /* Test with libsodium backend */
    uint8_t hash1[GM_SHA256_DIGEST_SIZE];
    gm_result_void sha_result = gm_sha256("test", 4, hash1);
    assert(GM_IS_OK(sha_result));

    /* Switch to test backend */
    gm_result_backend result = gm_crypto_set_backend(gm_crypto_backend_test());
    assert(GM_IS_OK(result));

    backend = gm_crypto_get_backend();
    assert(strcmp(backend->name, "test") == 0);

    /* Test with test backend (should be deterministic) */
    uint8_t hash2[GM_SHA256_DIGEST_SIZE];
    sha_result = gm_sha256("test", 4, hash2);
    assert(GM_IS_OK(sha_result));

    /* Test backend should produce predictable output */
    assert(hash2[0] == 4); /* Length & 0xFF */
    assert(hash2[1] == 0); /* (Length >> 8) & 0xFF */
    assert(hash2[2] == 0); /* (Length >> 16) & 0xFF */
    assert(hash2[3] == 0); /* (Length >> 24) & 0xFF */
    assert(memcmp(hash2 + 4, "test", 4) == 0);

    /* Hashes should be different */
    assert(memcmp(hash1, hash2, GM_SHA256_DIGEST_SIZE) != 0);

    /* Switch back to libsodium */
    result = gm_crypto_set_backend(gm_crypto_backend_libsodium());
    assert(GM_IS_OK(result));

    uint8_t hash3[GM_SHA256_DIGEST_SIZE];
    sha_result = gm_sha256("test", 4, hash3);
    assert(GM_IS_OK(sha_result));

    /* Should match first hash */
    assert(memcmp(hash1, hash3, GM_SHA256_DIGEST_SIZE) == 0);

    printf("✓ test_backend_switch\n");
}

/* Test deterministic random in test backend */
static void test_deterministic_random(void) {
    /* Switch to test backend */
    gm_result_backend result = gm_crypto_set_backend(gm_crypto_backend_test());
    assert(GM_IS_OK(result));

    /* Random should be deterministic */
    gm_result_u32 r1_result = gm_random_u32();
    assert(GM_IS_OK(r1_result));
    uint32_t r1 = GM_UNWRAP(r1_result);

    gm_result_u32 r2_result = gm_random_u32();
    assert(GM_IS_OK(r2_result));
    uint32_t r2 = GM_UNWRAP(r2_result);

    gm_result_u32 r3_result = gm_random_u32();
    assert(GM_IS_OK(r3_result));
    uint32_t r3 = GM_UNWRAP(r3_result);

    assert(r1 == 0);
    assert(r2 == 1);
    assert(r3 == 2);

    /* Test random bytes */
    uint8_t buf[10];
    gm_result_void rand_result = gm_random_bytes(buf, sizeof(buf));
    assert(GM_IS_OK(rand_result));

    for (int i = 0; i < 10; i++) {
        assert(buf[i] ==
               (uint8_t)(3 + i)); /* Continues from where u32 left off */
    }

    /* Switch to new test backend instance (resets counter) */
    result = gm_crypto_set_backend(gm_crypto_backend_test());
    assert(GM_IS_OK(result));

    /* Should restart from 0 */
    r1_result = gm_random_u32();
    assert(GM_IS_OK(r1_result));
    r1 = GM_UNWRAP(r1_result);
    assert(r1 == 0);

    printf("✓ test_deterministic_random\n");
}

/* Test streaming hash with backend */
static void test_streaming_hash(void) {
    /* Test with both backends */
    gm_crypto_backend_t *backends[] = {gm_crypto_backend_libsodium(),
                                       gm_crypto_backend_test()};

    for (int i = 0; i < 2; i++) {
        gm_result_backend result = gm_crypto_set_backend(backends[i]);
        assert(GM_IS_OK(result));

        /* Streaming hash */
        gm_sha256_ctx_t ctx;
        gm_result_void hash_result = gm_sha256_init(&ctx);
        assert(GM_IS_OK(hash_result));

        hash_result = gm_sha256_update(&ctx, "hello", 5);
        assert(GM_IS_OK(hash_result));

        hash_result = gm_sha256_update(&ctx, " ", 1);
        assert(GM_IS_OK(hash_result));

        hash_result = gm_sha256_update(&ctx, "world", 5);
        assert(GM_IS_OK(hash_result));

        uint8_t hash1[GM_SHA256_DIGEST_SIZE];
        hash_result = gm_sha256_final(&ctx, hash1);
        assert(GM_IS_OK(hash_result));

        /* One-shot hash */
        uint8_t hash2[GM_SHA256_DIGEST_SIZE];
        hash_result = gm_sha256("hello world", 11, hash2);
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
    /* NULL backend */
    gm_result_backend result = gm_crypto_set_backend(NULL);
    assert(GM_IS_ERR(result));
    gm_error_free(GM_UNWRAP_ERR(result));

    /* Backend with missing functions */
    gm_crypto_backend_t incomplete = {.name = "incomplete",
                                      .sha256 = NULL, /* Missing function */
                                      .sha256_init = NULL,
                                      .sha256_update = NULL,
                                      .sha256_final = NULL,
                                      .random_bytes = NULL,
                                      .random_u32 = NULL,
                                      .random_u64 = NULL,
                                      .context = NULL};

    result = gm_crypto_set_backend(&incomplete);
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

    /* Cleanup */
    gm_result_void cleanup_result = gm_crypto_cleanup();
    assert(GM_IS_OK(cleanup_result));

    printf("\n✅ All crypto backend tests passed!\n");
    return 0;
}