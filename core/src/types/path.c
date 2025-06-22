/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/types/path.h"
#include "gitmind/error.h"
#include <string.h>
#include <stdlib.h>

/* Path constants */
#define PATH_SEP_UNIX '/'
#define PATH_SEP_WIN '\\'
#define PATH_CURRENT_DIR "."
#define PATH_PARENT_DIR ".."
#define PATH_MAX_COMPONENTS 255
#define PATH_MAX_LENGTH 4096
#define PATH_ENCODED_PARENT_1 "%2e%2e"  /* URL encoded .. */
#define PATH_ENCODED_PARENT_2 "%2E%2E"  /* URL encoded .. */
#define PATH_ENCODED_DOT_1 "%2e"       /* URL encoded . */
#define PATH_ENCODED_DOT_2 "%2E"       /* URL encoded . */

/* Symlink pattern constants */
#define SYMLINK_EXT_WINDOWS ".lnk"
#define SYMLINK_EXT_LEN 4
#define SYMLINK_SUFFIX_MACOS '@'
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
#define ERR_MSG_NULL_PATH "NULL path"
#define ERR_MSG_NULL_BASE "NULL base path"
#define ERR_MSG_NULL_RELATIVE "NULL relative path"
#define ERR_MSG_ABS_REL_MISMATCH "Cannot make relative between absolute and relative paths"
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
#define HOMOGLYPH_FRACTION_SLASH "\xe2\x81\x84"     /* U+2044 FRACTION SLASH */
#define HOMOGLYPH_DIVISION_SLASH "\xe2\x88\x95"     /* U+2215 DIVISION SLASH */
#define HOMOGLYPH_WHITE_CIRCLE "\xe2\x9f\x8b"       /* U+27CB WHITE CIRCLE */
#define HOMOGLYPH_FULLWIDTH_SLASH "\xef\xbc\x8f"    /* U+FF0F FULLWIDTH SOLIDUS */
#define HOMOGLYPH_FULLWIDTH_BACKSLASH "\xef\xbc\xbc" /* U+FF3C FULLWIDTH REVERSE SOLIDUS */
#define HOMOGLYPH_ONE_DOT_LEADER "\xe2\x80\xa4"     /* U+2024 ONE DOT LEADER */
#define HOMOGLYPH_TWO_DOT_LEADER "\xe2\x80\xa5"     /* U+2025 TWO DOT LEADER */
#define HOMOGLYPH_ELLIPSIS "\xe2\x80\xa6"           /* U+2026 HORIZONTAL ELLIPSIS */
#define HOMOGLYPH_NBSP "\xc2\xa0"                   /* U+00A0 NO-BREAK SPACE */
#define HOMOGLYPH_ZWSP "\xe2\x80\x8b"               /* U+200B ZERO WIDTH SPACE */
#define HOMOGLYPH_WORD_JOINER "\xe2\x81\xa0"        /* U+2060 WORD JOINER */
#define HOMOGLYPH_BOM "\xef\xbb\xbf"                /* U+FEFF ZERO WIDTH NO-BREAK SPACE */

/* Helper to create error result for path */
static inline gm_result_path gm_err_path(gm_error_t* e) {
    return (gm_result_path){ .ok = false, .u.err = e };
}

/* Helper to create success result for path */
static inline gm_result_path gm_ok_path(gm_path_t p) {
    return (gm_result_path){ .ok = true, .u.val = p };
}

/* Detect path separator */
static char detect_separator(const char* str) {
    /* Look for first separator */
    const char* unix_sep = strchr(str, PATH_SEP_UNIX);
    const char* win_sep = strchr(str, PATH_SEP_WIN);
    
    if (unix_sep && !win_sep) return PATH_SEP_UNIX;
    if (win_sep && !unix_sep) return PATH_SEP_WIN;
    if (unix_sep && win_sep) {
        /* Use whichever comes first */
        return (unix_sep < win_sep) ? PATH_SEP_UNIX : PATH_SEP_WIN;
    }
    
    /* Default to system separator */
#ifdef _WIN32
    return PATH_SEP_WIN;
#else
    return PATH_SEP_UNIX;
#endif
}

