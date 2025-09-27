/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/fs/path_utils.h"

#include <stdbool.h>
#include <string.h>

#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/types.h"
#include "gitmind/security/memory.h"
#include "gitmind/util/memory.h"

static size_t trim_trailing_slashes(char *path) {
    size_t len = strlen(path);
    while (len > 1 && path[len - 1] == '/') {
        path[--len] = '\0';
    }
    return len;
}

static size_t find_last_separator(const char *path) {
    size_t len = strlen(path);
    while (len > 0) {
        if (path[len - 1] == '/') {
            return len - 1U;
        }
        --len;
    }
    return SIZE_MAX;
}

static bool needs_path_separator(const char *base_io, size_t base_len) {
    return base_len > 0 && base_io[base_len - 1] != '/';
}

GM_NODISCARD gm_result_void_t gm_fs_path_normalize_logical(const char *input,
                                                           char *output,
                                                           size_t output_size) {
    if (input == NULL || output == NULL || output_size == 0U) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "logical normalize requires buffers"));
    }

    char scratch[GM_PATH_MAX];
    if (gm_strcpy_safe(scratch, sizeof(scratch), input) != 0) {
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                    "path exceeds buffer"));
    }

    bool is_absolute = (scratch[0] == '/');
    char *segments[GM_PATH_MAX / 2];
    size_t segment_count = 0;

    char *cursor = scratch;
    while (*cursor != '\0') {
        while (*cursor == '/') {
            cursor++;
        }
        if (*cursor == '\0') {
            break;
        }

        char *start = cursor;
        while (*cursor != '\0' && *cursor != '/') {
            cursor++;
        }
        if (*cursor == '/') {
            *cursor = '\0';
            cursor++;
        }

        if (strcmp(start, ".") == 0) {
            continue;
        }

        if (strcmp(start, "..") == 0) {
            if (segment_count > 0) {
                segment_count--;
            } else if (!is_absolute) {
                segments[segment_count++] = start;
            }
            continue;
        }

        segments[segment_count++] = start;
    }

    size_t dst = 0;
    if (is_absolute) {
        output[dst++] = '/';
    }

    for (size_t idx = 0; idx < segment_count; ++idx) {
        const char *segment = segments[idx];
        size_t seg_len = strlen(segment);
        if (seg_len == 0) {
            continue;
        }

        if (dst > 0 && output[dst - 1] != '/') {
            if (dst >= output_size - 1) {
                return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                            "normalized path exceeds buffer"));
            }
            output[dst++] = '/';
        }

        if (dst + seg_len >= output_size) {
            return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                        "normalized path exceeds buffer"));
        }
        if (gm_memcpy_span(output + dst, output_size - dst, segment, seg_len) != 0) {
            return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                        "normalized path exceeds buffer"));
        }
        dst += seg_len;
    }

    if (dst == 0) {
        if (is_absolute) {
            output[0] = '/';
            dst = 1;
        } else {
            output[0] = '.';
            dst = 1;
        }
    }

    output[dst] = '\0';
    return gm_ok_void();
}

GM_NODISCARD gm_result_void_t gm_fs_path_dirname(const char *input,
                                                char *output,
                                                size_t output_size) {
    if (input == NULL || output == NULL || output_size == 0U) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "dirname requires buffers"));
    }

    char normalized[GM_PATH_MAX];
    GM_TRY(gm_fs_path_normalize_logical(input, normalized, sizeof(normalized)));

    trim_trailing_slashes(normalized);

    if (strcmp(normalized, ".") == 0) {
        if (gm_strcpy_safe(output, output_size, ".") != 0) {
            gm_memset_safe(output, output_size, 0, output_size);
            return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                        "dirname output exceeds buffer"));
        }
        return gm_ok_void();
    }

    if (strcmp(normalized, "/") == 0) {
        if (gm_strcpy_safe(output, output_size, "/") != 0) {
            gm_memset_safe(output, output_size, 0, output_size);
            return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                        "dirname output exceeds buffer"));
        }
        return gm_ok_void();
    }

    size_t slash_index = find_last_separator(normalized);
    const char *result_str = normalized;
    size_t result_len = 0U;

    if (slash_index == SIZE_MAX) {
        result_str = ".";
        result_len = 1U;
    } else if (slash_index == 0U) {
        result_str = "/";
        result_len = 1U;
    } else {
        result_len = slash_index;
    }

    if (result_str != normalized) {
        if (gm_strcpy_safe(output, output_size, result_str) != 0) {
            gm_memset_safe(output, output_size, 0, output_size);
            return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                        "dirname output exceeds buffer"));
        }
        return gm_ok_void();
    }

    if (result_len >= output_size) {
        gm_memset_safe(output, output_size, 0, output_size);
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                    "dirname output exceeds buffer"));
    }

    if (gm_memcpy_span(output, output_size, normalized, result_len) != 0) {
        gm_memset_safe(output, output_size, 0, output_size);
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                    "dirname output exceeds buffer"));
    }
    if (result_len > 1U && output[result_len - 1U] == '/') {
        result_len -= 1U;
    }
    output[result_len] = '\0';
    return gm_ok_void();
}

GM_NODISCARD gm_result_void_t gm_fs_path_basename_append(char *base_io,
                                                         size_t buffer_size,
                                                         size_t *inout_len,
                                                         const char *source_path) {
    if (base_io == NULL || inout_len == NULL || source_path == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "basename append requires parameters"));
    }

    char normalized[GM_PATH_MAX];
    GM_TRY(gm_fs_path_normalize_logical(source_path, normalized, sizeof(normalized)));

    size_t len = strlen(normalized);
    size_t idx = len;
    while (idx > 0 && normalized[idx - 1] != '/') {
        idx--;
    }

    const char *leaf = normalized + idx;
    if (*leaf == '\0') {
        return gm_ok_void();
    }

    size_t base_len = *inout_len;
    if (base_len > buffer_size) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "basename append length exceeds buffer"));
    }

    const bool need_sep = needs_path_separator(base_io, base_len);
    const size_t separator_len = need_sep ? 1U : 0U;
    size_t leaf_len = strlen(leaf);
    const size_t required = base_len + separator_len + leaf_len;

    if (required >= buffer_size ||
        leaf_len > buffer_size - base_len - separator_len - 1U) {
        gm_memset_safe(base_io, buffer_size, 0, buffer_size);
        *inout_len = 0U;
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                    "basename append exceeds buffer"));
    }

    size_t cursor = base_len;
    if (need_sep) {
        base_io[cursor++] = '/';
    }
    if (gm_memcpy_span(base_io + cursor, buffer_size - cursor, leaf, leaf_len) !=
        0) {
        gm_memset_safe(base_io, buffer_size, 0, buffer_size);
        *inout_len = 0U;
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                    "basename append exceeds buffer"));
    }
    cursor += leaf_len;
    base_io[cursor] = '\0';
    *inout_len = cursor;
    return gm_ok_void();
}
