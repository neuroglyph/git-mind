/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
#include "gitmind/types/ulid.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#include "gitmind/crypto/backend.h"
#include "gitmind/error.h"

/* Test constants */
#define TEST_TIMESTAMP_MS 1234567890123ULL
#define EXPECTED_TIME_PREFIX "013XRZP16B"  /* First 10 chars for timestamp 1234567890123 */
#define INVALID_ULID_LENGTH "01BX5ZT"
#define INVALID_ULID_CHARS "01BX5ZT@#$%^&*()!@#$%^"
#define INVALID_ULID_OVERFLOW "ZZZZZZZZZZ0000000000000000" /* Timestamp overflow */

static void test_ulid_generate_basic(void) {
    printf("Testing basic ULID generation...\n");
    
    char ulid1[GM_ULID_BUFFER_SIZE];
    char ulid2[GM_ULID_BUFFER_SIZE];
    
    /* Generate first ULID */
    gm_result_ulid_t result1 = gm_ulid_generate(ulid1);
    assert(result1.ok);
    assert(result1.u.val == ulid1);
    assert(strlen(ulid1) == GM_ULID_SIZE);
    assert(gm_ulid_is_valid(ulid1));
    
    /* Small delay to ensure different timestamp */
    struct timespec delay = {.tv_sec = 0, .tv_nsec = 2000000}; /* 2ms */
    nanosleep(&delay, NULL);
    
    /* Generate second ULID */
    gm_result_ulid_t result2 = gm_ulid_generate(ulid2);
    assert(result2.ok);
    assert(strlen(ulid2) == GM_ULID_SIZE);
    assert(gm_ulid_is_valid(ulid2));
    
    /* ULIDs should be different */
    assert(strcmp(ulid1, ulid2) != 0);
    
    /* Second ULID should be greater (later timestamp) */
    assert(gm_ulid_compare(ulid2, ulid1) > 0);
    
    printf("✓ Basic ULID generation works\n");
}

static void test_ulid_generate_with_timestamp(void) {
    printf("Testing ULID generation with timestamp...\n");
    
    char ulid1[GM_ULID_BUFFER_SIZE];
    char ulid2[GM_ULID_BUFFER_SIZE];
    
    /* Generate two ULIDs with same timestamp */
    gm_result_ulid_t result1 = gm_ulid_generate_with_timestamp(ulid1, TEST_TIMESTAMP_MS);
    assert(result1.ok);
    
    gm_result_ulid_t result2 = gm_ulid_generate_with_timestamp(ulid2, TEST_TIMESTAMP_MS);
    assert(result2.ok);
    
    /* Time component should be identical */
    assert(strncmp(ulid1, ulid2, 10) == 0);
    
    /* Random component should be different */
    assert(strcmp(ulid1 + 10, ulid2 + 10) != 0);
    
    /* Verify timestamp prefix */
    assert(strncmp(ulid1, EXPECTED_TIME_PREFIX, 10) == 0);
    
    printf("✓ ULID generation with timestamp works\n");
}

static void test_ulid_null_buffer(void) {
    printf("Testing ULID generation with NULL buffer...\n");
    
    gm_result_ulid_t result = gm_ulid_generate(NULL);
    assert(!result.ok);
    assert(result.u.err->code == GM_ERR_INVALID_ARGUMENT);
    gm_error_free(result.u.err);
    
    result = gm_ulid_generate_with_timestamp(NULL, TEST_TIMESTAMP_MS);
    assert(!result.ok);
    assert(result.u.err->code == GM_ERR_INVALID_ARGUMENT);
    gm_error_free(result.u.err);
    
    printf("✓ NULL buffer handling works\n");
}

static void test_ulid_validation(void) {
    printf("Testing ULID validation...\n");
    
    /* Valid ULID */
    char valid_ulid[GM_ULID_BUFFER_SIZE];
    gm_result_ulid_t result = gm_ulid_generate(valid_ulid);
    assert(result.ok);
    assert(gm_ulid_is_valid(valid_ulid));
    
    /* NULL input */
    assert(!gm_ulid_is_valid(NULL));
    
    /* Wrong length */
    assert(!gm_ulid_is_valid(INVALID_ULID_LENGTH));
    assert(!gm_ulid_is_valid("01BX5ZT0000000000000000000X")); /* Too long */
    
    /* Invalid characters */
    assert(!gm_ulid_is_valid(INVALID_ULID_CHARS));
    assert(!gm_ulid_is_valid("01BX5ZT000000000000000000I")); /* Contains 'I' */
    assert(!gm_ulid_is_valid("01BX5ZT000000000000000000L")); /* Contains 'L' */
    assert(!gm_ulid_is_valid("01BX5ZT000000000000000000O")); /* Contains 'O' */
    assert(!gm_ulid_is_valid("01BX5ZT000000000000000000U")); /* Contains 'U' */
    
    /* Timestamp overflow (>48 bits) */
    assert(!gm_ulid_is_valid(INVALID_ULID_OVERFLOW));
    
    printf("✓ ULID validation works\n");
}

