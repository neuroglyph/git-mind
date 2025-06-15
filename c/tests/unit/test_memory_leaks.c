// SPDX-License-Identifier: Apache-2.0
// © 2025 J. Kirby Ross / Neuroglyph Collective

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <stdlib.h>
#include <string.h>

#include "gitmind.h"

// Mock malloc/free to track allocations
static int malloc_count = 0;
static int free_count = 0;

void* __real_malloc(size_t size);
void __real_free(void* ptr);

void* __wrap_malloc(size_t size) {
    malloc_count++;
    return __real_malloc(size);
}

void __wrap_free(void* ptr) {
    if (ptr) free_count++;
    __real_free(ptr);
}

static void reset_counters(void) {
    malloc_count = 0;
    free_count = 0;
}

// Test link set allocation/deallocation
static void test_link_set_no_leaks(void **state) {
    (void) state;
    
    reset_counters();
    
    gm_link_set_t* set = gm_link_set_new();
    assert_non_null(set);
    
    // Add some links to trigger reallocation
    gm_link_t link = {
        .type = "TEST",
        .source = "file1.txt",
        .target = "file2.txt",
        .timestamp = 12345
    };
    
    for (int i = 0; i < 50; i++) {
        assert_int_equal(gm_link_set_add(set, &link), GM_OK);
    }
    
    gm_link_set_free(set);
    
    // Should have freed all allocations
    assert_true(malloc_count > 0); // Some allocations happened
    assert_int_equal(malloc_count, free_count); // All were freed
}

// Test traverse result allocation/deallocation
static void test_traverse_result_no_leaks(void **state) {
    (void) state;
    
    reset_counters();
    
    gm_traverse_result_t* result = gm_traverse_result_new();
    assert_non_null(result);
    
    // Add nodes to trigger reallocation
    gm_traverse_node_t node = {
        .path = "test.txt",
        .depth = 1,
        .parent = "parent.txt"
    };
    
    for (int i = 0; i < 50; i++) {
        assert_int_equal(gm_traverse_result_add(result, &node), GM_OK);
    }
    
    gm_traverse_result_free(result);
    
    // Should have freed all allocations
    assert_true(malloc_count > 0);
    assert_int_equal(malloc_count, free_count);
}

// Test error handling paths don't leak
static void test_error_paths_no_leaks(void **state) {
    (void) state;
    
    reset_counters();
    
    // Test link set allocation failure
    gm_link_set_t* set = gm_link_set_new();
    if (set) {
        // Simulate adding many items
        gm_link_t link = {.type = "TEST"};
        for (int i = 0; i < 1000; i++) {
            gm_link_set_add(set, &link);
        }
        gm_link_set_free(set);
    }
    
    // Even with potential allocation failures, should clean up
    assert_int_equal(malloc_count, free_count);
}

int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_link_set_no_leaks),
        cmocka_unit_test(test_traverse_result_no_leaks),
        cmocka_unit_test(test_error_paths_no_leaks),
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}