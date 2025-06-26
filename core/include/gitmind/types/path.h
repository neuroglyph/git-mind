/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_TYPES_PATH_H
#define GITMIND_TYPES_PATH_H

#include "gitmind/result.h"
#include "gitmind/types/string.h"

#include <stdbool.h>
#include <stddef.h>

/* Forward declaration */
typedef struct gm_path_rules gm_path_rules_t;

/**
 * @brief Path state
 */
typedef enum {
    GM_PATH_STATE_RAW,       /* As provided */
    GM_PATH_STATE_CANONICAL, /* Canonicalized */
    GM_PATH_STATE_RELATIVE   /* Made relative to base */
} gm_path_state_t;

/**
 * @brief Path type
 */
typedef enum {
    GM_PATH_TYPE_UNKNOWN,
    GM_PATH_TYPE_FILE,
    GM_PATH_TYPE_DIRECTORY,
    GM_PATH_TYPE_SYMLINK,
    GM_PATH_TYPE_URL,
    GM_PATH_TYPE_IDENTIFIER
} gm_path_type_t;

/**
 * @brief Path with validation state
 *
 * Paths are always validated on creation. The validation
 * rules determine what constitutes a valid path.
 */
typedef struct gm_path {
    gm_string_t value;     /* The path string */
    char separator;        /* Path separator ('/' or '\\') */
    bool is_absolute;      /* Absolute vs relative */
    gm_path_state_t state; /* RAW or CANONICAL */
    bool is_validated;     /* Passed validation */
    gm_path_type_t type;   /* File, directory, etc. */
} gm_path_t;

/* Define result type */
GM_RESULT_DEF(gm_result_path, gm_path_t);

/* Path creation with validation */
gm_result_path_t gm_path_new(const char *str);
gm_result_path_t gm_path_from_string(gm_string_t str);

/* Path operations */
gm_result_path_t gm_path_join(const gm_path_t *base, const gm_path_t *relative);
gm_result_path_t gm_path_dirname(const gm_path_t *path);
gm_result_path_t gm_path_basename(const gm_path_t *path);
gm_result_path_t gm_path_canonicalize(const gm_path_t *path);
gm_result_path_t gm_path_make_relative(const gm_path_t *path,
                                       const gm_path_t *base);

/* Path validation */
gm_result_void_t gm_path_validate(const gm_path_t *path,
                                  const gm_path_rules_t *rules);
bool gm_path_is_safe(const gm_path_t *path); /* No traversal, etc. */
bool gm_path_has_extension(const gm_path_t *path, const char *ext);

/* Path comparison */
bool gm_path_equals(const gm_path_t *a, const gm_path_t *b);
bool gm_path_starts_with(const gm_path_t *path, const gm_path_t *prefix);
bool gm_path_is_child_of(const gm_path_t *path, const gm_path_t *parent);

/* Path cleanup */
void gm_path_free(gm_path_t *path);

/* Utility functions */
static inline const char *gm_path_str(const gm_path_t *path) {
    return path ? gm_string_data(&path->value) : "";
}

static inline size_t gm_path_len(const gm_path_t *path) {
    return path ? gm_string_len(&path->value) : 0;
}

static inline bool gm_path_is_empty(const gm_path_t *path) {
    return !path || gm_string_is_empty(&path->value);
}

#endif /* GITMIND_TYPES_PATH_H */