/* Check if path is absolute */
static bool is_absolute_path(const char* str, char separator) {
    if (!str || !*str) return false;
    
    if (separator == PATH_SEP_UNIX) {
        return str[0] == PATH_SEP_UNIX;
    } else {
        /* Windows: C:\ or \\server\share */
        if (str[0] == PATH_SEP_WIN && str[1] == PATH_SEP_WIN) {
            return true;  /* UNC path */
        }
        if (str[1] == ':' && str[2] == PATH_SEP_WIN) {
            return true;  /* Drive letter */
        }
    }
    return false;
}

/* Basic path validation (no traversal check yet) */
static bool validate_path_basic(const char* str) {
    if (!str) return false;
    
    size_t len = strlen(str);
    if (len == 0) return true;  /* Empty path is valid */
    
    /* Check for null bytes */
    for (size_t i = 0; i < len; i++) {
        if (str[i] == '\0') return false;
    }
    
    /* More validation can be added here */
    return true;
}

/* Create new path from string */
gm_result_path gm_path_new(const char* str) {
    if (!str) {
        str = "";  /* Treat NULL as empty path */
    }
    
    /* Basic validation */
    if (!validate_path_basic(str)) {
        return gm_err_path(GM_ERROR(GM_ERR_INVALID_PATH, 
                                   ERR_MSG_INVALID_FORMAT));
    }
    
    /* Create string copy */
    gm_result_string str_result = gm_string_new(str);
    if (GM_IS_ERR(str_result)) {
        return gm_err_path(GM_UNWRAP_ERR(str_result));
    }
    
    gm_path_t path;
    path.value = GM_UNWRAP(str_result);
    path.separator = detect_separator(str);
    path.is_absolute = is_absolute_path(str, path.separator);
    path.state = GM_PATH_STATE_RAW;
    path.is_validated = true;  /* Basic validation passed */
    path.type = GM_PATH_TYPE_UNKNOWN;
    
    return gm_ok_path(path);
}

/* Create path from existing string (takes ownership) */
gm_result_path gm_path_from_string(gm_string_t str) {
    const char* data = gm_string_data(&str);
    
    /* Basic validation */
    if (!validate_path_basic(data)) {
        gm_string_free(&str);
        return gm_err_path(GM_ERROR(GM_ERR_INVALID_PATH, 
                                   ERR_MSG_INVALID_FORMAT));
    }
    
    gm_path_t path;
    path.value = str;  /* Take ownership */
    path.separator = detect_separator(data);
    path.is_absolute = is_absolute_path(data, path.separator);
    path.state = GM_PATH_STATE_RAW;
    path.is_validated = true;
    path.type = GM_PATH_TYPE_UNKNOWN;
    
    return gm_ok_path(path);
}

/* Join two paths */
gm_result_path gm_path_join(const gm_path_t* base, const gm_path_t* relative) {
    if (!base) {
        return gm_err_path(GM_ERROR(GM_ERR_INVALID_ARGUMENT, ERR_MSG_NULL_BASE));
    }
    if (!relative) {
        return gm_err_path(GM_ERROR(GM_ERR_INVALID_ARGUMENT, ERR_MSG_NULL_RELATIVE));
    }
    
    /* If relative is absolute, return copy of it */
    if (relative->is_absolute) {
        return gm_path_new(gm_path_str(relative));
    }
    
    /* If base is empty, return copy of relative */
    if (gm_path_is_empty(base)) {
        return gm_path_new(gm_path_str(relative));
    }
    
    /* If relative is empty, return copy of base */
    if (gm_path_is_empty(relative)) {
        return gm_path_new(gm_path_str(base));
    }
    
    /* Join with separator */
    const char* base_str = gm_path_str(base);
    size_t base_len = gm_path_len(base);
    bool needs_sep = (base_len > 0 && 
                     base_str[base_len - 1] != base->separator);
    
    /* Create joined string */
    gm_result_string result;
    if (needs_sep) {
        /* Need to add separator */
        char sep_str[2] = { base->separator, '\0' };
        gm_result_string with_sep = gm_string_concat(&base->value, 
                                                     &(gm_string_t){
                                                         .data = sep_str,
                                                         .length = 1,
                                                         .capacity = 2
                                                     });
        if (GM_IS_ERR(with_sep)) {
            return gm_err_path(GM_UNWRAP_ERR(with_sep));
        }
        gm_string_t temp = GM_UNWRAP(with_sep);
        result = gm_string_concat(&temp, &relative->value);
        gm_string_free(&temp);
    } else {
        result = gm_string_concat(&base->value, &relative->value);
    }
    
    if (GM_IS_ERR(result)) {
        return gm_err_path(GM_UNWRAP_ERR(result));
    }
    
    return gm_path_from_string(GM_UNWRAP(result));
}

