// SPDX-License-Identifier: Apache-2.0
// © 2025 J. Kirby Ross / Neuroglyph Collective

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>

#include "gitmind.h"

// Test valid paths
static void test_valid_paths(void **state) {
    (void) state; /* unused */
    
    // Simple filenames
    assert_int_equal(gm_validate_link_path("file.txt"), GM_OK);
    assert_int_equal(gm_validate_link_path("README.md"), GM_OK);
    assert_int_equal(gm_validate_link_path("test-file-123.c"), GM_OK);
    
    // Relative paths
    assert_int_equal(gm_validate_link_path("src/main.c"), GM_OK);
    assert_int_equal(gm_validate_link_path("a/b/c/d.txt"), GM_OK);
    assert_int_equal(gm_validate_link_path("./file.txt"), GM_OK);
    assert_int_equal(gm_validate_link_path("./src/test.c"), GM_OK);
    
    // Current directory
    assert_int_equal(gm_validate_link_path("."), GM_OK);
    
    // Dots in filenames (not path traversal)
    assert_int_equal(gm_validate_link_path("..file.txt"), GM_OK);
    assert_int_equal(gm_validate_link_path("file..txt"), GM_OK);
    assert_int_equal(gm_validate_link_path("..."), GM_OK);
    assert_int_equal(gm_validate_link_path("...."), GM_OK);
    assert_int_equal(gm_validate_link_path("foo/..bar/baz"), GM_OK);
}

// Test absolute paths (should be rejected)
static void test_absolute_paths(void **state) {
    (void) state; /* unused */
    
    // Unix absolute paths
    assert_int_equal(gm_validate_link_path("/etc/passwd"), GM_ERR_INVALID_ARG);
    assert_int_equal(gm_validate_link_path("/home/user/file"), GM_ERR_INVALID_ARG);
    assert_int_equal(gm_validate_link_path("/"), GM_ERR_INVALID_ARG);
    
    // Windows absolute paths
    assert_int_equal(gm_validate_link_path("C:\\file.txt"), GM_ERR_INVALID_ARG);
    assert_int_equal(gm_validate_link_path("D:\\folder\\file"), GM_ERR_INVALID_ARG);
    assert_int_equal(gm_validate_link_path("C:/file.txt"), GM_ERR_INVALID_ARG);
}

// Test path traversal attempts (should be rejected)
static void test_path_traversal(void **state) {
    (void) state; /* unused */
    
    // Simple parent directory references
    assert_int_equal(gm_validate_link_path(".."), GM_ERR_INVALID_ARG);
    assert_int_equal(gm_validate_link_path("../"), GM_ERR_INVALID_ARG);
    assert_int_equal(gm_validate_link_path("../file.txt"), GM_ERR_INVALID_ARG);
    assert_int_equal(gm_validate_link_path("../../etc/passwd"), GM_ERR_INVALID_ARG);
    
    // Traversal in the middle
    assert_int_equal(gm_validate_link_path("foo/../bar"), GM_ERR_INVALID_ARG);
    assert_int_equal(gm_validate_link_path("foo/../../bar"), GM_ERR_INVALID_ARG);
    assert_int_equal(gm_validate_link_path("a/b/../../../c"), GM_ERR_INVALID_ARG);
    
    // Traversal at the end
    assert_int_equal(gm_validate_link_path("foo/.."), GM_ERR_INVALID_ARG);
    assert_int_equal(gm_validate_link_path("foo/bar/.."), GM_ERR_INVALID_ARG);
    
    // With current directory
    assert_int_equal(gm_validate_link_path("./../../etc"), GM_ERR_INVALID_ARG);
    assert_int_equal(gm_validate_link_path("./../file"), GM_ERR_INVALID_ARG);
    
    // Windows-style traversal
    assert_int_equal(gm_validate_link_path("..\\file.txt"), GM_ERR_INVALID_ARG);
    assert_int_equal(gm_validate_link_path("foo\\..\\bar"), GM_ERR_INVALID_ARG);
}

// Test edge cases
static void test_edge_cases(void **state) {
    (void) state; /* unused */
    
    // Empty and NULL
    assert_int_equal(gm_validate_link_path(""), GM_ERR_INVALID_ARG);
    assert_int_equal(gm_validate_link_path(NULL), GM_ERR_INVALID_ARG);
    
    // Very long path (create a path near the limit)
    char long_path[GM_MAX_PATH + 10];
    memset(long_path, 'a', GM_MAX_PATH + 5);
    long_path[GM_MAX_PATH + 5] = '\0';
    assert_int_equal(gm_validate_link_path(long_path), GM_ERR_PATH_TOO_LONG);
    
    // Path exactly at limit should be rejected (>= check)
    memset(long_path, 'a', GM_MAX_PATH);
    long_path[GM_MAX_PATH] = '\0';
    assert_int_equal(gm_validate_link_path(long_path), GM_ERR_PATH_TOO_LONG);
    
    // Path just under limit should be OK
    memset(long_path, 'a', GM_MAX_PATH - 1);
    long_path[GM_MAX_PATH - 1] = '\0';
    assert_int_equal(gm_validate_link_path(long_path), GM_OK);
}

// Test URL encoding and other bypass attempts
static void test_bypass_attempts(void **state) {
    (void) state; /* unused */
    
    // URL encoded paths (should be treated as literal)
    assert_int_equal(gm_validate_link_path("foo%2F..%2Fbar"), GM_OK); // %2F = /
    assert_int_equal(gm_validate_link_path("%2E%2E"), GM_OK); // %2E = .
    
    // Unicode/UTF-8 (assuming these get through as-is)
    // These should be treated as literal characters, not path components
    assert_int_equal(gm_validate_link_path("foo\xe2\x80\xa6bar"), GM_OK); // … (ellipsis)
}

// Test error messages
static void test_error_messages(void **state) {
    (void) state; /* unused */
    
    // Test that appropriate error messages are set
    gm_validate_link_path("");
    assert_string_equal(gm_last_error(), "Empty path");
    
    gm_validate_link_path("/etc/passwd");
    assert_true(strstr(gm_last_error(), "Absolute paths not allowed") != NULL);
    
    gm_validate_link_path("../file");
    assert_true(strstr(gm_last_error(), "Path traversal not allowed") != NULL);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_valid_paths),
        cmocka_unit_test(test_absolute_paths),
        cmocka_unit_test(test_path_traversal),
        cmocka_unit_test(test_edge_cases),
        cmocka_unit_test(test_bypass_attempts),
        cmocka_unit_test(test_error_messages),
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}