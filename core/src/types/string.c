/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/types/string.h"

#include <string.h>

/* String views - these don't allocate memory */

/* Create string view */
gm_string_view_t gm_string_view(const char *str) {
    if (!str) {
        return (gm_string_view_t){.data = "", .length = 0};
    }
    return gm_string_view_n(str, strlen(str));
}

/* Create string view with length */
gm_string_view_t gm_string_view_n(const char *str, size_t len) {
    return (gm_string_view_t){.data = str, .length = len};
}

/* Create view from string */
gm_string_view_t gm_string_view_from_string(const gm_string_t *str) {
    if (!str) {
        return (gm_string_view_t){.data = "", .length = 0};
    }
    return (gm_string_view_t){.data = str->data, .length = str->length};
}

/* Comparison functions - these don't allocate memory */

/* Check if string is empty */
bool gm_string_is_empty(const gm_string_t *str) {
    return (bool)(!str || str->length == 0);
}

/* Compare strings */
bool gm_string_equals(const gm_string_t *str1, const gm_string_t *str2) {
    if (str1 == str2) {
        return true;
    }
    if (!str1 || !str2) {
        return false;
    }
    if (str1->length != str2->length) {
        return false;
    }
    return memcmp(str1->data, str2->data, str1->length) == 0;
}

/* Check prefix */
bool gm_string_starts_with(const gm_string_t *str, const char *prefix) {
    if (!str || !prefix) {
        return false;
    }
    size_t prefix_len = strlen(prefix);
    if (prefix_len > str->length) {
        return false;
    }
    return memcmp(str->data, prefix, prefix_len) == 0;
}

/* Check suffix */
bool gm_string_ends_with(const gm_string_t *str, const char *suffix) {
    if (!str || !suffix) {
        return false;
    }
    size_t suffix_len = strlen(suffix);
    if (suffix_len > str->length) {
        return false;
    }
    return memcmp(str->data + str->length - suffix_len, suffix, suffix_len) ==
           0;
}