/* Extract directory name */
gm_result_path gm_path_dirname(const gm_path_t* path) {
    if (!path) {
        return gm_err_path(GM_ERROR(GM_ERR_INVALID_ARGUMENT, ERR_MSG_NULL_PATH));
    }
    
    const char* str = gm_path_str(path);
    size_t len = gm_path_len(path);
    
    /* Empty path returns "." */
    if (len == 0) {
        return gm_path_new(PATH_CURRENT_DIR);
    }
    
    /* Find last separator */
    size_t last_sep = len;
    for (size_t i = len; i > 0; i--) {
        if (str[i - 1] == path->separator) {
            last_sep = i - 1;
            break;
        }
    }
    
    /* No separator found */
    if (last_sep == len) {
        return gm_path_new(PATH_CURRENT_DIR);
    }
    
    /* Root directory special case */
    if (last_sep == 0 && path->is_absolute) {
        char root[2] = { path->separator, '\0' };
        return gm_path_new(root);
    }
    
    /* Extract directory part */
    gm_result_string dir_result = gm_string_substring(&path->value, 0, last_sep);
    if (GM_IS_ERR(dir_result)) {
        return gm_err_path(GM_UNWRAP_ERR(dir_result));
    }
    
    return gm_path_from_string(GM_UNWRAP(dir_result));
}

/* Extract base name */
gm_result_path gm_path_basename(const gm_path_t* path) {
    if (!path) {
        return gm_err_path(GM_ERROR(GM_ERR_INVALID_ARGUMENT, ERR_MSG_NULL_PATH));
    }
    
    const char* str = gm_path_str(path);
    size_t len = gm_path_len(path);
    
    /* Empty path returns empty */
    if (len == 0) {
        return gm_path_new("");
    }
    
    /* Find last separator */
    size_t start = 0;
    for (size_t i = len; i > 0; i--) {
        if (str[i - 1] == path->separator) {
            start = i;
            break;
        }
    }
    
    /* Extract base name */
    gm_result_string base_result = gm_string_substring(&path->value, start, len - start);
    if (GM_IS_ERR(base_result)) {
        return gm_err_path(GM_UNWRAP_ERR(base_result));
    }
    
    return gm_path_from_string(GM_UNWRAP(base_result));
}

/* Split path into components */
static gm_result_void split_path_components(const char* str, char sep, 
                                           char*** components, size_t* count) {
    *components = NULL;
    *count = 0;
    
    if (!str || !*str) {
        return gm_ok_void();
    }
    
    /* Count components */
    size_t n = 1;
    for (const char* p = str; *p; p++) {
        if (*p == sep) n++;
    }
    
    /* Allocate component array */
    char** comps = calloc(n + 1, sizeof(char*));
    if (!comps) {
        return gm_err_void(GM_ERROR(GM_ERR_OUT_OF_MEMORY, 
                                   ERR_MSG_ALLOC_COMPONENTS));
    }
    
    /* Split into components */
    size_t idx = 0;
    const char* start = str;
    const char* p = str;
    
    while (*p) {
        if (*p == sep) {
            size_t len = p - start;
            if (len > 0 || idx == 0) {  /* Keep leading empty for absolute paths */
                comps[idx] = malloc(len + 1);
                if (!comps[idx]) {
                    /* Clean up on failure */
                    for (size_t i = 0; i < idx; i++) {
                        free(comps[i]);
                    }
                    free(comps);
                    return gm_err_void(GM_ERROR(GM_ERR_OUT_OF_MEMORY, 
                                               ERR_MSG_ALLOC_COMPONENT));
                }
                memcpy(comps[idx], start, len);
                comps[idx][len] = '\0';
                idx++;
            }
            start = p + 1;
        }
        p++;
    }
    
    /* Handle last component */
    size_t len = p - start;
    if (len > 0) {
        comps[idx] = malloc(len + 1);
        if (!comps[idx]) {
            for (size_t i = 0; i < idx; i++) {
                free(comps[i]);
            }
            free(comps);
            return gm_err_void(GM_ERROR(GM_ERR_OUT_OF_MEMORY, 
                                       ERR_MSG_ALLOC_COMPONENT));
        }
        memcpy(comps[idx], start, len);
        comps[idx][len] = '\0';
        idx++;
    }
    
    *components = comps;
    *count = idx;
    return gm_ok_void();
}

