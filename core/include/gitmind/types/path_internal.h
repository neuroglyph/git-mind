/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_TYPES_PATH_INTERNAL_H
#define GITMIND_TYPES_PATH_INTERNAL_H

#include <stddef.h>
#include <stdint.h>

/* Forward declaration */
typedef struct gm_path gm_path_t;

/* Type-safe wrappers to prevent parameter swapping */

/* Wrapper for string length to distinguish from other size_t parameters */
typedef struct {
    size_t value;
} gm_string_length_t;

/* Wrapper for path separator character */
typedef struct {
    char value;
} gm_path_separator_t;

/* Wrapper for current capacity */
typedef struct {
    size_t value;
} gm_current_capacity_t;

/* Wrapper for needed capacity */
typedef struct {
    size_t value;
} gm_needed_capacity_t;

/* Wrapper for UTF8 state */
typedef struct {
    uint32_t value;
} gm_utf8_state_t;

/* Wrapper for code point */
typedef struct {
    uint32_t value;
} gm_utf8_codepoint_t;

/* Wrapper for base path */
typedef struct {
    const gm_path_t *value;
} gm_base_path_t;

/* Wrapper for relative path */
typedef struct {
    const gm_path_t *value;
} gm_relative_path_t;

/* Helper macros for creating wrapped values */
#define GM_STRING_LENGTH(n) ((gm_string_length_t){.value = (n)})
#define GM_PATH_SEPARATOR(c) ((gm_path_separator_t){.value = (c)})
#define GM_CURRENT_CAPACITY(n) ((gm_current_capacity_t){.value = (n)})
#define GM_NEEDED_CAPACITY(n) ((gm_needed_capacity_t){.value = (n)})
#define GM_UTF8_STATE(s) ((gm_utf8_state_t){.value = (s)})
#define GM_UTF8_CODEPOINT(c) ((gm_utf8_codepoint_t){.value = (c)})
#define GM_BASE_PATH(p) ((gm_base_path_t){.value = (p)})
#define GM_RELATIVE_PATH(p) ((gm_relative_path_t){.value = (p)})

#endif