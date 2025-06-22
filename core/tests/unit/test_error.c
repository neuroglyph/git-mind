/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/error.h"
#include "gitmind/result.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Test basic error creation */
static void test_error_new(void) {
    gm_error_t* err = gm_error_new(GM_ERR_INVALID_ARGUMENT, "Test error: %d", 42);
    
    assert(err != NULL);
    assert(err->code == GM_ERR_INVALID_ARGUMENT);
    assert(strcmp(err->message, "Test error: 42") == 0);
    assert(err->cause == NULL);
    
    gm_error_free(err);
    printf("✓ test_error_new\n");
}

/* Test error with location */
static void test_error_with_location(void) {
    gm_error_t* err = GM_ERROR(GM_ERR_NOT_FOUND, "File not found: %s", "test.txt");
    
    assert(err != NULL);
    assert(err->code == GM_ERR_NOT_FOUND);
    assert(err->file != NULL);
    assert(err->line > 0);
    assert(err->func != NULL);
    assert(strstr(err->func, "test_error_with_location") != NULL);
    
    gm_error_free(err);
    printf("✓ test_error_with_location\n");
}

/* Test error chaining */
static void test_error_chain(void) {
    gm_error_t* cause = gm_error_new(GM_ERR_FILE_NOT_FOUND, "config.toml not found");
    gm_error_t* err = gm_error_wrap(cause, GM_ERR_INVALID_STATE, "Failed to load config");
    
    assert(err != NULL);
    assert(err->code == GM_ERR_INVALID_STATE);
    assert(err->cause == cause);
    assert(err->cause->code == GM_ERR_FILE_NOT_FOUND);
    
    gm_error_free(err); /* Should free entire chain */
    printf("✓ test_error_chain\n");
}

/* Test result types */
static void test_result_success(void) {
    gm_result_int result = gm_ok_int(42);
    
    assert(GM_IS_OK(result));
    assert(!GM_IS_ERR(result));
    assert(GM_UNWRAP(result) == 42);
    
    printf("✓ test_result_success\n");
}

/* Test result error */
static void test_result_error(void) {
    gm_error_t* err = gm_error_new(GM_ERR_INVALID_ARGUMENT, "Bad input");
    gm_result_int result = gm_err_int(err);
    
    assert(!GM_IS_OK(result));
    assert(GM_IS_ERR(result));
    assert(GM_UNWRAP_ERR(result) == err);
    assert(GM_UNWRAP_ERR(result)->code == GM_ERR_INVALID_ARGUMENT);
    
    gm_error_free(GM_UNWRAP_ERR(result));
    printf("✓ test_result_error\n");
}

/* Test function that uses results */
static gm_result_int divide(int a, int b) {
    if (b == 0) {
        return gm_err_int(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "Division by zero"));
    }
    return gm_ok_int(a / b);
}

/* Test GM_TRY macro */
static gm_result_int calculate(int x, int y) {
    gm_result_int div_result = divide(x, y);
    GM_TRY(div_result);
    
    int quotient = GM_UNWRAP(div_result);
    return gm_ok_int(quotient * 2);
}

static void test_try_macro(void) {
    /* Success case */
    gm_result_int result = calculate(10, 2);
    assert(GM_IS_OK(result));
    assert(GM_UNWRAP(result) == 10); /* (10/2) * 2 = 10 */
    
    /* Error case */
    result = calculate(10, 0);
    assert(GM_IS_ERR(result));
    assert(GM_UNWRAP_ERR(result)->code == GM_ERR_INVALID_ARGUMENT);
    
    gm_error_free(GM_UNWRAP_ERR(result));
    printf("✓ test_try_macro\n");
}

/* Test error formatting */
static void test_error_format(void) {
    gm_error_t* cause = gm_error_new(GM_ERR_FILE_NOT_FOUND, "config.toml not found");
    gm_error_t* err = gm_error_wrap(cause, GM_ERR_INVALID_STATE, "Failed to initialize");
    
    char* formatted = gm_error_format(err);
    assert(formatted != NULL);
    assert(strstr(formatted, "Failed to initialize") != NULL);
    assert(strstr(formatted, "caused by:") != NULL);
    assert(strstr(formatted, "config.toml not found") != NULL);
    
    free(formatted);
    gm_error_free(err);
    printf("✓ test_error_format\n");
}

int main(void) {
    printf("Running error handling tests...\n\n");
    
    test_error_new();
    test_error_with_location();
    test_error_chain();
    test_result_success();
    test_result_error();
    test_try_macro();
    test_error_format();
    
    printf("\n✅ All tests passed!\n");
    return 0;
}