/* Free path components */
static void free_components(char** components, size_t count) {
    if (components) {
        for (size_t i = 0; i < count; i++) {
            free(components[i]);
        }
        free(components);
    }
}

/* Canonicalize path (remove . and .. components) */
gm_result_path gm_path_canonicalize(const gm_path_t* path) {
    if (!path) {
        return gm_err_path(GM_ERROR(GM_ERR_INVALID_ARGUMENT, ERR_MSG_NULL_PATH));
    }
    
    const char* str = gm_path_str(path);
    
    /* Split into components */
    char** components = NULL;
    size_t comp_count = 0;
    gm_result_void split_result = split_path_components(str, path->separator, 
                                                       &components, &comp_count);
    if (GM_IS_ERR(split_result)) {
        return gm_err_path(GM_UNWRAP_ERR(split_result));
    }
    
    /* Process components to resolve . and .. */
    char** canonical = calloc(comp_count + 1, sizeof(char*));
    if (!canonical) {
        free_components(components, comp_count);
        return gm_err_path(GM_ERROR(GM_ERR_OUT_OF_MEMORY, 
                                   ERR_MSG_ALLOC_CANONICAL));
    }
    
    size_t out_idx = 0;
    
    for (size_t i = 0; i < comp_count; i++) {
        const char* comp = components[i];
        
        /* Skip . components */
        if (strcmp(comp, PATH_CURRENT_DIR) == 0) {
            continue;
        }
        
        /* Handle .. components */
        if (strcmp(comp, PATH_PARENT_DIR) == 0) {
            if (out_idx > 0 && strcmp(canonical[out_idx - 1], "") != 0) {
                /* Go up one level (but not past root) */
                out_idx--;
            } else if (!path->is_absolute) {
                /* Keep .. for relative paths that go above start */
                canonical[out_idx++] = components[i];
                components[i] = NULL;  /* Transfer ownership */
            }
            /* For absolute paths, .. at root is ignored */
            continue;
        }
        
        /* Normal component */
        canonical[out_idx++] = components[i];
        components[i] = NULL;  /* Transfer ownership */
    }
    
    /* Build result string */
    gm_result_string result_str;
    
    if (out_idx == 0) {
        /* Empty result */
        if (path->is_absolute) {
            /* Absolute path reduced to root */
            char root[2] = { path->separator, '\0' };
            result_str = gm_string_new(root);
        } else {
            /* Relative path reduced to current dir */
            result_str = gm_string_new(PATH_CURRENT_DIR);
        }
    } else {
        /* Join components */
        result_str = gm_string_new("");
        if (GM_IS_ERR(result_str)) {
            free_components((char**)canonical, out_idx);
            free_components(components, comp_count);
            return gm_err_path(GM_UNWRAP_ERR(result_str));
        }
        
        gm_string_t joined = GM_UNWRAP(result_str);
        
        for (size_t i = 0; i < out_idx; i++) {
            if (i > 0 || (path->is_absolute && strlen(canonical[0]) > 0)) {
                /* Add separator except before empty first component of absolute path */
                char sep_str[2] = { path->separator, '\0' };
                gm_result_void append_result = gm_string_append(&joined, sep_str);
                if (GM_IS_ERR(append_result)) {
                    gm_string_free(&joined);
                    free_components((char**)canonical, out_idx);
                    free_components(components, comp_count);
                    return gm_err_path(GM_UNWRAP_ERR(append_result));
                }
            }
            
            gm_result_void append_result = gm_string_append(&joined, canonical[i]);
            if (GM_IS_ERR(append_result)) {
                gm_string_free(&joined);
                free_components((char**)canonical, out_idx);
                free_components(components, comp_count);
                return gm_err_path(GM_UNWRAP_ERR(append_result));
            }
        }
        
        result_str = (gm_result_string){ .ok = true, .u.val = joined };
    }
    
    /* Clean up */
    free_components((char**)canonical, out_idx);
    free_components(components, comp_count);
    
    if (GM_IS_ERR(result_str)) {
        return gm_err_path(GM_UNWRAP_ERR(result_str));
    }
    
    /* Create canonicalized path */
    gm_result_path final_result = gm_path_from_string(GM_UNWRAP(result_str));
    if (GM_IS_OK(final_result)) {
        gm_path_t canonical_path = GM_UNWRAP(final_result);
        canonical_path.state = GM_PATH_STATE_CANONICAL;
        return gm_ok_path(canonical_path);
    }
    
    return final_result;
}

