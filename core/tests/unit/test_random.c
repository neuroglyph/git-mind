/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/crypto/random.h"
#include "gitmind/error.h"
#include "gitmind/result.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define RANDOM_TEST_SIZE 32
#define RANDOM_TEST_ITERATIONS 100

/* Test random bytes generation */
static void test_random_bytes(void) {
    uint8_t buf1[RANDOM_TEST_SIZE];
    uint8_t buf2[RANDOM_TEST_SIZE];

    /* Generate two random buffers */
    gm_result_void result1 = gm_random_bytes(buf1, sizeof(buf1));
    assert(GM_IS_OK(result1));

    gm_result_void result2 = gm_random_bytes(buf2, sizeof(buf2));
    assert(GM_IS_OK(result2));

    /* They should be different (probability of collision is negligible) */
    assert(memcmp(buf1, buf2, sizeof(buf1)) != 0);

    /* Should not be all zeros */
    uint8_t zeros[RANDOM_TEST_SIZE] = {0};
    assert(memcmp(buf1, zeros, sizeof(buf1)) != 0);
    assert(memcmp(buf2, zeros, sizeof(buf2)) != 0);

    /* Test error case - NULL buffer */
    gm_result_void err_result = gm_random_bytes(NULL, 10);
    assert(GM_IS_ERR(err_result));
    gm_error_free(GM_UNWRAP_ERR(err_result));

    /* Test zero size - should succeed but do nothing */
    gm_result_void zero_result = gm_random_bytes(buf1, 0);
    assert(GM_IS_OK(zero_result));

    printf("✓ test_random_bytes\n");
}

/* Test random u32 generation */
static void test_random_u32(void) {
    uint32_t values[RANDOM_TEST_ITERATIONS];

    /* Generate multiple random values */
    for (int i = 0; i < RANDOM_TEST_ITERATIONS; i++) {
        gm_result_u32 result = gm_random_u32();
        assert(GM_IS_OK(result));
        values[i] = GM_UNWRAP(result);
    }

    /* Check for uniqueness (very likely with CSPRNG) */
    int unique_count = 0;
    for (int i = 0; i < RANDOM_TEST_ITERATIONS; i++) {
        int is_unique = 1;
        for (int j = 0; j < i; j++) {
            if (values[i] == values[j]) {
                is_unique = 0;
                break;
            }
        }
        if (is_unique)
            unique_count++;
    }

    /* Should have mostly unique values */
    assert(unique_count > RANDOM_TEST_ITERATIONS * 0.9);

    printf("✓ test_random_u32\n");
}

/* Test random u64 generation */
static void test_random_u64(void) {
    gm_result_u64 result1 = gm_random_u64();
    assert(GM_IS_OK(result1));
    uint64_t val1 = GM_UNWRAP(result1);

    gm_result_u64 result2 = gm_random_u64();
    assert(GM_IS_OK(result2));
    uint64_t val2 = GM_UNWRAP(result2);

    /* Should be different */
    assert(val1 != val2);

    /* Should not be zero (extremely unlikely) */
    assert(val1 != 0);
    assert(val2 != 0);

    printf("✓ test_random_u64\n");
}

/* Test entropy quality (basic check) */
static void test_entropy_quality(void) {
    /* Generate a larger buffer */
    uint8_t buf[1024];
    gm_result_void result = gm_random_bytes(buf, sizeof(buf));
    assert(GM_IS_OK(result));

    /* Count bit distribution */
    int zero_bits = 0;
    int one_bits = 0;

    for (size_t i = 0; i < sizeof(buf); i++) {
        for (int bit = 0; bit < 8; bit++) {
            if (buf[i] & (1 << bit)) {
                one_bits++;
            } else {
                zero_bits++;
            }
        }
    }

    /* Should be roughly 50/50 distribution */
    int total_bits = sizeof(buf) * 8;
    double zero_ratio = (double)zero_bits / total_bits;
    double one_ratio = (double)one_bits / total_bits;

    /* Allow 45-55% range */
    assert(zero_ratio > 0.45 && zero_ratio < 0.55);
    assert(one_ratio > 0.45 && one_ratio < 0.55);

    printf("✓ test_entropy_quality (0s: %.1f%%, 1s: %.1f%%)\n",
           zero_ratio * 100, one_ratio * 100);
}

int main(void) {
    printf("Running CSPRNG tests...\n\n");

    test_random_bytes();
    test_random_u32();
    test_random_u64();
    test_entropy_quality();

    printf("\n✅ All CSPRNG tests passed!\n");
    return 0;
}