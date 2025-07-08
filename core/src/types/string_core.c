/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/security/memory.h"
#include "gitmind/types/string.h"
#include "gitmind/types/path_internal.h"

#include <stdlib.h>
#include <string.h>

/* Minimum capacity for string growth */
#define MIN_CAPACITY 16
#define GROWTH_FACTOR 2

/* Helper to calculate next capacity - grows from current to accommodate needed */
static size_t next_capacity(gm_current_capacity_t current /* current capacity */, 
                           gm_needed_capacity_t needed /* minimum needed */) {
    /* Parameter order: current capacity, then minimum needed capacity */
    size_t new_cap = current.value * GROWTH_FACTOR;
    if (new_cap < needed.value) {
        new_cap = needed.value;
    }
    if (new_cap < MIN_CAPACITY) {
        new_cap = MIN_CAPACITY;
    }
    return new_cap;
}

/* Helper to create error result for string */
static inline gm_result_string_t gm_err_string(gm_error_t *err) {
    return (gm_result_string_t){.ok = false, .u.err = err};
}

/* Create new string from C string */
gm_result_string_t gm_string_new(const char *str) {
    if (!str) {
        return gm_string_new_n("", 0);
    }
    return gm_string_new_n(str, strlen(str));
}

/* Create new string with specific length */
gm_result_string_t gm_string_new_n(const char *str, size_t len) {
    gm_string_t new_str;
    new_str.length = len;
    new_str.capacity = len + 1; /* +1 for null terminator */

    new_str.data = malloc(new_str.capacity);
    if (!new_str.data) {
        return (gm_result_string_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERR_OUT_OF_MEMORY,
                              "Failed to allocate %zu bytes", new_str.capacity)};
    }

    if (str && len > 0) {
        GM_MEMCPY_SAFE(new_str.data, new_str.capacity, str, len);
    }
    new_str.data[len] = '\0';

    return (gm_result_string_t){.ok = true, .u.val = new_str};
}

/* Create string with pre-allocated capacity */
gm_result_string_t gm_string_with_capacity(size_t capacity) {
    gm_string_t new_str;
    new_str.length = 0;
    new_str.capacity = capacity > 0 ? capacity : MIN_CAPACITY;

    new_str.data = malloc(new_str.capacity);
    if (!new_str.data) {
        return (gm_result_string_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERR_OUT_OF_MEMORY,
                              "Failed to allocate %zu bytes", new_str.capacity)};
    }

    new_str.data[0] = '\0';
    return (gm_result_string_t){.ok = true, .u.val = new_str};
}

/* Create string from owned buffer */
gm_result_string_t gm_string_from_owned(char *str, size_t len, size_t capacity) {
    if (!str || capacity < len + 1) {
        return (gm_result_string_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                              "Invalid owned string parameters")};
    }

    gm_string_t new_str;
    new_str.data = str;
    new_str.length = len;
    new_str.capacity = capacity;

    return (gm_result_string_t){.ok = true, .u.val = new_str};
}

/* Copy string */
gm_result_string_t gm_string_copy(const gm_string_t *str) {
    if (!str) {
        return gm_string_new("");
    }
    return gm_string_new_n(str->data, str->length);
}

/* Concatenate strings */
gm_result_string_t gm_string_concat(const gm_string_t *str_a, const gm_string_t *str_b) {
    size_t len_a = str_a ? str_a->length : 0;
    size_t len_b = str_b ? str_b->length : 0;
    size_t total = len_a + len_b;

    gm_result_string_t result = gm_string_with_capacity(total + 1);
    if (GM_IS_ERR(result)) {
        return result;
    }

    gm_string_t concat_str = GM_UNWRAP(result);
    if (str_a && len_a > 0) {
        GM_MEMCPY_SAFE(concat_str.data, concat_str.capacity, str_a->data, len_a);
    }
    if (str_b && len_b > 0) {
        GM_MEMCPY_SAFE(concat_str.data + len_a, concat_str.capacity - len_a, str_b->data, len_b);
    }
    concat_str.data[total] = '\0';
    concat_str.length = total;

    return (gm_result_string_t){.ok = true, .u.val = concat_str};
}

/* Append to string */
gm_result_void_t gm_string_append(gm_string_t *str, const char *suffix) {
    if (!suffix) {
        return gm_ok_void();
    }
    return gm_string_append_n(str, suffix, strlen(suffix));
}

/* Append with length */
gm_result_void_t gm_string_append_n(gm_string_t *str, const char *suffix,
                                  size_t len) {
    if (!str) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "nullptr string"));
    }
    if (!suffix || len == 0) {
        return gm_ok_void();
    }

    size_t new_len = str->length + len;
    if (new_len + 1 > str->capacity) {
        size_t new_cap = next_capacity(GM_CURRENT_CAPACITY(str->capacity), GM_NEEDED_CAPACITY(new_len + 1));
        char *new_data = realloc(str->data, new_cap);
        if (!new_data) {
            return gm_err_void(
                GM_ERROR(GM_ERR_OUT_OF_MEMORY, "Failed to grow string"));
        }
        str->data = new_data;
        str->capacity = new_cap;
    }

    GM_MEMCPY_SAFE(str->data + str->length, str->capacity - str->length, suffix,
                   len);
    str->length = new_len;
    str->data[new_len] = '\0';

    return gm_ok_void();
}

/* Clear string contents */
gm_result_void_t gm_string_clear(gm_string_t *str) {
    if (!str) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "nullptr string"));
    }

    str->length = 0;
    if (str->data) {
        str->data[0] = '\0';
    }

    return gm_ok_void();
}

/* Extract substring */
gm_result_string_t gm_string_substring(const gm_string_t *str, size_t start,
                                     size_t len) {
    if (!str) {
        return gm_err_string(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "nullptr string"));
    }

    /* Bounds check */
    if (start > str->length) {
        return gm_err_string(
            GM_ERROR(GM_ERR_INVALID_LENGTH,
                     "Start position %zu exceeds string length %zu", start,
                     str->length));
    }

    /* Adjust length if it would exceed string bounds */
    size_t available = str->length - start;
    if (len > available) {
        len = available;
    }

    return gm_string_new_n(str->data + start, len);
}

/* Trim whitespace from both ends */
gm_result_string_t gm_string_trim(const gm_string_t *str) {
    if (!str) {
        return gm_err_string(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "nullptr string"));
    }

    /* Empty string returns empty string */
    if (str->length == 0) {
        return gm_string_new("");
    }

    /* Find first non-whitespace character */
    size_t start = 0;
    while (start < str->length &&
           (str->data[start] == ' ' || str->data[start] == '\t' ||
            str->data[start] == '\n' || str->data[start] == '\r')) {
        start++;
    }

    /* All whitespace? Return empty string */
    if (start == str->length) {
        return gm_string_new("");
    }

    /* Find last non-whitespace character */
    size_t end = str->length - 1;
    while (end > start && (str->data[end] == ' ' || str->data[end] == '\t' ||
                           str->data[end] == '\n' || str->data[end] == '\r')) {
        end--;
    }

    /* Extract the trimmed portion */
    size_t trimmed_len = end - start + 1;
    return gm_string_new_n(str->data + start, trimmed_len);
}

/* Free string */
void gm_string_free(gm_string_t *str) {
    if (str && str->data) {
        free(str->data);
        str->data = nullptr;
        str->length = 0;
        str->capacity = 0;
    }
}