/* Make path relative to base */
gm_result_path gm_path_make_relative(const gm_path_t* path, const gm_path_t* base) {
    if (!path || !base) {
        return gm_err_path(GM_ERROR(GM_ERR_INVALID_ARGUMENT, ERR_MSG_NULL_PATH));
    }
    
    /* Both paths must be either absolute or relative */
    if (path->is_absolute != base->is_absolute) {
        return gm_err_path(GM_ERROR(GM_ERR_INVALID_ARGUMENT, 
                                   ERR_MSG_ABS_REL_MISMATCH));
    }
    
    /* Canonicalize both paths first */
    gm_result_path canon_path = gm_path_canonicalize(path);
    if (GM_IS_ERR(canon_path)) {
        return canon_path;
    }
    gm_path_t canonical_path = GM_UNWRAP(canon_path);
    
    gm_result_path canon_base = gm_path_canonicalize(base);
    if (GM_IS_ERR(canon_base)) {
        gm_path_free(&canonical_path);
        return canon_base;
    }
    gm_path_t canonical_base = GM_UNWRAP(canon_base);
    
    /* Split both into components */
    char** path_comps = NULL;
    size_t path_count = 0;
    gm_result_void split1 = split_path_components(gm_path_str(&canonical_path), 
                                                  path->separator, 
                                                  &path_comps, &path_count);
    if (GM_IS_ERR(split1)) {
        gm_path_free(&canonical_path);
        gm_path_free(&canonical_base);
        return gm_err_path(GM_UNWRAP_ERR(split1));
    }
    
    char** base_comps = NULL;
    size_t base_count = 0;
    gm_result_void split2 = split_path_components(gm_path_str(&canonical_base), 
                                                  base->separator, 
                                                  &base_comps, &base_count);
    if (GM_IS_ERR(split2)) {
        free_components(path_comps, path_count);
        gm_path_free(&canonical_path);
        gm_path_free(&canonical_base);
        return gm_err_path(GM_UNWRAP_ERR(split2));
    }
    
    /* Find common prefix */
    size_t common = 0;
    while (common < path_count && common < base_count &&
           strcmp(path_comps[common], base_comps[common]) == 0) {
        common++;
    }
    
    /* Build relative path */
    gm_result_string result = gm_string_new("");
    if (GM_IS_ERR(result)) {
        free_components(path_comps, path_count);
        free_components(base_comps, base_count);
        gm_path_free(&canonical_path);
        gm_path_free(&canonical_base);
        return gm_err_path(GM_UNWRAP_ERR(result));
    }
    
    gm_string_t rel_str = GM_UNWRAP(result);
    
    /* Add .. for each remaining base component */
    for (size_t i = common; i < base_count; i++) {
        if (gm_string_len(&rel_str) > 0) {
            char sep_str[2] = { path->separator, '\0' };
            gm_result_void append_res = gm_string_append(&rel_str, sep_str);
            if (GM_IS_ERR(append_res)) {
                gm_string_free(&rel_str);
                free_components(path_comps, path_count);
                free_components(base_comps, base_count);
                gm_path_free(&canonical_path);
                gm_path_free(&canonical_base);
                return gm_err_path(GM_UNWRAP_ERR(append_res));
            }
        }
        
        gm_result_void append_res = gm_string_append(&rel_str, PATH_PARENT_DIR);
        if (GM_IS_ERR(append_res)) {
            gm_string_free(&rel_str);
            free_components(path_comps, path_count);
            free_components(base_comps, base_count);
            gm_path_free(&canonical_path);
            gm_path_free(&canonical_base);
            return gm_err_path(GM_UNWRAP_ERR(append_res));
        }
    }
    
    /* Add remaining path components */
    for (size_t i = common; i < path_count; i++) {
        if (gm_string_len(&rel_str) > 0) {
            char sep_str[2] = { path->separator, '\0' };
            gm_result_void append_res = gm_string_append(&rel_str, sep_str);
            if (GM_IS_ERR(append_res)) {
                gm_string_free(&rel_str);
                free_components(path_comps, path_count);
                free_components(base_comps, base_count);
                gm_path_free(&canonical_path);
                gm_path_free(&canonical_base);
                return gm_err_path(GM_UNWRAP_ERR(append_res));
            }
        }
        
        gm_result_void append_res = gm_string_append(&rel_str, path_comps[i]);
        if (GM_IS_ERR(append_res)) {
            gm_string_free(&rel_str);
            free_components(path_comps, path_count);
            free_components(base_comps, base_count);
            gm_path_free(&canonical_path);
            gm_path_free(&canonical_base);
            return gm_err_path(GM_UNWRAP_ERR(append_res));
        }
    }
    
    /* Handle empty result */
    if (gm_string_len(&rel_str) == 0) {
        gm_string_free(&rel_str);
        result = gm_string_new(PATH_CURRENT_DIR);
        if (GM_IS_ERR(result)) {
            free_components(path_comps, path_count);
            free_components(base_comps, base_count);
            gm_path_free(&canonical_path);
            gm_path_free(&canonical_base);
            return gm_err_path(GM_UNWRAP_ERR(result));
        }
        rel_str = GM_UNWRAP(result);
    }
    
    /* Clean up */
    free_components(path_comps, path_count);
    free_components(base_comps, base_count);
    gm_path_free(&canonical_path);
    gm_path_free(&canonical_base);
    
    /* Create relative path */
    gm_result_path rel_path = gm_path_from_string(rel_str);
    if (GM_IS_OK(rel_path)) {
        gm_path_t relative = GM_UNWRAP(rel_path);
        relative.state = GM_PATH_STATE_RELATIVE;
        return gm_ok_path(relative);
    }
    
    return rel_path;
}

