/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <gitmind/edge.h>
#include <gitmind/error.h>
#include <gitmind/context.h>
#include <gitmind/types.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Test constants */
#define BUFFER_SIZE 1024

/* Test helper to verify error codes */
static void assert_error_code(gm_error_t *err, int expected_code) {
    assert(err != NULL);
    assert(err->code == expected_code);
    gm_error_free(err);
}

/* Mock time operations for deterministic testing */
static int mock_time_called = 0;
static int mock_clock_gettime(clockid_t clk_id, struct timespec *ts) {
    (void)clk_id; /* Suppress unused parameter warning */
    mock_time_called++;
    ts->tv_sec = 1234567890;
    ts->tv_nsec = 123456789;
    return 0;
}

static gm_time_ops_t test_time_ops = {.clock_gettime = mock_clock_gettime};

/* Mock git operations for testing */
static int mock_resolve_blob_called = 0;
static int mock_resolve_blob(void *repo, const char *path, uint8_t *sha) {
    (void)repo; /* Suppress unused parameter warning */
    mock_resolve_blob_called++;
    
    /* Return deterministic SHAs based on path */
    if (strcmp(path, "src/a.c") == 0) {
        memset(sha, 0xAA, GM_SHA1_SIZE);
        return 0;
    } else if (strcmp(path, "src/b.c") == 0) {
        memset(sha, 0xBB, GM_SHA1_SIZE);
        return 0;
    }
    
    return -1; /* Not found */
}

/* Test edge creation with valid inputs */
static void test_edge_create_success(void) {
    printf("test_edge_create_success... ");
    
    gm_context_t ctx = {
        .time_ops = &test_time_ops,
        .git_ops = {.resolve_blob = mock_resolve_blob},
        .git_repo = NULL /* Mock doesn't need real repo */
    };
    
    mock_time_called = 0;
    mock_resolve_blob_called = 0;
    
    gm_result_edge_t result = gm_edge_create(&ctx, "src/a.c", "src/b.c", GM_REL_DEPENDS_ON);
    
    assert(result.ok);
    assert(mock_time_called == 1);
    assert(mock_resolve_blob_called == 2);
    
    gm_edge_t edge = result.u.val;
    assert(strcmp(edge.src_path, "src/a.c") == 0);
    assert(strcmp(edge.tgt_path, "src/b.c") == 0);
    assert(edge.rel_type == GM_REL_DEPENDS_ON);
    assert(edge.timestamp == 1234567890123UL); /* sec * 1000 + ns/1000000 */
    assert(strlen(edge.ulid) == GM_ULID_SIZE);
    
    /* Check SHAs */
    uint8_t expected_src[GM_SHA1_SIZE];
    uint8_t expected_tgt[GM_SHA1_SIZE];
    memset(expected_src, 0xAA, GM_SHA1_SIZE);
    memset(expected_tgt, 0xBB, GM_SHA1_SIZE);
    
    assert(memcmp(edge.src_sha, expected_src, GM_SHA1_SIZE) == 0);
    assert(memcmp(edge.tgt_sha, expected_tgt, GM_SHA1_SIZE) == 0);
    
    printf("OK\n");
}

/* Test edge creation with invalid arguments */
static void test_edge_create_invalid_args(void) {
    printf("test_edge_create_invalid_args... ");
    
    gm_context_t ctx = {
        .time_ops = &test_time_ops,
        .git_ops = {.resolve_blob = mock_resolve_blob},
        .git_repo = NULL
    };
    
    /* NULL context */
    gm_result_edge_t result = gm_edge_create(NULL, "a", "b", GM_REL_DEPENDS_ON);
    assert(!result.ok);
    assert_error_code(result.u.err, GM_ERR_INVALID_ARGUMENT);
    
    /* NULL source path */
    result = gm_edge_create(&ctx, NULL, "b", GM_REL_DEPENDS_ON);
    assert(!result.ok);
    assert_error_code(result.u.err, GM_ERR_INVALID_ARGUMENT);
    
    /* NULL target path */
    result = gm_edge_create(&ctx, "a", NULL, GM_REL_DEPENDS_ON);
    assert(!result.ok);
    assert_error_code(result.u.err, GM_ERR_INVALID_ARGUMENT);
    
    printf("OK\n");
}

