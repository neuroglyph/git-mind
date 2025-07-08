/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/crypto/sha256.h"
#include "gitmind/crypto/backend.h"
#include "gitmind/error.h"
#include "gitmind/result.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Test vector structure */
typedef struct {
    const char *msg;
    const char *hex;
} test_vector_t;

/* Convert hex string to bytes */
static void hex_to_bytes(const char *hex, uint8_t *out, size_t out_len) {
    for (size_t i = 0; i < out_len; i++) {
        char hex_byte[3] = {hex[i * 2], hex[i * 2 + 1], '\0'};
        char *endptr;
        unsigned long byte = strtoul(hex_byte, &endptr, 16);
        
        /* Check for conversion errors */
        if (endptr != hex_byte + 2 || byte > UINT8_MAX) {
            /* For tests, we'll just set to 0 on error */
            out[i] = 0;
        } else {
            out[i] = (uint8_t)byte;
        }
    }
}

/* Convert bytes to hex string */
static void bytes_to_hex(const uint8_t *bytes, size_t len, char *hex) {
    static const char hex_chars[] = "0123456789abcdef";
    for (size_t i = 0; i < len; i++) {
        hex[i * 2] = hex_chars[bytes[i] >> 4];
        hex[i * 2 + 1] = hex_chars[bytes[i] & 0x0F];
    }
    hex[len * 2] = '\0';
}

/* Test SHA256 with official test vectors */
static void test_sha256_vectors(void) {
    gm_result_crypto_context_t ctx_result = gm_crypto_context_create(gm_crypto_backend_libsodium());
    assert(GM_IS_OK(ctx_result));
    gm_crypto_context_t ctx = GM_UNWRAP(ctx_result);

    /* NIST test vectors */
    static const test_vector_t vectors[] = {
        /* Empty string */
        {"",
         "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"},
        /* "abc" */
        {"abc",
         "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"},
        /* "The quick brown fox jumps over the lazy dog" */
        {"The quick brown fox jumps over the lazy dog",
         "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592"},
        /* Long string */
        {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq",
         "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1"}};

    for (size_t i = 0; i < sizeof(vectors) / sizeof(vectors[0]); i++) {
        uint8_t out[GM_SHA256_DIGEST_SIZE];
        uint8_t expected[GM_SHA256_DIGEST_SIZE];
        char out_hex[GM_SHA256_DIGEST_SIZE * 2 + 1];

        /* Convert expected hex to bytes */
        hex_to_bytes(vectors[i].hex, expected, GM_SHA256_DIGEST_SIZE);

        /* Compute SHA256 */
        gm_result_void_t result =
            gm_sha256_with_context(&ctx, vectors[i].msg, strlen(vectors[i].msg), out);
        assert(GM_IS_OK(result));

        /* Verify */
        if (memcmp(out, expected, GM_SHA256_DIGEST_SIZE) != 0) {
            bytes_to_hex(out, GM_SHA256_DIGEST_SIZE, out_hex);
            printf("❌ SHA256 test vector %zu failed\n", i);
            printf("  Input:    \"%s\"\n", vectors[i].msg);
            printf("  Expected: %s\n", vectors[i].hex);
            printf("  Got:      %s\n", out_hex);
            assert(0);
        }
    }

    printf("✓ test_sha256_vectors\n");
}

/* Test streaming SHA256 API */
static void test_sha256_streaming(void) {
    gm_result_crypto_context_t ctx_result = gm_crypto_context_create(gm_crypto_backend_libsodium());
    assert(GM_IS_OK(ctx_result));
    gm_crypto_context_t ctx = GM_UNWRAP(ctx_result);

    /* Test that streaming produces same result as one-shot */
    const char *test_data = "The quick brown fox jumps over the lazy dog";
    uint8_t one_shot[GM_SHA256_DIGEST_SIZE];
    uint8_t streaming[GM_SHA256_DIGEST_SIZE];

    /* One-shot */
    gm_result_void_t result = gm_sha256_with_context(&ctx, test_data, strlen(test_data), one_shot);
    assert(GM_IS_OK(result));

    /* Streaming in chunks */
    gm_sha256_ctx_t stream_ctx;
    result = gm_sha256_init_with_context(&ctx, &stream_ctx);
    assert(GM_IS_OK(result));

    result = gm_sha256_update_with_context(&ctx, &stream_ctx, "The quick ", 10);
    assert(GM_IS_OK(result));

    result = gm_sha256_update_with_context(&ctx, &stream_ctx, "brown fox ", 10);
    assert(GM_IS_OK(result));

    result = gm_sha256_update_with_context(&ctx, &stream_ctx, "jumps over ", 11);
    assert(GM_IS_OK(result));

    result = gm_sha256_update_with_context(&ctx, &stream_ctx, "the lazy dog", 12);
    assert(GM_IS_OK(result));

    result = gm_sha256_final_with_context(&ctx, &stream_ctx, streaming);
    assert(GM_IS_OK(result));

    /* Should match */
    assert(memcmp(one_shot, streaming, GM_SHA256_DIGEST_SIZE) == 0);

    printf("✓ test_sha256_streaming\n");
}

/* Test edge cases */
static void test_sha256_edge_cases(void) {
    gm_result_crypto_context_t ctx_result = gm_crypto_context_create(gm_crypto_backend_libsodium());
    assert(GM_IS_OK(ctx_result));
    gm_crypto_context_t ctx = GM_UNWRAP(ctx_result);

    uint8_t out[GM_SHA256_DIGEST_SIZE];
    gm_result_void_t result;

    /* nullptr data with zero length should work */
    result = gm_sha256_with_context(&ctx, nullptr, 0, out);
    assert(GM_IS_OK(result));

    /* Large data */
    char large_data[8192];
    memset(large_data, 'A', sizeof(large_data));
    result = gm_sha256_with_context(&ctx, large_data, sizeof(large_data), out);
    assert(GM_IS_OK(result));

    /* Test error cases */
    /* nullptr output buffer */
    result = gm_sha256_with_context(&ctx, "test", 4, nullptr);
    assert(GM_IS_ERR(result));
    gm_error_free(GM_UNWRAP_ERR(result));

    /* nullptr context for init */
    gm_sha256_ctx_t test_ctx;
    result = gm_sha256_init_with_context(nullptr, &test_ctx);
    assert(GM_IS_ERR(result));
    gm_error_free(GM_UNWRAP_ERR(result));

    /* nullptr context for update */
    result = gm_sha256_update_with_context(&ctx, nullptr, "test", 4);
    assert(GM_IS_ERR(result));
    gm_error_free(GM_UNWRAP_ERR(result));

    /* nullptr context for final */
    result = gm_sha256_final_with_context(&ctx, nullptr, out);
    assert(GM_IS_ERR(result));
    gm_error_free(GM_UNWRAP_ERR(result));

    printf("✓ test_sha256_edge_cases\n");
}

int main(void) {
    printf("Running SHA256 tests...\n\n");

    test_sha256_vectors();
    test_sha256_streaming();
    test_sha256_edge_cases();

    printf("\n✅ All SHA256 tests passed!\n");
    return 0;
}