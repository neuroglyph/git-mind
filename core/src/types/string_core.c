/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/security/memory.h"
#include "gitmind/types/string.h"

#include <stdlib.h>
#include <string.h>

/* Minimum capacity for string growth */
#define MIN_CAPACITY 16
#define GROWTH_FACTOR 2

/* Helper to calculate next capacity */
static size_t next_capacity(size_t current, size_t needed) {
    size_t new_cap = current * GROWTH_FACTOR;
    if (new_cap < needed) {
        new_cap = needed;
    }
    if (new_cap < MIN_CAPACITY) {
        new_cap = MIN_CAPACITY;
    }
    return new_cap;
}

/* Helper to create error result for string */
static inline gm_result_string gm_err_string(gm_error_t *e) {
    return (gm_result_string){.ok = false, .u.err = e};
}

/* Create new string from C string */
gm_result_string gm_string_new(const char *str) {
    if (!str) {
        return gm_string_new_n("", 0);
    }
    return gm_string_new_n(str, strlen(str));
}

/* Create new string with specific length */
gm_result_string gm_string_new_n(const char *str, size_t len) {
    gm_string_t s;
    s.length = len;
    s.capacity = len + 1; /* +1 for null terminator */

    s.data = malloc(s.capacity);
    if (!s.data) {
        return (gm_result_string){
            .ok = false,
            .u.err = GM_ERROR(GM_ERR_OUT_OF_MEMORY,
                              "Failed to allocate %zu bytes", s.capacity)};
    }

    if (str && len > 0) {
        GM_MEMCPY_SAFE(s.data, s.capacity, str, len);
    }
    s.data[len] = '\0';

    return (gm_result_string){.ok = true, .u.val = s};
}

/* Create string with pre-allocated capacity */
gm_result_string gm_string_with_capacity(size_t capacity) {
    gm_string_t s;
    s.length = 0;
    s.capacity = capacity > 0 ? capacity : MIN_CAPACITY;

    s.data = malloc(s.capacity);
    if (!s.data) {
        return (gm_result_string){
            .ok = false,
            .u.err = GM_ERROR(GM_ERR_OUT_OF_MEMORY,
                              "Failed to allocate %zu bytes", s.capacity)};
    }

    s.data[0] = '\0';
    return (gm_result_string){.ok = true, .u.val = s};
}

/* Create string from owned buffer */
gm_result_string gm_string_from_owned(char *str, size_t len, size_t capacity) {
    if (!str || capacity < len + 1) {
        return (gm_result_string){
            .ok = false,
            .u.err = GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                              "Invalid owned string parameters")};
    }

    gm_string_t s;
    s.data = str;
    s.length = len;
    s.capacity = capacity;

    return (gm_result_string){.ok = true, .u.val = s};
}

/* Copy string */
gm_result_string gm_string_copy(const gm_string_t *str) {
    if (!str) {
        return gm_string_new("");
    }
    return gm_string_new_n(str->data, str->length);
}

/* Concatenate strings */
gm_result_string gm_string_concat(const gm_string_t *a, const gm_string_t *b) {
    size_t len_a = a ? a->length : 0;
    size_t len_b = b ? b->length : 0;
    size_t total = len_a + len_b;

    gm_result_string result = gm_string_with_capacity(total + 1);
    if (GM_IS_ERR(result)) {
        return result;
    }

    gm_string_t s = GM_UNWRAP(result);
    if (a && len_a > 0) {
        GM_MEMCPY_SAFE(s.data, s.capacity, a->data, len_a);
    }
    if (b && len_b > 0) {
        GM_MEMCPY_SAFE(s.data + len_a, s.capacity - len_a, b->data, len_b);
    }
    s.data[total] = '\0';
    s.length = total;

    return (gm_result_string){.ok = true, .u.val = s};
}

/* Append to string */
gm_result_void gm_string_append(gm_string_t *str, const char *suffix) {
    if (!suffix) {
        return gm_ok_void();
    }
    return gm_string_append_n(str, suffix, strlen(suffix));
}

/* Append with length */
gm_result_void gm_string_append_n(gm_string_t *str, const char *suffix,
                                  size_t len) {
    if (!str) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "NULL string"));
    }
    if (!suffix || len == 0) {
        return gm_ok_void();
    }

    size_t new_len = str->length + len;
    if (new_len + 1 > str->capacity) {
        size_t new_cap = next_capacity(str->capacity, new_len + 1);
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
gm_result_void gm_string_clear(gm_string_t *str) {
    if (!str) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "NULL string"));
    }

    str->length = 0;
    if (str->data) {
        str->data[0] = '\0';
    }

    return gm_ok_void();
}

/* Extract substring */
gm_result_string gm_string_substring(const gm_string_t *str, size_t start,
                                     size_t len) {
    if (!str) {
        return gm_err_string(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "NULL string"));
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
gm_result_string gm_string_trim(const gm_string_t *str) {
    if (!str) {
        return gm_err_string(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "NULL string"));
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
        str->data = NULL;
        str->length = 0;
        str->capacity = 0;
    }
}