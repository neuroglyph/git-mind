/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/crypto/sha256.h"
#include "gitmind/error.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Test vector structure */
typedef struct {
    const char *msg;
    const char *hex;
} test_vector_t;

/* Convert hex string to bytes */
static void hex_to_bytes(const char *hex, uint8_t *out, size_t out_len) {
    for (size_t i = 0; i < out_len; i++) {
        unsigned int byte;
        sscanf(hex + i * 2, "%2x", &byte);
        out[i] = (uint8_t)byte;
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
        gm_result_void result =
            gm_sha256(vectors[i].msg, strlen(vectors[i].msg), out);
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
    /* Test that streaming produces same result as one-shot */
    const char *test_data = "The quick brown fox jumps over the lazy dog";
    uint8_t one_shot[GM_SHA256_DIGEST_SIZE];
    uint8_t streaming[GM_SHA256_DIGEST_SIZE];

    /* One-shot */
    gm_result_void result = gm_sha256(test_data, strlen(test_data), one_shot);
    assert(GM_IS_OK(result));

    /* Streaming in chunks */
    gm_sha256_ctx_t ctx;
    result = gm_sha256_init(&ctx);
    assert(GM_IS_OK(result));

    result = gm_sha256_update(&ctx, "The quick ", 10);
    assert(GM_IS_OK(result));

    result = gm_sha256_update(&ctx, "brown fox ", 10);
    assert(GM_IS_OK(result));

    result = gm_sha256_update(&ctx, "jumps over ", 11);
    assert(GM_IS_OK(result));

    result = gm_sha256_update(&ctx, "the lazy dog", 12);
    assert(GM_IS_OK(result));

    result = gm_sha256_final(&ctx, streaming);
    assert(GM_IS_OK(result));

    /* Should match */
    assert(memcmp(one_shot, streaming, GM_SHA256_DIGEST_SIZE) == 0);

    printf("✓ test_sha256_streaming\n");
}

/* Test edge cases */
static void test_sha256_edge_cases(void) {
    uint8_t out[GM_SHA256_DIGEST_SIZE];
    gm_result_void result;

    /* NULL data with zero length should work */
    result = gm_sha256(NULL, 0, out);
    assert(GM_IS_OK(result));

    /* Large data */
    char large_data[8192];
    memset(large_data, 'A', sizeof(large_data));
    result = gm_sha256(large_data, sizeof(large_data), out);
    assert(GM_IS_OK(result));

    /* Test error cases */
    /* NULL output buffer */
    result = gm_sha256("test", 4, NULL);
    assert(GM_IS_ERR(result));
    gm_error_free(GM_UNWRAP_ERR(result));

    /* NULL context for init */
    result = gm_sha256_init(NULL);
    assert(GM_IS_ERR(result));
    gm_error_free(GM_UNWRAP_ERR(result));

    /* NULL context for update */
    result = gm_sha256_update(NULL, "test", 4);
    assert(GM_IS_ERR(result));
    gm_error_free(GM_UNWRAP_ERR(result));

    /* NULL context for final */
    result = gm_sha256_final(NULL, out);
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