/* Test edge equality comparison */
static void test_edge_equal(void) {
    printf("test_edge_equal... ");
    
    gm_edge_t edge1 = {0};
    gm_edge_t edge2 = {0};
    
    /* Set up identical edges */
    memset(edge1.src_sha, 0xAA, GM_SHA1_SIZE);
    memset(edge1.tgt_sha, 0xBB, GM_SHA1_SIZE);
    edge1.rel_type = GM_REL_DEPENDS_ON;
    
    memset(edge2.src_sha, 0xAA, GM_SHA1_SIZE);
    memset(edge2.tgt_sha, 0xBB, GM_SHA1_SIZE);
    edge2.rel_type = GM_REL_DEPENDS_ON;
    
    /* Should be equal */
    assert(gm_edge_equal(&edge1, &edge2) == true);
    
    /* Different source SHA */
    edge2.src_sha[0] = 0xCC;
    assert(gm_edge_equal(&edge1, &edge2) == false);
    edge2.src_sha[0] = 0xAA; /* Restore */
    
    /* Different target SHA */
    edge2.tgt_sha[0] = 0xCC;
    assert(gm_edge_equal(&edge1, &edge2) == false);
    edge2.tgt_sha[0] = 0xBB; /* Restore */
    
    /* Different relationship type */
    edge2.rel_type = GM_REL_IMPLEMENTS;
    assert(gm_edge_equal(&edge1, &edge2) == false);
    
    /* NULL edge pointers */
    assert(gm_edge_equal(NULL, &edge2) == false);
    assert(gm_edge_equal(&edge1, NULL) == false);
    assert(gm_edge_equal(NULL, NULL) == false);
    
    printf("OK\n");
}

/* Test edge formatting */
static void test_edge_format(void) {
    printf("test_edge_format... ");
    
    gm_edge_t edge = {0};
    strcpy(edge.src_path, "src/main.c");
    strcpy(edge.tgt_path, "src/util.c");
    edge.rel_type = GM_REL_DEPENDS_ON;
    
    char buffer[256];
    gm_result_void_t result = gm_edge_format(&edge, buffer, sizeof(buffer));
    
    assert(result.ok);
    assert(strcmp(buffer, "DEPENDS_ON: src/main.c -> src/util.c") == 0);
    
    /* Test all relationship types */
    edge.rel_type = GM_REL_IMPLEMENTS;
    result = gm_edge_format(&edge, buffer, sizeof(buffer));
    assert(result.ok);
    assert(strstr(buffer, "IMPLEMENTS:") != NULL);
    
    edge.rel_type = GM_REL_REFERENCES;
    result = gm_edge_format(&edge, buffer, sizeof(buffer));
    assert(result.ok);
    assert(strstr(buffer, "REFERENCES:") != NULL);
    
    edge.rel_type = GM_REL_AUGMENTS;
    result = gm_edge_format(&edge, buffer, sizeof(buffer));
    assert(result.ok);
    assert(strstr(buffer, "AUGMENTS:") != NULL);
    
    /* Test custom type */
    edge.rel_type = 9999;
    result = gm_edge_format(&edge, buffer, sizeof(buffer));
    assert(result.ok);
    assert(strstr(buffer, "CUSTOM:") != NULL);
    
    printf("OK\n");
}

/* Test edge formatting with invalid arguments */
static void test_edge_format_invalid_args(void) {
    printf("test_edge_format_invalid_args... ");
    
    gm_edge_t edge = {0};
    char buffer[256];
    
    /* NULL edge */
    gm_result_void_t result = gm_edge_format(NULL, buffer, sizeof(buffer));
    assert(!result.ok);
    assert_error_code(result.u.err, GM_ERR_INVALID_ARGUMENT);
    
    /* NULL buffer */
    result = gm_edge_format(&edge, NULL, sizeof(buffer));
    assert(!result.ok);
    assert_error_code(result.u.err, GM_ERR_INVALID_ARGUMENT);
    
    /* Zero buffer size */
    result = gm_edge_format(&edge, buffer, 0);
    assert(!result.ok);
    assert_error_code(result.u.err, GM_ERR_INVALID_ARGUMENT);
    
    printf("OK\n");
}

