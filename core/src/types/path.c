/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/types/path.h"
#include "gitmind/types/path_internal.h"

#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/security/memory.h"
#include "gitmind/types/string.h"

#include <stdlib.h>
#include <string.h>

/* Path constants */
static const char GM_PATH_SEP_UNIX = '/';
static const char GM_PATH_SEP_WIN = '\\';
#define PATH_CURRENT_DIR "."
#define PATH_PARENT_DIR ".."

/* Path numeric constants */
enum {
    GM_PATH_MAX_COMPONENTS = 255,
    GM_PATH_MAX_LENGTH = 4096,
    GM_SYMLINK_EXT_LEN = 4
};

/* Path separator character constants */
static const char GM_SYMLINK_SUFFIX_MACOS = '@';
#define PATH_ENCODED_PARENT_1 "%2e%2e" /* URL encoded .. */
#define PATH_ENCODED_PARENT_2 "%2E%2E" /* URL encoded .. */
#define PATH_ENCODED_DOT_1 "%2e"       /* URL encoded . */
#define PATH_ENCODED_DOT_2 "%2E"       /* URL encoded . */

/* Symlink pattern constants */
#define SYMLINK_EXT_WINDOWS ".lnk"
#define SYMLINK_ARROW_PATTERN "->"
#define SYMLINK_ARROW_SPACED " -> "

/* Error message constants */
#define ERR_MSG_SYMLINK_LNK "Symlink patterns not allowed (.lnk)"
#define ERR_MSG_SYMLINK_AT "Symlink patterns not allowed (@ suffix)"
#define ERR_MSG_SYMLINK_ARROW "Symlink patterns not allowed (->)"
#define ERR_MSG_SYMLINK_GENERIC "Symlink patterns not allowed"
#define ERR_MSG_HIDDEN_FILES "Hidden files not allowed"
#define ERR_MSG_PATH_TOO_LONG "Path exceeds maximum length of %zu"
#define ERR_MSG_ABS_NOT_ALLOWED "Absolute paths not allowed"
#define ERR_MSG_REL_NOT_ALLOWED "Relative paths not allowed"
#define ERR_MSG_TRAVERSAL "Path traversal not allowed"
#define ERR_MSG_UNSAFE "Path contains unsafe sequences"
#define ERR_MSG_INVALID_FORMAT "Invalid path format"
#define ERR_MSG_NULL_PATH "nullptr path"
#define ERR_MSG_NULL_BASE "nullptr base path"
#define ERR_MSG_NULL_RELATIVE "nullptr relative path"
#define ERR_MSG_ABS_REL_MISMATCH                                               \
    "Cannot make relative between absolute and relative paths"
#define ERR_MSG_ALLOC_COMPONENTS "Failed to allocate components"
#define ERR_MSG_ALLOC_COMPONENT "Failed to allocate component"
#define ERR_MSG_ALLOC_CANONICAL "Failed to allocate canonical array"
#define ERR_MSG_OUT_OF_MEMORY "Failed to allocate"

/* Encoded traversal patterns */
#define ENCODED_PARENT_HEX_SLASH "..\\x2f"     /* .. with hex encoded / */
#define ENCODED_PARENT_HEX_BACKSLASH "..\\x5c" /* .. with hex encoded \ */
#define ENCODED_PARENT_OCT_SLASH "..\\057"     /* .. with octal encoded / */
#define ENCODED_PARENT_OCT_BACKSLASH "..\\134" /* .. with octal encoded \ */
#define ENCODED_UNICODE_DOTS "\\u002e\\u002e"  /* unicode .. */

/* UTF-8 homoglyphs */
#define HOMOGLYPH_FRACTION_SLASH "\xe2\x81\x84" /* U+2044 FRACTION SLASH */
#define HOMOGLYPH_DIVISION_SLASH "\xe2\x88\x95" /* U+2215 DIVISION SLASH */
#define HOMOGLYPH_WHITE_CIRCLE "\xe2\x9f\x8b"   /* U+27CB WHITE CIRCLE */
#define HOMOGLYPH_FULLWIDTH_SLASH                                              \
    "\xef\xbc\x8f" /* U+FF0F FULLWIDTH SOLIDUS                                 \
                    */
#define HOMOGLYPH_FULLWIDTH_BACKSLASH                                          \
    "\xef\xbc\xbc" /* U+FF3C FULLWIDTH REVERSE SOLIDUS */
#define HOMOGLYPH_ONE_DOT_LEADER "\xe2\x80\xa4" /* U+2024 ONE DOT LEADER */
#define HOMOGLYPH_TWO_DOT_LEADER "\xe2\x80\xa5" /* U+2025 TWO DOT LEADER */
#define HOMOGLYPH_ELLIPSIS "\xe2\x80\xa6"       /* U+2026 HORIZONTAL ELLIPSIS */
#define HOMOGLYPH_NBSP "\xc2\xa0"               /* U+00A0 NO-BREAK SPACE */
#define HOMOGLYPH_ZWSP "\xe2\x80\x8b"           /* U+200B ZERO WIDTH SPACE */
#define HOMOGLYPH_WORD_JOINER "\xe2\x81\xa0"    /* U+2060 WORD JOINER */
#define HOMOGLYPH_BOM "\xef\xbb\xbf" /* U+FEFF ZERO WIDTH NO-BREAK SPACE */

/* Helper to create error result for path */
static inline gm_result_path_t gm_err_path(gm_error_t *err) {
    return (gm_result_path_t){.ok = false, .u.err = err};
}