/* Default path validation rules */
static const struct {
    size_t max_length;
    bool allow_traversal;
    bool allow_absolute;
    bool allow_relative;
    bool allow_symlinks;
    bool allow_hidden;
} DEFAULT_PATH_RULES = {
    .max_length = PATH_MAX_LENGTH,
    .allow_traversal = false,
    .allow_absolute = true,
    .allow_relative = true,
    .allow_symlinks = false,
    .allow_hidden = true
};

/* Validate path with rules */
gm_result_void gm_path_validate(const gm_path_t* path, const gm_path_rules_t* rules) {
    if (!path) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, ERR_MSG_NULL_PATH));
    }
    
    const char* str = gm_path_str(path);
    size_t len = gm_path_len(path);
    
    /* Use default rules if none provided */
    /* For now, we use hardcoded defaults since gm_path_rules_t is not yet defined */
    size_t max_length = DEFAULT_PATH_RULES.max_length;
    bool allow_traversal = DEFAULT_PATH_RULES.allow_traversal;
    bool allow_absolute = DEFAULT_PATH_RULES.allow_absolute;
    bool allow_relative = DEFAULT_PATH_RULES.allow_relative;
    
    /* Suppress unused parameter warning */
    (void)rules;
    
    /* Check length */
    if (len > max_length) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_PATH, 
                                   ERR_MSG_PATH_TOO_LONG, max_length));
    }
    
    /* Check absolute/relative constraints */
    if (path->is_absolute && !allow_absolute) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_PATH, 
                                   ERR_MSG_ABS_NOT_ALLOWED));
    }
    
    if (!path->is_absolute && !allow_relative) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_PATH, 
                                   ERR_MSG_REL_NOT_ALLOWED));
    }
    
    /* Check traversal */
    if (!allow_traversal && strstr(str, "..")) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_PATH, 
                                   ERR_MSG_TRAVERSAL));
    }
    
    /* Check safety (more comprehensive than just traversal) */
    if (!gm_path_is_safe(path)) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_PATH, 
                                   ERR_MSG_UNSAFE));
    }
    
    /* Check for hidden files if not allowed */
    if (!DEFAULT_PATH_RULES.allow_hidden) {
        /* Check each component for hidden files */
        char** components = NULL;
        size_t comp_count = 0;
        gm_result_void split_res = split_path_components(str, path->separator, 
                                                        &components, &comp_count);
        if (GM_IS_OK(split_res)) {
            for (size_t i = 0; i < comp_count; i++) {
                if (components[i][0] == '.' && components[i][1] != '\0' &&
                    strcmp(components[i], PATH_CURRENT_DIR) != 0 &&
                    strcmp(components[i], PATH_PARENT_DIR) != 0) {
                    free_components(components, comp_count);
                    return gm_err_void(GM_ERROR(GM_ERR_INVALID_PATH, 
                                               ERR_MSG_HIDDEN_FILES));
                }
            }
            free_components(components, comp_count);
        }
    }
    
    /* Check for symlink-like patterns if not allowed */
    if (!DEFAULT_PATH_RULES.allow_symlinks) {
        /* While we can't resolve symlinks without filesystem access,
         * we can detect common symlink patterns and flag them */
        
        /* Check for symlink indicators in path components */
        char** components = NULL;
        size_t comp_count = 0;
        gm_result_void split_res = split_path_components(str, path->separator, 
                                                        &components, &comp_count);
        if (GM_IS_OK(split_res)) {
            for (size_t i = 0; i < comp_count; i++) {
                /* Check for common symlink naming patterns */
                const char* comp = components[i];
                size_t comp_len = strlen(comp);
                
                /* Check for .lnk extension (Windows) */
                if (comp_len > SYMLINK_EXT_LEN && strcmp(comp + comp_len - SYMLINK_EXT_LEN, SYMLINK_EXT_WINDOWS) == 0) {
                    free_components(components, comp_count);
                    return gm_err_void(GM_ERROR(GM_ERR_INVALID_PATH, 
                                               ERR_MSG_SYMLINK_LNK));
                }
                
                /* Check for @ suffix (macOS Finder symlink indicator) */
                if (comp_len > 1 && comp[comp_len - 1] == SYMLINK_SUFFIX_MACOS) {
                    free_components(components, comp_count);
                    return gm_err_void(GM_ERROR(GM_ERR_INVALID_PATH, 
                                               ERR_MSG_SYMLINK_AT));
                }
                
                /* Check for -> arrow pattern (common in ls -la output) */
                if (strstr(comp, SYMLINK_ARROW_PATTERN)) {
                    free_components(components, comp_count);
                    return gm_err_void(GM_ERROR(GM_ERR_INVALID_PATH, 
                                               ERR_MSG_SYMLINK_ARROW));
                }
            }
            free_components(components, comp_count);
        }
        
        /* Check for full path symlink patterns */
        if (strstr(str, SYMLINK_ARROW_SPACED)) {
            return gm_err_void(GM_ERROR(GM_ERR_INVALID_PATH, 
                                       ERR_MSG_SYMLINK_GENERIC));
        }
    }
    
    /* Note: Full symlink resolution would require filesystem access.
     * Allowed prefixes/roots and disallowed patterns would be specified 
     * in gm_path_rules_t when it's defined. */
    
    return gm_ok_void();
}