/* Test CBOR encode/decode round-trip */
static void test_cbor_round_trip(void) {
    printf("test_cbor_round_trip... ");
    
    /* Create original edge */
    gm_edge_t original = {0};
    memset(original.src_sha, 0xAA, GM_SHA1_SIZE);
    memset(original.tgt_sha, 0xBB, GM_SHA1_SIZE);
    original.rel_type = GM_REL_DEPENDS_ON;
    original.confidence = 0x3C00; /* 1.0 in half-float */
    original.timestamp = 1234567890123UL;
    strcpy(original.src_path, "src/main.c");
    strcpy(original.tgt_path, "src/util.c");
    strcpy(original.ulid, "01ARZ3NDEKTSV4RRFFQ69G5FAV");
    
    /* Encode to CBOR */
    uint8_t buffer[BUFFER_SIZE];
    size_t len = BUFFER_SIZE;
    gm_result_void_t encode_result = gm_edge_encode_cbor(&original, buffer, &len);
    
    assert(encode_result.ok);
    assert(len > 0);
    assert(len < BUFFER_SIZE);
    
    /* Decode from CBOR */
    gm_result_edge_t decode_result = gm_edge_decode_cbor(buffer, len);
    
    assert(decode_result.ok);
    
    gm_edge_t decoded = decode_result.u.val;
    
    /* Verify all fields match */
    assert(memcmp(decoded.src_sha, original.src_sha, GM_SHA1_SIZE) == 0);
    assert(memcmp(decoded.tgt_sha, original.tgt_sha, GM_SHA1_SIZE) == 0);
    assert(decoded.rel_type == original.rel_type);
    assert(decoded.confidence == original.confidence);
    assert(decoded.timestamp == original.timestamp);
    assert(strcmp(decoded.src_path, original.src_path) == 0);
    assert(strcmp(decoded.tgt_path, original.tgt_path) == 0);
    assert(strcmp(decoded.ulid, original.ulid) == 0);
    
    printf("OK\n");
}

/* Test CBOR encode with invalid arguments */
static void test_cbor_encode_invalid_args(void) {
    printf("test_cbor_encode_invalid_args... ");
    
    gm_edge_t edge = {0};
    uint8_t buffer[BUFFER_SIZE];
    size_t len = BUFFER_SIZE;
    
    /* NULL edge */
    gm_result_void_t result = gm_edge_encode_cbor(NULL, buffer, &len);
    assert(!result.ok);
    assert_error_code(result.u.err, GM_ERR_INVALID_ARGUMENT);
    
    /* NULL buffer */
    result = gm_edge_encode_cbor(&edge, NULL, &len);
    assert(!result.ok);
    assert_error_code(result.u.err, GM_ERR_INVALID_ARGUMENT);
    
    /* NULL len */
    result = gm_edge_encode_cbor(&edge, buffer, NULL);
    assert(!result.ok);
    assert_error_code(result.u.err, GM_ERR_INVALID_ARGUMENT);
    
    printf("OK\n");
}

/* Test CBOR decode with invalid arguments */
static void test_cbor_decode_invalid_args(void) {
    printf("test_cbor_decode_invalid_args... ");
    
    uint8_t buffer[BUFFER_SIZE];
    
    /* NULL buffer */
    gm_result_edge_t result = gm_edge_decode_cbor(NULL, BUFFER_SIZE);
    assert(!result.ok);
    assert_error_code(result.u.err, GM_ERR_INVALID_ARGUMENT);
    
    /* Zero length */
    result = gm_edge_decode_cbor(buffer, 0);
    assert(!result.ok);
    assert_error_code(result.u.err, GM_ERR_INVALID_ARGUMENT);
    
    printf("OK\n");
}

/* Test CBOR decode with invalid data */
static void test_cbor_decode_invalid_data(void) {
    printf("test_cbor_decode_invalid_data... ");
    
    uint8_t buffer[BUFFER_SIZE];
    
    /* Invalid CBOR map header */
    buffer[0] = 0x80; /* Array instead of map */
    gm_result_edge_t result = gm_edge_decode_cbor(buffer, 1);
    assert(!result.ok);
    assert_error_code(result.u.err, GM_ERR_INVALID_FORMAT);
    
    /* Wrong number of fields */
    buffer[0] = 0xA7; /* Map with 7 fields instead of 8 */
    result = gm_edge_decode_cbor(buffer, 1);
    assert(!result.ok);
    assert_error_code(result.u.err, GM_ERR_INVALID_FORMAT);
    
    printf("OK\n");
}

/* Main test runner */
int main(void) {
    printf("Running Edge Module Tests\n");
    printf("=========================\n");
    
    test_edge_create_success();
    test_edge_create_invalid_args();
    test_edge_equal();
    test_edge_format();
    test_edge_format_invalid_args();
    test_cbor_round_trip();
    test_cbor_encode_invalid_args();
    test_cbor_decode_invalid_args();
    test_cbor_decode_invalid_data();
    
    printf("\nAll Edge Tests Passed! ✅\n");
    return 0;
}