/* Helper to create success result for path */
static inline gm_result_path_t gm_ok_path(gm_path_t path) {
    return (gm_result_path_t){.ok = true, .u.val = path};
}

/* Helper to create error result for string */
static inline gm_result_string_t gm_err_string(gm_error_t *err) {
    return (gm_result_string_t){.ok = false, .u.err = err};
}

/* Detect path separator */
static char detect_separator(const char *str) {
    /* Look for first separator */
    const char *unix_sep = strchr(str, GM_PATH_SEP_UNIX);
    const char *win_sep = strchr(str, GM_PATH_SEP_WIN);

    if (unix_sep && !win_sep) {
        return GM_PATH_SEP_UNIX;
    }
    if (win_sep && !unix_sep) {
        return GM_PATH_SEP_WIN;
    }
    if (unix_sep && win_sep) {
        /* Use whichever comes first */
        return (unix_sep < win_sep) ? GM_PATH_SEP_UNIX : GM_PATH_SEP_WIN;
    }

    /* Default to system separator */
#ifdef _WIN32
    return GM_PATH_SEP_WIN;
#else
    return GM_PATH_SEP_UNIX;
#endif
}

/* Check if path is absolute */
static bool is_absolute_path(const char *str, char separator) {
    if (!str || !*str) {
        return false;
    }

    if (separator == GM_PATH_SEP_UNIX) {
        return str[0] == GM_PATH_SEP_UNIX;
    }

    /* Windows: C:\ or \\server\share */
    if (str[0] == GM_PATH_SEP_WIN && str[1] == GM_PATH_SEP_WIN) {
        return true; /* UNC path */
    }
    if (str[1] == ':' && str[2] == GM_PATH_SEP_WIN) {
        return true; /* Drive letter */
    }
    return false;
}

/* Basic path validation (no traversal check yet) */
static bool validate_path_basic(const char *str) {
    if (!str) {
        return false;
    }

    size_t len = strlen(str);
    if (len == 0) {
        return true; /* Empty path is valid */
    }

    /* Check for null bytes */
    for (size_t i = 0; i < len; i++) {
        if (str[i] == '\0') {
            return false;
        }
    }

    /* More validation can be added here */
    return true;
}

/* Initialize path structure */
static void init_path(gm_path_t *path, gm_string_t value, const char *str) {
    path->value = value;
    path->separator = detect_separator(str);
    path->is_absolute = is_absolute_path(str, path->separator);
    path->state = GM_PATH_STATE_RAW;
    path->is_validated = true;
    path->type = GM_PATH_TYPE_UNKNOWN;
}

/* Create new path from string */
gm_result_path_t gm_path_new(const char *str) {
    if (!str) {
        str = "";
    }

    if (!validate_path_basic(str)) {
        return gm_err_path(GM_ERROR(GM_ERR_INVALID_PATH, ERR_MSG_INVALID_FORMAT));
    }

    gm_result_string_t str_result = gm_string_new(str);
    if (GM_IS_ERR(str_result)) {
        return gm_err_path(GM_UNWRAP_ERR(str_result));
    }

    gm_path_t path;
    init_path(&path, GM_UNWRAP(str_result), str);
    return gm_ok_path(path);
}

/* Create path from existing string (takes ownership) */
gm_result_path_t gm_path_from_string(gm_string_t str) {
    const char *data = gm_string_data(&str);

    /* Basic validation */
    if (!validate_path_basic(data)) {
        gm_string_free(&str);
        return gm_err_path(
            GM_ERROR(GM_ERR_INVALID_PATH, ERR_MSG_INVALID_FORMAT));
    }

    gm_path_t path;
    path.value = str; /* Take ownership */
    path.separator = detect_separator(data);
    path.is_absolute = is_absolute_path(data, path.separator);
    path.state = GM_PATH_STATE_RAW;
    path.is_validated = true;
    path.type = GM_PATH_TYPE_UNKNOWN;

    return gm_ok_path(path);
}

/* Validate join parameters - base comes first, relative second */
static gm_error_t *validate_join_params(gm_base_path_t base /* base path */, 
                                       gm_relative_path_t relative /* relative path to append */) {
    /* Parameters are ordered: base first, then relative to append to it */
    if (!base.value) {
        return GM_ERROR(GM_ERR_INVALID_ARGUMENT, ERR_MSG_NULL_BASE);
    }
    if (!relative.value) {
        return GM_ERROR(GM_ERR_INVALID_ARGUMENT, ERR_MSG_NULL_RELATIVE);
    }
    return nullptr;
}

/* Check if path needs separator at end */
static bool needs_trailing_separator(const gm_path_t *path) {
    const char *str = gm_path_str(path);
    size_t len = gm_path_len(path);
    return (bool)(len > 0 && str[len - 1] != path->separator);
}

/* Join paths with separator */
static gm_result_string_t join_with_separator(const gm_path_t *base, const gm_path_t *relative) {
    if (!needs_trailing_separator(base)) {
        return gm_string_concat(&base->value, &relative->value);
    }
    
    /* Need to add separator */
    char sep_str[2] = {base->separator, '\0'};
    gm_string_t sep = {.data = sep_str, .length = 1, .capacity = 2};
    
    gm_result_string_t with_sep = gm_string_concat(&base->value, &sep);
    if (GM_IS_ERR(with_sep)) {
        return with_sep;
    }
    
    gm_string_t temp = GM_UNWRAP(with_sep);
    gm_result_string_t result = gm_string_concat(&temp, &relative->value);
    gm_string_free(&temp);
    return result;
}

