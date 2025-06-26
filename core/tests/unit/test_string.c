/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/types/string.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Test string creation */
static void test_string_new(void) {
    gm_result_string_t result = gm_string_new("hello world");
    assert(GM_IS_OK(result));

    gm_string_t str = GM_UNWRAP(result);
    assert(strcmp(str.data, "hello world") == 0);
    assert(str.length == 11);
    assert(str.capacity >= 12); /* room for null terminator */

    gm_string_free(&str);
    printf("✓ test_string_new\n");
}

/* Test empty string */
static void test_string_empty(void) {
    gm_result_string_t result = gm_string_new("");
    assert(GM_IS_OK(result));

    gm_string_t str = GM_UNWRAP(result);
    assert(gm_string_is_empty(&str));
    assert(str.length == 0);
    assert(str.data[0] == '\0');

    gm_string_free(&str);
    printf("✓ test_string_empty\n");
}

/* Test string append */
static void test_string_append(void) {
    gm_result_string_t result = gm_string_new("hello");
    assert(GM_IS_OK(result));

    gm_string_t str = GM_UNWRAP(result);

    /* Append string */
    gm_result_void_t append_result = gm_string_append(&str, " world");
    assert(GM_IS_OK(append_result));

    assert(strcmp(str.data, "hello world") == 0);
    assert(str.length == 11);

    /* Append more to test growth */
    append_result = gm_string_append(
        &str, "! This is a longer string to force reallocation");
    assert(GM_IS_OK(append_result));

    assert(gm_string_starts_with(&str, "hello world!"));

    gm_string_free(&str);
    printf("✓ test_string_append\n");
}

/* Test string views */
static void test_string_view(void) {
    /* View from C string */
    gm_string_view_t view = gm_string_view("hello world");
    assert(view.length == 11);
    assert(memcmp(view.data, "hello world", 11) == 0);

    /* View from owned string */
    gm_result_string_t result = gm_string_new("test string");
    assert(GM_IS_OK(result));

    gm_string_t str = GM_UNWRAP(result);
    gm_string_view_t str_view = gm_string_view_from_string(&str);

    assert(str_view.length == str.length);
    assert(str_view.data == str.data);

    /* Views can be compared */
    gm_string_view_t view2 = gm_string_view("test string");
    assert(gm_string_view_equals(str_view, view2));

    gm_string_free(&str);
    printf("✓ test_string_view\n");
}

/* Test string comparison */
static void test_string_compare(void) {
    gm_result_string_t r1 = gm_string_new("hello");
    gm_result_string_t r2 = gm_string_new("hello");
    gm_result_string_t r3 = gm_string_new("world");

    assert(GM_IS_OK(r1) && GM_IS_OK(r2) && GM_IS_OK(r3));

    gm_string_t s1 = GM_UNWRAP(r1);
    gm_string_t s2 = GM_UNWRAP(r2);
    gm_string_t s3 = GM_UNWRAP(r3);

    assert(gm_string_equals(&s1, &s2));
    assert(!gm_string_equals(&s1, &s3));

    /* Test prefix/suffix */
    assert(gm_string_starts_with(&s1, "hel"));
    assert(!gm_string_starts_with(&s1, "wor"));
    assert(gm_string_ends_with(&s1, "llo"));
    assert(!gm_string_ends_with(&s1, "wor"));

    gm_string_free(&s1);
    gm_string_free(&s2);
    gm_string_free(&s3);
    printf("✓ test_string_compare\n");
}

/* Test string concatenation */
static void test_string_concat(void) {
    gm_result_string_t r1 = gm_string_new("hello");
    gm_result_string_t r2 = gm_string_new(" world");

    assert(GM_IS_OK(r1) && GM_IS_OK(r2));

    gm_string_t s1 = GM_UNWRAP(r1);
    gm_string_t s2 = GM_UNWRAP(r2);

    gm_result_string_t result = gm_string_concat(&s1, &s2);
    assert(GM_IS_OK(result));

    gm_string_t concat = GM_UNWRAP(result);
    assert(strcmp(concat.data, "hello world") == 0);
    assert(concat.length == 11);

    gm_string_free(&s1);
    gm_string_free(&s2);
    gm_string_free(&concat);
    printf("✓ test_string_concat\n");
}

