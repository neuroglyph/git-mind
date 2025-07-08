/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_TYPES_STRING_H
#define GITMIND_TYPES_STRING_H

#include "gitmind/result.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

/**
 * @brief Owned string with explicit length and capacity
 *
 * ALWAYS owns its data. Use gm_string_free() when done.
 */
typedef struct gm_string {
    char *data;      /* UTF-8 data (null-terminated) */
    size_t length;   /* Length in bytes (excluding null) */
    size_t capacity; /* Allocated capacity */
} gm_string_t;

/**
 * @brief Non-owning string view (separate type for clarity)
 *
 * NEVER owns its data. Points to existing string.
 * The pointed-to data must outlive the view.
 */
typedef struct gm_string_view {
    const char *data; /* Points to existing string */
    size_t length;    /* Length in bytes */
} gm_string_view_t;

/* Define result types */
GM_RESULT_DEF(gm_result_string, gm_string_t);
GM_RESULT_DEF(gm_result_string_view, gm_string_view_t);

/* String creation (always owned) */
gm_result_string_t gm_string_new(const char *str);
gm_result_string_t gm_string_new_n(const char *str, size_t len);
gm_result_string_t gm_string_from_owned(char *str, size_t len, size_t capacity);
gm_result_string_t gm_string_with_capacity(size_t capacity);

/* String view creation (never owns) */
gm_string_view_t gm_string_view(const char *str);
gm_string_view_t gm_string_view_n(const char *str, size_t len);
gm_string_view_t gm_string_view_from_string(const gm_string_t *str);

/* String operations (all bounds-checked) */
gm_result_string_t gm_string_copy(const gm_string_t *str);
gm_result_string_t gm_string_concat(const gm_string_t *str_a,
                                    const gm_string_t *str_b);
gm_result_string_t gm_string_substring(const gm_string_t *str, size_t start,
                                       size_t len);
gm_result_string_t gm_string_trim(const gm_string_t *str);

/* String modification (in-place) */
gm_result_void_t gm_string_append(gm_string_t *str, const char *suffix);
gm_result_void_t gm_string_append_n(gm_string_t *str, const char *suffix,
                                    size_t len);
gm_result_void_t gm_string_clear(gm_string_t *str);

/* String validation */
gm_result_void_t gm_string_validate_utf8(const gm_string_t *str);
bool gm_string_is_empty(const gm_string_t *str);
bool gm_string_equals(const gm_string_t *str1, const gm_string_t *str2);
bool gm_string_starts_with(const gm_string_t *str, const char *prefix);
bool gm_string_ends_with(const gm_string_t *str, const char *suffix);

/* String cleanup (only for owned strings) */
void gm_string_free(gm_string_t *str);

/* Utility functions */
static inline size_t gm_string_len(const gm_string_t *str) {
    return str ? str->length : 0;
}

static inline const char *gm_string_data(const gm_string_t *str) {
    return str ? str->data : "";
}

static inline bool gm_string_view_equals(gm_string_view_t view1,
                                         gm_string_view_t view2) {
    if (view1.length != view2.length) {
        return false;
    }
    if (view1.data == view2.data) {
        return true;
    }
    return memcmp(view1.data, view2.data, view1.length) == 0;
}

#endif /* GITMIND_TYPES_STRING_H */