/* Join two paths */
gm_result_path_t gm_path_join(const gm_path_t *base, const gm_path_t *relative) {
    gm_error_t *err = validate_join_params(GM_BASE_PATH(base), GM_RELATIVE_PATH(relative));
    if (err) {
        return gm_err_path(err);
    }

    /* Special cases */
    if (relative->is_absolute) {
        return gm_path_new(gm_path_str(relative));
    }
    if (gm_path_is_empty(base)) {
        return gm_path_new(gm_path_str(relative));
    }
    if (gm_path_is_empty(relative)) {
        return gm_path_new(gm_path_str(base));
    }

    /* Join with separator */
    gm_result_string_t result = join_with_separator(base, relative);
    if (GM_IS_ERR(result)) {
        return gm_err_path(GM_UNWRAP_ERR(result));
    }

    return gm_path_from_string(GM_UNWRAP(result));
}

/* Find last separator in path - searches str of given length for separator char */
static size_t find_last_separator(const char *str, 
                                 gm_path_separator_t sep /* separator character */, 
                                 gm_string_length_t len /* string length */) {
    for (size_t i = len.value; i > 0; i--) {
        if (str[i - 1] == sep.value) {
            return i - 1;
        }
    }
    return len.value; /* Not found */
}

/* Create root path */
static gm_result_path_t create_root_path(char sep) {
    char root[2] = {sep, '\0'};
    return gm_path_new(root);
}

/* Extract directory substring */
static gm_result_path_t extract_dirname(const gm_path_t *path, size_t last_sep) {
    gm_result_string_t dir_result = 
        gm_string_substring(&path->value, 0, last_sep);
    if (GM_IS_ERR(dir_result)) {
        return gm_err_path(GM_UNWRAP_ERR(dir_result));
    }
    return gm_path_from_string(GM_UNWRAP(dir_result));
}

/* Check for empty path and return current directory */
static gm_result_path_t handle_dirname_empty_path(size_t len) {
    return len == 0 ? gm_path_new(PATH_CURRENT_DIR) : gm_err_path(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "Invalid state"));
}

/* Handle dirname when no separator found */
static gm_result_path_t handle_dirname_no_separator(size_t last_sep, size_t len) {
    return last_sep == len ? gm_path_new(PATH_CURRENT_DIR) : gm_err_path(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "Invalid state"));
}

/* Extract directory name */
gm_result_path_t gm_path_dirname(const gm_path_t *path) {
    if (!path) {
        return gm_err_path(GM_ERROR(GM_ERR_INVALID_ARGUMENT, ERR_MSG_NULL_PATH));
    }

    const char *str = gm_path_str(path);
    size_t len = gm_path_len(path);

    if (len == 0) {
        return handle_dirname_empty_path(len);
    }

    size_t last_sep = find_last_separator(str, GM_PATH_SEPARATOR(path->separator), GM_STRING_LENGTH(len));
    if (last_sep == len) {
        return handle_dirname_no_separator(last_sep, len);
    }

    if (last_sep == 0 && path->is_absolute) {
        return create_root_path(path->separator);
    }

    return extract_dirname(path, last_sep);
}

/* Find start position of basename (after last separator) */
static size_t find_basename_start(const char *str, 
                                 gm_path_separator_t separator /* path separator */,
                                 gm_string_length_t len /* string length */) {
    for (size_t i = len.value; i > 0; i--) {
        if (str[i - 1] == separator.value) {
            return i;
        }
    }
    return 0;
}

/* Extract basename substring */
static gm_result_path_t extract_basename(const gm_path_t *path, size_t start, size_t len) {
    gm_result_string_t base_result = gm_string_substring(&path->value, start, len - start);
    if (GM_IS_ERR(base_result)) {
        return gm_err_path(GM_UNWRAP_ERR(base_result));
    }
    return gm_path_from_string(GM_UNWRAP(base_result));
}

/* Extract base name */
gm_result_path_t gm_path_basename(const gm_path_t *path) {
    if (!path) {
        return gm_err_path(GM_ERROR(GM_ERR_INVALID_ARGUMENT, ERR_MSG_NULL_PATH));
    }

    const char *str = gm_path_str(path);
    size_t len = gm_path_len(path);

    if (len == 0) {
        return gm_path_new("");
    }

    size_t start = find_basename_start(str, GM_PATH_SEPARATOR(path->separator), GM_STRING_LENGTH(len));
    return extract_basename(path, start, len);
}

/* Free path components */
static void free_components(char **components, size_t count) {
    if (components) {
        for (size_t i = 0; i < count; i++) {
            free(components[i]);
        }
        free((void *)components);
    }
}

/* Count path components */
static size_t count_path_components(const char *str, char sep) {
    size_t count = 1;
    for (const char *ptr = str; *ptr; ptr++) {
        if (*ptr == sep) {
            count++;
        }
    }
    return count;
}

/* Allocate and copy a single component */
static char *copy_component(const char *start, size_t len) {
    char *comp = malloc(len + 1);
    if (comp) {
        if (len > 0 && len <= GM_PATH_MAX_LENGTH) {
            GM_MEMCPY_SAFE(comp, len + 1, start, len);
        }
        comp[len] = '\0';
    }
    return comp;
}