/* Test UTF-8 validation */
static void test_string_utf8(void) {
    gm_result_string_t result = gm_string_new("valid utf-8");
    assert(GM_IS_OK(result));

    gm_string_t str = GM_UNWRAP(result);
    gm_result_void_t valid = gm_string_validate_utf8(&str);
    assert(GM_IS_OK(valid));

    /* Test with invalid UTF-8 (continuation byte as start) */
    str.data[5] = '\x80'; /* Invalid UTF-8 start byte */
    valid = gm_string_validate_utf8(&str);
    assert(GM_IS_ERR(valid));
    gm_error_free(GM_UNWRAP_ERR(valid));

    gm_string_free(&str);
    printf("✓ test_string_utf8\n");
}

/* Test substring extraction */
static void test_string_substring(void) {
    gm_result_string_t result = gm_string_new("hello world");
    assert(GM_IS_OK(result));

    gm_string_t str = GM_UNWRAP(result);

    /* Extract "world" */
    gm_result_string_t sub_result = gm_string_substring(&str, 6, 5);
    assert(GM_IS_OK(sub_result));
    gm_string_t sub = GM_UNWRAP(sub_result);
    assert(strcmp(sub.data, "world") == 0);
    assert(sub.length == 5);
    gm_string_free(&sub);

    /* Extract from start */
    sub_result = gm_string_substring(&str, 0, 5);
    assert(GM_IS_OK(sub_result));
    sub = GM_UNWRAP(sub_result);
    assert(strcmp(sub.data, "hello") == 0);
    gm_string_free(&sub);

    /* Extract beyond end (should truncate) */
    sub_result = gm_string_substring(&str, 6, 100);
    assert(GM_IS_OK(sub_result));
    sub = GM_UNWRAP(sub_result);
    assert(strcmp(sub.data, "world") == 0);
    assert(sub.length == 5);
    gm_string_free(&sub);

    /* Start position out of bounds */
    sub_result = gm_string_substring(&str, 100, 5);
    assert(GM_IS_ERR(sub_result));
    gm_error_free(GM_UNWRAP_ERR(sub_result));

    /* NULL string */
    sub_result = gm_string_substring(NULL, 0, 5);
    assert(GM_IS_ERR(sub_result));
    gm_error_free(GM_UNWRAP_ERR(sub_result));

    gm_string_free(&str);
    printf("✓ test_string_substring\n");
}

/* Test string trimming */
static void test_string_trim(void) {
    /* Test various whitespace combinations */
    const char *test_cases[] = {"  hello world  ",
                                "\t\thello world\t\t",
                                "\n\nhello world\n\n",
                                "\r\nhello world\r\n",
                                "   \t\n\rhello world\r\n\t   ",
                                "hello world", /* No whitespace */
                                "   ",         /* All whitespace */
                                "",            /* Empty string */
                                NULL};

    const char *expected[] = {"hello world",
                              "hello world",
                              "hello world",
                              "hello world",
                              "hello world",
                              "hello world",
                              "",
                              ""};

    for (int i = 0; test_cases[i] != NULL; i++) {
        gm_result_string_t result = gm_string_new(test_cases[i]);
        assert(GM_IS_OK(result));

        gm_string_t str = GM_UNWRAP(result);

        gm_result_string_t trim_result = gm_string_trim(&str);
        assert(GM_IS_OK(trim_result));

        gm_string_t trimmed = GM_UNWRAP(trim_result);
        assert(strcmp(trimmed.data, expected[i]) == 0);

        gm_string_free(&str);
        gm_string_free(&trimmed);
    }

    /* Test NULL string */
    gm_result_string_t trim_result = gm_string_trim(NULL);
    assert(GM_IS_ERR(trim_result));
    gm_error_free(GM_UNWRAP_ERR(trim_result));

    printf("✓ test_string_trim\n");
}

int main(void) {
    printf("Running string type tests...\n\n");

    test_string_new();
    test_string_empty();
    test_string_append();
    test_string_view();
    test_string_compare();
    test_string_concat();
    test_string_utf8();
    test_string_substring();
    test_string_trim();

    printf("\n✅ All string tests passed!\n");
    return 0;
}