/* Check if path contains control characters */
static bool has_control_chars(const char* str, size_t len) {
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)str[i];
        if (c < 0x20 || c == 0x7F) {  /* Control chars */
            return true;
        }
    }
    return false;
}

/* Check if path contains encoded traversal sequences */
static bool has_encoded_traversal(const char* str) {
    /* Check for URL encoded .. */
    if (strstr(str, PATH_ENCODED_PARENT_1) || 
        strstr(str, PATH_ENCODED_PARENT_2)) {
        return true;
    }
    
    /* Check for other encoding tricks */
    const char* patterns[] = {
        ENCODED_PARENT_HEX_SLASH,
        ENCODED_PARENT_HEX_BACKSLASH,
        ENCODED_PARENT_OCT_SLASH,
        ENCODED_PARENT_OCT_BACKSLASH,
        ENCODED_UNICODE_DOTS,
        NULL
    };
    
    for (int i = 0; patterns[i]; i++) {
        if (strstr(str, patterns[i])) {
            return true;
        }
    }
    
    return false;
}

/* Check if path is safe (no directory traversal) */
bool gm_path_is_safe(const gm_path_t* path) {
    if (!path) return false;
    
    const char* str = gm_path_str(path);
    size_t len = gm_path_len(path);
    
    /* Empty path is safe */
    if (len == 0) return true;
    
    /* Check length limit */
    if (len > PATH_MAX_LENGTH) {
        return false;
    }
    
    /* Check for control characters */
    if (has_control_chars(str, len)) {
        return false;
    }
    
    /* Check for .. sequences */
    if (strstr(str, "..")) {
        return false;
    }
    
    /* Check for encoded traversal */
    if (has_encoded_traversal(str)) {
        return false;
    }
    
    /* Check for null bytes (defense in depth) */
    for (size_t i = 0; i < len; i++) {
        if (str[i] == '\0') {
            return false;
        }
    }
    
    /* Check for common homoglyphs that could be used to trick users */
    /* This is a basic check for the most dangerous homoglyphs in paths */
    const char* dangerous_sequences[] = {
        HOMOGLYPH_FRACTION_SLASH,     /* looks like / */
        HOMOGLYPH_DIVISION_SLASH,     /* looks like / */
        HOMOGLYPH_WHITE_CIRCLE,       /* looks like O */
        HOMOGLYPH_FULLWIDTH_SLASH,    /* looks like / */
        HOMOGLYPH_FULLWIDTH_BACKSLASH, /* looks like \ */
        HOMOGLYPH_ONE_DOT_LEADER,     /* looks like . */
        HOMOGLYPH_TWO_DOT_LEADER,     /* looks like .. */
        HOMOGLYPH_ELLIPSIS,           /* looks like ... */
        HOMOGLYPH_NBSP,               /* non-breaking space */
        HOMOGLYPH_ZWSP,               /* zero width space */
        HOMOGLYPH_WORD_JOINER,        /* word joiner */
        HOMOGLYPH_BOM,                /* byte order mark */
        NULL
    };
    
    for (int i = 0; dangerous_sequences[i]; i++) {
        if (strstr(str, dangerous_sequences[i])) {
            return false;
        }
    }
    
    return true;
}

/* Check if path has extension */
bool gm_path_has_extension(const gm_path_t* path, const char* ext) {
    if (!path || !ext) return false;
    
    size_t len = gm_path_len(path);
    size_t ext_len = strlen(ext);
    
    if (ext_len >= len) return false;
    
    /* Check if ends with extension */
    return gm_string_ends_with(&path->value, ext);
}

/* Compare paths */
bool gm_path_equals(const gm_path_t* a, const gm_path_t* b) {
    if (a == b) return true;
    if (!a || !b) return false;
    
    /* For now, simple string comparison */
    /* Full implementation would canonicalize first */
    return gm_string_equals(&a->value, &b->value);
}

/* Check if path starts with prefix */
bool gm_path_starts_with(const gm_path_t* path, const gm_path_t* prefix) {
    if (!path || !prefix) return false;
    
    return gm_string_starts_with(&path->value, gm_path_str(prefix));
}

/* Check if path is child of parent */
bool gm_path_is_child_of(const gm_path_t* path, const gm_path_t* parent) {
    if (!path || !parent) return false;
    
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
    const char* path_str = gm_path_str(path);
    return path_str[parent_len] == path->separator;
}

/* Free path */
void gm_path_free(gm_path_t* path) {
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