/* Process a single path component during splitting */
static gm_result_void_t process_path_component(char **comps, size_t *idx,
                                             const char *start, size_t len,
                                             bool is_first) {
    if (len > 0 || (int)is_first) { /* Keep leading empty for absolute paths */
        comps[*idx] = copy_component(start, len);
        if (!comps[*idx]) {
            return gm_err_void(
                GM_ERROR(GM_ERR_OUT_OF_MEMORY, ERR_MSG_ALLOC_COMPONENT));
        }
        (*idx)++;
    }
    return gm_ok_void();
}

/* Allocate components array based on path */
static gm_result_void_t allocate_components_array(const char *str, char sep, char ***comps) {
    size_t comp_count = count_path_components(str, sep);
    *comps = (char **)calloc(comp_count + 1, sizeof(char *));
    if (!*comps) {
        return gm_err_void(GM_ERROR(GM_ERR_OUT_OF_MEMORY, ERR_MSG_ALLOC_COMPONENTS));
    }
    return gm_ok_void();
}

/* Process component at separator boundary */
static gm_result_void_t process_component_at_separator(char **comps, size_t *idx, 
                                                      const char *start, size_t len, bool is_first) {
    return process_path_component(comps, idx, start, len, is_first);
}

/* Process component found at separator boundary */
static gm_result_void_t handle_separator_boundary(char **comps, size_t *idx, const char **start, const char *ptr) {
    gm_result_void_t res = process_component_at_separator(comps, idx, *start, ptr - *start, *idx == 0);
    if (GM_IS_OK(res)) {
        *start = ptr + 1;
    }
    return res;
}

/* Process all components in path string */
static gm_result_void_t process_all_components(const char *str, char sep, char **comps, size_t *idx) {
    const char *start = str;
    const char *ptr = str;

    while (*ptr) {
        if (*ptr != sep) {
            ptr++;
            continue;
        }
        
        gm_result_void_t res = handle_separator_boundary(comps, idx, &start, ptr);
        if (GM_IS_ERR(res)) {
            return res;
        }
        ptr++;
    }

    size_t final_len = ptr - start;
    return (final_len > 0) ? process_component_at_separator(comps, idx, start, final_len, false) : gm_ok_void();
}

/* Split path into components */
static gm_result_void_t split_path_components(const char *str, char sep,
                                            char ***components, size_t *count) {
    *components = nullptr;
    *count = 0;

    if (!str || !*str) {
        return gm_ok_void();
    }

    char **comps = nullptr;
    gm_result_void_t alloc_res = allocate_components_array(str, sep, &comps);
    if (GM_IS_ERR(alloc_res)) {
        return alloc_res;
    }

    size_t idx = 0;
    gm_result_void_t process_res = process_all_components(str, sep, comps, &idx);
    if (GM_IS_ERR(process_res)) {
        free_components(comps, idx);
        return process_res;
    }

    *components = comps;
    *count = idx;
    return gm_ok_void();
}

/* Process canonical path component */
typedef struct {
    char **canonical;
    size_t *out_idx;
    bool is_absolute;
    char **components;
} canonical_context_t;

static bool process_canonical_component(const char *comp,
                                        canonical_context_t *ctx, size_t idx) {
    /* Skip . components */
    if (strcmp(comp, PATH_CURRENT_DIR) == 0) {
        return true;
    }

    /* Handle .. components */
    if (strcmp(comp, PATH_PARENT_DIR) == 0) {
        if (*ctx->out_idx > 0 &&
            strcmp(ctx->canonical[*ctx->out_idx - 1], "") != 0) {
            /* Go up one level (but not past root) */
            (*ctx->out_idx)--;
        } else if (!ctx->is_absolute) {
            /* Keep .. for relative paths that go above start */
            ctx->canonical[(*ctx->out_idx)++] = ctx->components[idx];
            ctx->components[idx] = nullptr; /* Transfer ownership */
        }
        /* For absolute paths, .. at root is ignored */
        return true;
    }

    return false;
}

/* Handle empty component list */
static gm_result_string_t handle_empty_components(bool is_absolute, char separator) {
    if (is_absolute) {
        char root[2] = {separator, '\0'};
        return gm_string_new(root);
    }
    return gm_string_new(PATH_CURRENT_DIR);
}

/* Check if separator needed before component */
static bool needs_separator(size_t index, bool is_absolute, const char *first_component) {
    return index > 0 || (is_absolute && strlen(first_component) > 0);
}

/* Append single component with separator if needed */
static gm_result_void_t append_component_with_sep(gm_string_t *joined, const char *component,
                                                 size_t index, bool is_absolute, char separator) {
    if (needs_separator(index, is_absolute, index == 0 ? component : "")) {
        char sep_str[2] = {separator, '\0'};
        gm_result_void_t sep_result = gm_string_append(joined, sep_str);
        if (GM_IS_ERR(sep_result)) {
            return sep_result;
        }
    }
    return gm_string_append(joined, component);
}

/* Build path string from components */
static gm_result_string_t build_path_from_components(char **components, size_t count,
                                                   bool is_absolute, char separator) {
    if (count == 0) {
        return handle_empty_components(is_absolute, separator);
    }

    gm_result_string_t result_str = gm_string_new("");
    if (GM_IS_ERR(result_str)) {
        return result_str;
    }

    gm_string_t joined = GM_UNWRAP(result_str);
    for (size_t i = 0; i < count; i++) {
        gm_result_void_t append_result = 
            append_component_with_sep(&joined, components[i], i, is_absolute, separator);
        if (GM_IS_ERR(append_result)) {
            gm_string_free(&joined);
            return gm_err_string(GM_UNWRAP_ERR(append_result));
        }
    }

    return (gm_result_string_t){.ok = true, .u.val = joined};
}

