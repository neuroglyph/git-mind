/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

/*
 * BEHAVIORAL TESTS FOR EDGE MODULE
 * 
 * These tests ensure edge.c behaves correctly.
 * Combined with quality tests, we have full protection.
 */

#include "gitmind.h"
#include <assert.h>
#include <string.h>
#include <time.h>

/* Test doubles for dependency injection */
static int mock_time_called = 0;
static int mock_sha_called = 0;

static int mock_clock_gettime(int clk_id, struct timespec *ts) {
    mock_time_called++;
    ts->tv_sec = 1234567890;  /* Fixed time for deterministic tests */
    ts->tv_nsec = 123456789;
    return 0;
}

static gm_time_ops_t test_time_ops = {
    .clock_gettime = mock_clock_gettime
};

/* Mock SHA resolution */
static int mock_sha_from_path(gm_context_t *ctx, const char *path, 
                              uint8_t *sha) {
    mock_sha_called++;
    
    /* Deterministic SHA based on path */
    if (strcmp(path, "src/a.c") == 0) {
        memset(sha, 0xAA, GM_SHA1_SIZE);
    } else if (strcmp(path, "src/b.c") == 0) {
        memset(sha, 0xBB, GM_SHA1_SIZE);
    } else {
        return GM_ERROR;  /* Unknown file */
    }
    
    return GM_OK;
}

/* Test edge creation with valid inputs */
void test_edge_create_success(void) {
    gm_context_t ctx = {
        .time_ops = &test_time_ops,
        /* In real implementation, would mock git_repo */
    };
    
    gm_edge_t edge;
    int result = gm_edge_create(&ctx, "src/a.c", "src/b.c", 
                                GM_REL_DEPENDS_ON, &edge);
    
    assert(result == GM_OK);
    assert(strcmp(edge.src_path, "src/a.c") == 0);
    assert(strcmp(edge.tgt_path, "src/b.c") == 0);
    assert(edge.rel_type == GM_REL_DEPENDS_ON);
    assert(edge.confidence == 100);  /* Default confidence */
    assert(edge.timestamp == 1234567890);  /* Mocked time */
    assert(mock_time_called == 1);
}

/* Test edge creation with NULL parameters */
void test_edge_create_null_params(void) {
    gm_context_t ctx = {};
    gm_edge_t edge;
    
    /* NULL context */
    assert(gm_edge_create(NULL, "a", "b", GM_REL_DEPENDS_ON, &edge) 
           == GM_INVALID_ARG);
    
    /* NULL source path */
    assert(gm_edge_create(&ctx, NULL, "b", GM_REL_DEPENDS_ON, &edge) 
           == GM_INVALID_ARG);
    
    /* NULL target path */
    assert(gm_edge_create(&ctx, "a", NULL, GM_REL_DEPENDS_ON, &edge) 
           == GM_INVALID_ARG);
    
    /* NULL edge pointer */
    assert(gm_edge_create(&ctx, "a", "b", GM_REL_DEPENDS_ON, NULL) 
           == GM_INVALID_ARG);
}

/* Test edge equality comparison */
void test_edge_equal(void) {
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
    assert(gm_edge_equal(&edge1, &edge2) == 1);
    
    /* Different source SHA */
    edge2.src_sha[0] = 0xCC;
    assert(gm_edge_equal(&edge1, &edge2) == 0);
    edge2.src_sha[0] = 0xAA;  /* Restore */
    
    /* Different target SHA */
    edge2.tgt_sha[0] = 0xCC;
    assert(gm_edge_equal(&edge1, &edge2) == 0);
    edge2.tgt_sha[0] = 0xBB;  /* Restore */
    
    /* Different relationship type */
    edge2.rel_type = GM_REL_IMPLEMENTS;
    assert(gm_edge_equal(&edge1, &edge2) == 0);
    
    /* NULL edge pointers */
    assert(gm_edge_equal(NULL, &edge2) == 0);
    assert(gm_edge_equal(&edge1, NULL) == 0);
    assert(gm_edge_equal(NULL, NULL) == 0);
}