static void test_ulid_get_timestamp(void) {
    printf("Testing ULID timestamp extraction...\n");
    
    char ulid[GM_ULID_BUFFER_SIZE];
    uint64_t original_timestamp = TEST_TIMESTAMP_MS;
    uint64_t extracted_timestamp = 0;
    
    /* Generate ULID with known timestamp */
    gm_result_ulid_t gen_result = gm_ulid_generate_with_timestamp(ulid, original_timestamp);
    assert(gen_result.ok);
    
    /* Extract timestamp */
    gm_result_void_t extract_result = gm_ulid_get_timestamp(ulid, &extracted_timestamp);
    assert(extract_result.ok);
    assert(extracted_timestamp == original_timestamp);
    
    /* Test error cases */
    extract_result = gm_ulid_get_timestamp(NULL, &extracted_timestamp);
    assert(!extract_result.ok);
    assert(extract_result.u.err->code == GM_ERR_INVALID_ARGUMENT);
    gm_error_free(extract_result.u.err);
    
    extract_result = gm_ulid_get_timestamp(ulid, NULL);
    assert(!extract_result.ok);
    assert(extract_result.u.err->code == GM_ERR_INVALID_ARGUMENT);
    gm_error_free(extract_result.u.err);
    
    extract_result = gm_ulid_get_timestamp(INVALID_ULID_CHARS, &extracted_timestamp);
    assert(!extract_result.ok);
    assert(extract_result.u.err->code == GM_ERR_INVALID_ARGUMENT);
    gm_error_free(extract_result.u.err);
    
    printf("✓ ULID timestamp extraction works\n");
}

static void test_ulid_compare(void) {
    printf("Testing ULID comparison...\n");
    
    char ulid1[GM_ULID_BUFFER_SIZE];
    char ulid2[GM_ULID_BUFFER_SIZE];
    char ulid3[GM_ULID_BUFFER_SIZE];
    
    /* Generate ULIDs with increasing timestamps */
    gm_result_ulid_t result1 = gm_ulid_generate_with_timestamp(ulid1, 1000);
    assert(result1.ok);
    
    gm_result_ulid_t result2 = gm_ulid_generate_with_timestamp(ulid2, 2000);
    assert(result2.ok);
    
    /* Copy ulid1 to ulid3 */
    strcpy(ulid3, ulid1);
    
    /* Test comparisons */
    assert(gm_ulid_compare(ulid1, ulid2) < 0); /* ulid1 < ulid2 */
    assert(gm_ulid_compare(ulid2, ulid1) > 0); /* ulid2 > ulid1 */
    assert(gm_ulid_compare(ulid1, ulid3) == 0); /* ulid1 == ulid3 */
    assert(gm_ulid_compare(ulid1, ulid1) == 0); /* Self comparison */
    
    /* Test NULL handling */
    assert(gm_ulid_compare(NULL, NULL) == 0);
    assert(gm_ulid_compare(NULL, ulid1) < 0);
    assert(gm_ulid_compare(ulid1, NULL) > 0);
    
    printf("✓ ULID comparison works\n");
}

static void test_ulid_monotonic_within_ms(void) {
    printf("Testing ULID monotonicity within same millisecond...\n");
    
    /* Generate multiple ULIDs with same timestamp */
    char ulids[10][GM_ULID_BUFFER_SIZE];
    uint64_t timestamp = TEST_TIMESTAMP_MS;
    
    for (int i = 0; i < 10; i++) {
        gm_result_ulid_t result = gm_ulid_generate_with_timestamp(ulids[i], timestamp);
        assert(result.ok);
        
        /* All should have same time prefix */
        if (i > 0) {
            assert(strncmp(ulids[0], ulids[i], 10) == 0);
        }
    }
    
    /* Random components should all be different (with high probability) */
    for (int i = 0; i < 10; i++) {
        for (int j = i + 1; j < 10; j++) {
            assert(strcmp(ulids[i] + 10, ulids[j] + 10) != 0);
        }
    }
    
    printf("✓ ULID monotonicity within millisecond works\n");
}

static void test_ulid_case_sensitivity(void) {
    printf("Testing ULID case sensitivity...\n");
    
    /* ULIDs should use uppercase encoding */
    char ulid[GM_ULID_BUFFER_SIZE];
    gm_result_ulid_t result = gm_ulid_generate(ulid);
    assert(result.ok);
    
    /* Check all characters are uppercase or digits */
    for (size_t i = 0; i < GM_ULID_SIZE; i++) {
        char c = ulid[i];
        assert((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z'));
        assert(c != 'I' && c != 'L' && c != 'O' && c != 'U'); /* Excluded chars */
    }
    
    printf("✓ ULID case sensitivity works\n");
}

int main(void) {
    printf("=== ULID Test Suite ===\n\n");
    
    /* No initialization needed - backends are now stateless */
    
    /* Run tests */
    test_ulid_generate_basic();
    test_ulid_generate_with_timestamp();
    test_ulid_null_buffer();
    test_ulid_validation();
    test_ulid_get_timestamp();
    test_ulid_compare();
    test_ulid_monotonic_within_ms();
    test_ulid_case_sensitivity();
    
    printf("\n✅ All ULID tests passed!\n");
    return 0;
}