/* Canonicalize path (remove . and .. components) */
/* Allocate canonical components array */
static gm_result_void_t allocate_canonical_array(size_t comp_count, char ***canonical) {
    *canonical = (char **)calloc(comp_count + 1, sizeof(char *));
    if (!*canonical) {
        return gm_err_void(GM_ERROR(GM_ERR_OUT_OF_MEMORY, ERR_MSG_ALLOC_CANONICAL));
    }
    return gm_ok_void();
}

/* Process components and build canonical array */
static size_t build_canonical_components(char **components, size_t comp_count, 
                                       char **canonical, bool is_absolute) {
    size_t out_idx = 0;
    canonical_context_t ctx = {.canonical = canonical, .out_idx = &out_idx,
                               .is_absolute = is_absolute, .components = components};

    for (size_t i = 0; i < comp_count; i++) {
        if (!process_canonical_component(components[i], &ctx, i)) {
            canonical[out_idx++] = components[i];
            components[i] = nullptr;
        }
    }
    return out_idx;
}

/* Finalize canonicalized path */
static gm_result_path_t finalize_canonical_path(gm_result_string_t result_str) {
    gm_result_path_t final_result = gm_path_from_string(GM_UNWRAP(result_str));
    if (GM_IS_OK(final_result)) {
        gm_path_t canonical_path = GM_UNWRAP(final_result);
        canonical_path.state = GM_PATH_STATE_CANONICAL;
        return gm_ok_path(canonical_path);
    }
    return final_result;
}

/* Split and canonicalize components */
static gm_result_path_t canonicalize_components(const gm_path_t *path) {
    char **components = nullptr;
    size_t comp_count = 0;
    gm_result_void_t split_result = split_path_components(gm_path_str(path), path->separator, &components, &comp_count);
    if (GM_IS_ERR(split_result)) {
        return gm_err_path(GM_UNWRAP_ERR(split_result));
    }

    char **canonical = nullptr;
    gm_result_void_t alloc_result = allocate_canonical_array(comp_count, &canonical);
    if (GM_IS_ERR(alloc_result)) {
        free_components(components, comp_count);
        return gm_err_path(GM_UNWRAP_ERR(alloc_result));
    }

    size_t out_idx = build_canonical_components(components, comp_count, canonical, path->is_absolute);
    gm_result_string_t result_str = build_path_from_components(canonical, out_idx, path->is_absolute, path->separator);
    
    free_components(canonical, out_idx);
    free_components(components, comp_count);

    return GM_IS_ERR(result_str) ? gm_err_path(GM_UNWRAP_ERR(result_str)) : finalize_canonical_path(result_str);
}

/* Canonicalize path */
gm_result_path_t gm_path_canonicalize(const gm_path_t *path) {
    if (!path) {
        return gm_err_path(GM_ERROR(GM_ERR_INVALID_ARGUMENT, ERR_MSG_NULL_PATH));
    }
    return canonicalize_components(path);
}

/* Find common prefix length between two component arrays */
static size_t find_common_prefix(char **path_comps, size_t path_count,
                                 char **base_comps, size_t base_count) {
    size_t common = 0;
    while (common < path_count && common < base_count &&
           strcmp(path_comps[common], base_comps[common]) == 0) {
        common++;
    }
    return common;
}

/* Append relative traversal components to string */
typedef struct {
    size_t base_count;
    size_t common;
} traversal_counts_t;

/* Append separator if string not empty */
static gm_result_void_t append_separator_if_needed(gm_string_t *str, char separator) {
    if (gm_string_len(str) > 0) {
        char sep_str[2] = {separator, '\0'};
        return gm_string_append(str, sep_str);
    }
    return gm_ok_void();
}

static gm_result_void_t append_relative_traversal(gm_string_t *str,
                                                traversal_counts_t counts,
                                                char separator) {
    for (size_t i = counts.common; i < counts.base_count; i++) {
        gm_result_void_t sep_res = append_separator_if_needed(str, separator);
        if (GM_IS_ERR(sep_res)) {
            return sep_res;
        }
        gm_result_void_t append_res = gm_string_append(str, PATH_PARENT_DIR);
        if (GM_IS_ERR(append_res)) {
            return append_res;
        }
    }
    return gm_ok_void();
}

/* Append remaining path components to string */
typedef struct {
    char **components;
    size_t count;
    size_t start_idx;
} component_range_t;

static gm_result_void_t append_remaining_components(gm_string_t *str,
                                                  component_range_t range,
                                                  char separator) {
    for (size_t i = range.start_idx; i < range.count; i++) {
        gm_result_void_t sep_res = append_separator_if_needed(str, separator);
        if (GM_IS_ERR(sep_res)) {
            return sep_res;
        }
        gm_result_void_t append_res = gm_string_append(str, range.components[i]);
        if (GM_IS_ERR(append_res)) {
            return append_res;
        }
    }
    return gm_ok_void();
}

/* Check if path component is hidden */
static bool is_hidden_component(const char *comp) {
    return (bool)(comp[0] == '.' && comp[1] != '\0' &&
                  strcmp(comp, PATH_CURRENT_DIR) != 0 &&
                  strcmp(comp, PATH_PARENT_DIR) != 0);
}

/* Check for Windows .lnk extension */
static bool has_windows_lnk_extension(const char *comp, size_t comp_len) {
    return (bool)(comp_len > GM_SYMLINK_EXT_LEN &&
                  strcmp(comp + comp_len - GM_SYMLINK_EXT_LEN, SYMLINK_EXT_WINDOWS) == 0);
}