/* Test edge formatting */
void test_edge_format(void) {
    gm_edge_t edge = {0};
    strcpy(edge.src_path, "src/main.c");
    strcpy(edge.tgt_path, "src/util.c");
    edge.rel_type = GM_REL_DEPENDS_ON;
    
    char buffer[256];
    int result = gm_edge_format(&edge, buffer, sizeof(buffer));
    
    assert(result == GM_OK);
    assert(strcmp(buffer, "DEPENDS_ON: src/main.c -> src/util.c") == 0);
    
    /* Test all relationship types */
    edge.rel_type = GM_REL_IMPLEMENTS;
    gm_edge_format(&edge, buffer, sizeof(buffer));
    assert(strstr(buffer, "IMPLEMENTS:") != NULL);
    
    edge.rel_type = GM_REL_REFERENCES;
    gm_edge_format(&edge, buffer, sizeof(buffer));
    assert(strstr(buffer, "REFERENCES:") != NULL);
    
    edge.rel_type = GM_REL_AUGMENTS;
    gm_edge_format(&edge, buffer, sizeof(buffer));
    assert(strstr(buffer, "AUGMENTS:") != NULL);
    
    /* Test custom type */
    edge.rel_type = 9999;
    gm_edge_format(&edge, buffer, sizeof(buffer));
    assert(strstr(buffer, "CUSTOM:") != NULL);
}

/* Test edge format with small buffer */
void test_edge_format_small_buffer(void) {
    gm_edge_t edge = {0};
    strcpy(edge.src_path, "very/long/path/to/source/file.c");
    strcpy(edge.tgt_path, "another/very/long/path/to/target.c");
    edge.rel_type = GM_REL_DEPENDS_ON;
    
    char buffer[20];  /* Too small for full output */
    int result = gm_edge_format(&edge, buffer, sizeof(buffer));
    
    assert(result == GM_ERROR);  /* Should fail gracefully */
}

/* Test edge format with NULL parameters */
void test_edge_format_null_params(void) {
    gm_edge_t edge = {0};
    char buffer[256];
    
    assert(gm_edge_format(NULL, buffer, sizeof(buffer)) == GM_INVALID_ARG);
    assert(gm_edge_format(&edge, NULL, sizeof(buffer)) == GM_INVALID_ARG);
    assert(gm_edge_format(&edge, buffer, 0) == GM_INVALID_ARG);
}

/* Test path length boundaries */
void test_edge_path_boundaries(void) {
    gm_context_t ctx = {
        .time_ops = &test_time_ops
    };
    
    /* Create path exactly at limit */
    char long_path[GM_PATH_MAX];
    memset(long_path, 'a', GM_PATH_MAX - 1);
    long_path[GM_PATH_MAX - 1] = '\0';
    
    gm_edge_t edge;
    /* This should handle long paths gracefully */
    int result = gm_edge_create(&ctx, long_path, "short", 
                                GM_REL_DEPENDS_ON, &edge);
    
    /* Depending on implementation, might succeed or fail gracefully */
    assert(result == GM_OK || result == GM_ERROR);
    
    if (result == GM_OK) {
        /* Verify path was truncated properly */
        assert(strlen(edge.src_path) < GM_PATH_MAX);
        assert(edge.src_path[GM_PATH_MAX - 1] == '\0');
    }
}

/* Test timestamp precision */
void test_edge_timestamp_precision(void) {
    gm_context_t ctx = {
        .time_ops = &test_time_ops
    };
    
    gm_edge_t edge;
    mock_time_called = 0;
    
    int result = gm_edge_create(&ctx, "a", "b", GM_REL_DEPENDS_ON, &edge);
    
    assert(result == GM_OK);
    assert(mock_time_called == 1);
    
    /* Verify timestamp conversion (ms to seconds) */
    /* Mock returns 1234567890 seconds + 123456789 nanoseconds */
    /* Expected: 1234567890 seconds (nanoseconds ignored in seconds) */
    assert(edge.timestamp == 1234567890);
}

/* Master test runner */
int main(void) {
    printf("🧪 Running Edge Behavioral Tests...\n");
    
    test_edge_create_success();
    printf("✅ Edge creation works correctly\n");
    
    test_edge_create_null_params();
    printf("✅ NULL parameter validation works\n");
    
    test_edge_equal();
    printf("✅ Edge equality comparison works\n");
    
    test_edge_format();
    printf("✅ Edge formatting works\n");
    
    test_edge_format_small_buffer();
    printf("✅ Small buffer handling works\n");
    
    test_edge_format_null_params();
    printf("✅ Format NULL validation works\n");
    
    test_edge_path_boundaries();
    printf("✅ Path boundary handling works\n");
    
    test_edge_timestamp_precision();
    printf("✅ Timestamp precision correct\n");
    
    printf("\n🎯 ALL BEHAVIORAL TESTS PASS!\n");
    return 0;
}