/* Check for macOS @ suffix */
static bool has_macos_symlink_suffix(const char *comp, size_t comp_len) {
    return (bool)(comp_len > 1 && comp[comp_len - 1] == GM_SYMLINK_SUFFIX_MACOS);
}

/* Check for arrow patterns in component */
static bool has_arrow_pattern(const char *comp) {
    return (bool)(strstr(comp, SYMLINK_ARROW_PATTERN) != nullptr ||
                  strstr(comp, SYMLINK_ARROW_SPACED) != nullptr);
}

/* Check for symlink pattern in component */
static gm_result_void_t check_symlink_pattern(const char *comp) {
    size_t comp_len = strlen(comp);

    if (has_windows_lnk_extension(comp, comp_len)) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_PATH, ERR_MSG_SYMLINK_LNK));
    }
    if (has_macos_symlink_suffix(comp, comp_len)) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_PATH, ERR_MSG_SYMLINK_AT));
    }
    if (has_arrow_pattern(comp)) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_PATH, ERR_MSG_SYMLINK_ARROW));
    }
    return gm_ok_void();
}

/* Validate path components for hidden files and symlinks */
/* Validate single component for safety rules */
static gm_result_void_t validate_single_component(const char *comp, bool allow_hidden, bool allow_symlinks) {
    if (!allow_hidden && (int)is_hidden_component(comp)) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_PATH, ERR_MSG_HIDDEN_FILES));
    }
    if (!allow_symlinks) {
        return check_symlink_pattern(comp);
    }
    return gm_ok_void();
}

static gm_result_void_t validate_path_components(const char *str, char separator,
                                               bool allow_hidden, bool allow_symlinks) {
    char **components = nullptr;
    size_t comp_count = 0;
    gm_result_void_t split_res = split_path_components(str, separator, &components, &comp_count);
    if (GM_IS_ERR(split_res)) {
        return split_res;
    }

    for (size_t i = 0; i < comp_count; i++) {
        gm_result_void_t validate_res = validate_single_component(components[i], allow_hidden, allow_symlinks);
        if (GM_IS_ERR(validate_res)) {
            free_components(components, comp_count);
            return validate_res;
        }
    }

    free_components(components, comp_count);
    return gm_ok_void();
}

/* Cleanup resources for make_relative */
typedef struct {
    char **path_comps;
    size_t path_count;
    char **base_comps;
    size_t base_count;
} make_relative_resources_t;

static void cleanup_make_relative(make_relative_resources_t *res) {
    if (res->path_comps) {
        free_components(res->path_comps, res->path_count);
    }
    if (res->base_comps) {
        free_components(res->base_comps, res->base_count);
    }
}

/* Build relative path string from components */
typedef struct {
    make_relative_resources_t *res;
    size_t common;
    char separator;
} relative_path_context_t;

/* Append traversal and components to build relative path */
static gm_result_void_t append_relative_path_parts(gm_string_t *rel_str, relative_path_context_t ctx) {
    traversal_counts_t trav_counts = {.base_count = ctx.res->base_count, .common = ctx.common};
    gm_result_void_t traverse_res = append_relative_traversal(rel_str, trav_counts, ctx.separator);
    if (GM_IS_ERR(traverse_res)) {
        return traverse_res;
    }

    component_range_t comp_range = {.components = ctx.res->path_comps,
                                    .count = ctx.res->path_count,
                                    .start_idx = ctx.common};
    return append_remaining_components(rel_str, comp_range, ctx.separator);
}

static gm_result_string_t build_relative_path_string(relative_path_context_t ctx) {
    gm_result_string_t result = gm_string_new("");
    if (GM_IS_ERR(result)) {
        return result;
    }

    gm_string_t rel_str = GM_UNWRAP(result);
    gm_result_void_t append_res = append_relative_path_parts(&rel_str, ctx);
    if (GM_IS_ERR(append_res)) {
        gm_string_free(&rel_str);
        return gm_err_string(GM_UNWRAP_ERR(append_res));
    }

    if (gm_string_len(&rel_str) == 0) {
        gm_string_free(&rel_str);
        return gm_string_new(PATH_CURRENT_DIR);
    }

    return (gm_result_string_t){.ok = true, .u.val = rel_str};
}

/* Check paths for make_relative - returns error if invalid */
static gm_result_void_t check_make_relative_args(const gm_path_t *path,
                                               const gm_path_t *base) {
    if (!path || !base) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, ERR_MSG_NULL_PATH));
    }

    /* Both paths must be either absolute or relative */
    if (path->is_absolute != base->is_absolute) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, ERR_MSG_ABS_REL_MISMATCH));
    }

    return gm_ok_void();
}

/* Canonicalize paths for make_relative */
typedef struct {
    gm_path_t canonical_path;
    gm_path_t canonical_base;
    bool path_ok;
    bool base_ok;
} canonical_paths_t;

static gm_result_void_t canonicalize_both_paths(const gm_path_t *path, const gm_path_t *base,
                                               canonical_paths_t *out) {
    gm_result_path_t canon_path = gm_path_canonicalize(path);
    if (GM_IS_ERR(canon_path)) {
        return gm_err_void(GM_UNWRAP_ERR(canon_path));
    }
    out->canonical_path = GM_UNWRAP(canon_path);
    out->path_ok = true;

    gm_result_path_t canon_base = gm_path_canonicalize(base);
    if (GM_IS_ERR(canon_base)) {
        return gm_err_void(GM_UNWRAP_ERR(canon_base));
    }
    out->canonical_base = GM_UNWRAP(canon_base);
    out->base_ok = true;
    return gm_ok_void();
}

/* Split both paths into components */
static gm_result_void_t split_both_paths(canonical_paths_t *paths, char separator, make_relative_resources_t *res) {
    gm_result_void_t split1 = split_path_components(gm_path_str(&paths->canonical_path),
                                                   separator, &res->path_comps, &res->path_count);
    if (GM_IS_ERR(split1)) {
        return split1;
    }

    gm_result_void_t split2 = split_path_components(gm_path_str(&paths->canonical_base),
                                                   separator, &res->base_comps, &res->base_count);
    if (GM_IS_ERR(split2)) {
        cleanup_make_relative(res);
        return split2;
    }
    return gm_ok_void();
}

/* Create relative path from components */
static gm_result_path_t create_relative_from_string(gm_result_string_t str_result) {
    if (GM_IS_ERR(str_result)) {
        return gm_err_path(GM_UNWRAP_ERR(str_result));
    }

    gm_string_t rel_str = GM_UNWRAP(str_result);
    gm_result_path_t rel_path = gm_path_from_string(rel_str);
    if (GM_IS_OK(rel_path)) {
        gm_path_t relative = GM_UNWRAP(rel_path);
        relative.state = GM_PATH_STATE_RELATIVE;
        return gm_ok_path(relative);
    }
    return rel_path;
}

/* Split paths and build relative */
static gm_result_path_t process_relative_path(canonical_paths_t *paths, char separator) {
    make_relative_resources_t res = {0};
    
    gm_result_void_t split_res = split_both_paths(paths, separator, &res);
    if (GM_IS_ERR(split_res)) {
        return gm_err_path(GM_UNWRAP_ERR(split_res));
    }

    size_t common = find_common_prefix(res.path_comps, res.path_count,
                                       res.base_comps, res.base_count);
    relative_path_context_t rel_ctx = {.res = &res, .common = common, .separator = separator};
    gm_result_string_t str_result = build_relative_path_string(rel_ctx);
    cleanup_make_relative(&res);
    
    return create_relative_from_string(str_result);
}

/* Make path relative to base */
gm_result_path_t gm_path_make_relative(const gm_path_t *path, const gm_path_t *base) {
    gm_result_void_t check_res = check_make_relative_args(path, base);
    if (GM_IS_ERR(check_res)) {
        return gm_err_path(GM_UNWRAP_ERR(check_res));
    }

    canonical_paths_t paths = {0};
    gm_result_void_t canon_res = canonicalize_both_paths(path, base, &paths);
    if (GM_IS_ERR(canon_res)) {
        if (paths.path_ok) {
            gm_path_free(&paths.canonical_path);
        }
        return gm_err_path(GM_UNWRAP_ERR(canon_res));
    }

    gm_result_path_t result = process_relative_path(&paths, path->separator);
    gm_path_free(&paths.canonical_path);
    gm_path_free(&paths.canonical_base);
    return result;
}

/* Default path validation rules */
static const struct {
    size_t max_length;
    bool allow_traversal;
    bool allow_absolute;
    bool allow_relative;
    bool allow_symlinks;
    bool allow_hidden;
} GM_DEFAULT_PATH_RULES = {.max_length = GM_PATH_MAX_LENGTH,
                        .allow_traversal = false,
                        .allow_absolute = true,
                        .allow_relative = true,
                        .allow_symlinks = false,
                        .allow_hidden = true};

/* Check basic path constraints */
static gm_result_void_t check_path_constraints(const gm_path_t *path, size_t max_length,
                                              bool allow_absolute, bool allow_relative) {
    size_t len = gm_path_len(path);
    if (len > max_length) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_PATH, ERR_MSG_PATH_TOO_LONG, max_length));
    }
    if ((int)path->is_absolute && !allow_absolute) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_PATH, ERR_MSG_ABS_NOT_ALLOWED));
    }
    if (!path->is_absolute && !allow_relative) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_PATH, ERR_MSG_REL_NOT_ALLOWED));
    }
    return gm_ok_void();
}

/* Check path safety and traversal */
static gm_result_void_t check_path_safety(const gm_path_t *path, bool allow_traversal) {
    const char *str = gm_path_str(path);
    if (!allow_traversal && strstr(str, "..")) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_PATH, ERR_MSG_TRAVERSAL));
    }
    if (!gm_path_is_safe(path)) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_PATH, ERR_MSG_UNSAFE));
    }
    return gm_ok_void();
}

/* Validate all path constraints and safety */
static gm_result_void_t validate_all_rules(const gm_path_t *path) {
    gm_result_void_t constraint_res = check_path_constraints(path, 
        GM_DEFAULT_PATH_RULES.max_length, GM_DEFAULT_PATH_RULES.allow_absolute,
        GM_DEFAULT_PATH_RULES.allow_relative);
    if (GM_IS_ERR(constraint_res)) {
        return constraint_res;
    }

    gm_result_void_t safety_res = check_path_safety(path, GM_DEFAULT_PATH_RULES.allow_traversal);
    if (GM_IS_ERR(safety_res)) {
        return safety_res;
    }

    gm_result_void_t comp_res = validate_path_components(gm_path_str(path), path->separator,
        GM_DEFAULT_PATH_RULES.allow_hidden, GM_DEFAULT_PATH_RULES.allow_symlinks);
    if (GM_IS_ERR(comp_res)) {
        return comp_res;
    }

    if (!GM_DEFAULT_PATH_RULES.allow_symlinks && strstr(gm_path_str(path), SYMLINK_ARROW_SPACED)) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_PATH, ERR_MSG_SYMLINK_GENERIC));
    }
    return gm_ok_void();
}

/* Validate path with rules */
gm_result_void_t gm_path_validate(const gm_path_t *path, const gm_path_rules_t *rules) {
    if (!path) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, ERR_MSG_NULL_PATH));
    }
    (void)rules; /* Suppress unused parameter warning */
    return validate_all_rules(path);
}

/* Check if path contains control characters */
static bool has_control_chars(const char *str, size_t len) {
    for (size_t i = 0; i < len; i++) {
        unsigned char chr = (unsigned char)str[i];
        if (chr < 0x20 || chr == 0x7F) { /* Control chars */
            return true;
        }
    }
    return false;
}

/* Check if path contains encoded traversal sequences */
static bool has_encoded_traversal(const char *str) {
    /* Check for URL encoded .. */
    if (strstr(str, PATH_ENCODED_PARENT_1) ||
        strstr(str, PATH_ENCODED_PARENT_2)) {
        return true;
    }

    /* Check for other encoding tricks */
    const char *patterns[] = {
        ENCODED_PARENT_HEX_SLASH, ENCODED_PARENT_HEX_BACKSLASH,
        ENCODED_PARENT_OCT_SLASH, ENCODED_PARENT_OCT_BACKSLASH,
        ENCODED_UNICODE_DOTS,     nullptr};

    for (int i = 0; patterns[i]; i++) {
        if (strstr(str, patterns[i])) {
            return true;
        }
    }

    return false;
}

/* Check for null bytes in path */
static bool has_null_bytes(const char *str, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (str[i] == '\0') {
            return true;
        }
    }
    return false;
}

/* Check for dangerous homoglyphs */
static bool has_dangerous_homoglyphs(const char *str) {
    const char *dangerous_sequences[] = {
        HOMOGLYPH_FRACTION_SLASH, HOMOGLYPH_DIVISION_SLASH, HOMOGLYPH_WHITE_CIRCLE,
        HOMOGLYPH_FULLWIDTH_SLASH, HOMOGLYPH_FULLWIDTH_BACKSLASH, HOMOGLYPH_ONE_DOT_LEADER,
        HOMOGLYPH_TWO_DOT_LEADER, HOMOGLYPH_ELLIPSIS, HOMOGLYPH_NBSP,
        HOMOGLYPH_ZWSP, HOMOGLYPH_WORD_JOINER, HOMOGLYPH_BOM, nullptr
    };
    
    for (int i = 0; dangerous_sequences[i]; i++) {
        if (strstr(str, dangerous_sequences[i])) {
            return true;
        }
    }
    return false;
}

/* Perform basic path safety checks */
static bool perform_basic_safety_checks(const char *str, size_t len) {
    if (len == 0) return true;
    if (len > GM_PATH_MAX_LENGTH) return false;
    if (has_control_chars(str, len)) return false;
    if (strstr(str, "..")) return false;
    if (has_encoded_traversal(str)) return false;
    return true;
}

/* Check if path is safe (no directory traversal) */
bool gm_path_is_safe(const gm_path_t *path) {
    if (!path) {
        return false;
    }

    const char *str = gm_path_str(path);
    size_t len = gm_path_len(path);

    if (!perform_basic_safety_checks(str, len)) {
        return false;
    }

    if (has_null_bytes(str, len)) {
        return false;
    }

    if (has_dangerous_homoglyphs(str)) {
        return false;
    }

    return true;
}

/* Check if path has extension */
bool gm_path_has_extension(const gm_path_t *path, const char *ext) {
    if (!path || !ext) {
        return false;
    }

    size_t len = gm_path_len(path);
    size_t ext_len = strlen(ext);

    if (ext_len >= len) {
        return false;
    }

    /* Check if ends with extension */
    return gm_string_ends_with(&path->value, ext);
}

/* Compare paths */
bool gm_path_equals(const gm_path_t *path_a, const gm_path_t *path_b) {
    if (path_a == path_b) {
        return true;
    }
    if (!path_a || !path_b) {
        return false;
    }

    /* For now, simple string comparison */
    /* Full implementation would canonicalize first */
    return gm_string_equals(&path_a->value, &path_b->value);
}

/* Check if path starts with prefix */
bool gm_path_starts_with(const gm_path_t *path, const gm_path_t *prefix) {
    if (!path || !prefix) {
        return false;
    }

    return gm_string_starts_with(&path->value, gm_path_str(prefix));
}

/* Check if path is child of parent */
bool gm_path_is_child_of(const gm_path_t *path, const gm_path_t *parent) {
    if (!path || !parent) {
        return false;
    }

    /* Must start with parent */
    if (!gm_path_starts_with(path, parent)) {
        return false;
    }

    /* And have more components */
    size_t parent_len = gm_path_len(parent);
    size_t path_len = gm_path_len(path);

    if (path_len <= parent_len) {
        return false;
    }

    /* Check for separator after parent */
    const char *path_str = gm_path_str(path);
    return path_str[parent_len] == path->separator;
}

/* Free path */
void gm_path_free(gm_path_t *path) {
    if (path) {
        gm_string_free(&path->value);
        /* Reset other fields */
        path->separator = '/';
        path->is_absolute = false;
        path->state = GM_PATH_STATE_RAW;
        path->is_validated = false;
        path->type = GM_PATH_TYPE_UNKNOWN;
    }
}