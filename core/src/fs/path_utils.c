/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/fs/path_utils.h"

#include <string.h>

#include "gitmind/error.h"
#include "gitmind/types.h"
#include "gitmind/util/memory.h"

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
        memcpy(output + dst, segment, seg_len);
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

    size_t len = strlen(normalized);
    if (len == 0) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "dirname requires non-empty path"));
    }

    size_t end = len;
    while (end > 1 && normalized[end - 1] == '/') {
        normalized[--end] = '\0';
    }

    if (strcmp(normalized, ".") == 0) {
        if (gm_strcpy_safe(output, output_size, ".") != 0) {
            return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                        "dirname output exceeds buffer"));
        }
        return gm_ok_void();
    }

    size_t idx = end;
    while (idx > 0 && normalized[idx - 1] != '/') {
        idx--;
    }

    if (idx == 0) {
        if (gm_strcpy_safe(output, output_size, ".") != 0) {
            return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                        "dirname output exceeds buffer"));
        }
        return gm_ok_void();
    }

    size_t out_len = idx;
    if (out_len >= output_size) {
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                    "dirname output exceeds buffer"));
    }

    memcpy(output, normalized, out_len);
    if (out_len > 1 && output[out_len - 1] == '/') {
        out_len--;
    }
    output[out_len] = '\0';
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
    if (base_len > 0 && base_io[base_len - 1] != '/') {
        if (base_len + 1 >= buffer_size) {
            return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                        "basename append exceeds buffer"));
        }
        base_io[base_len++] = '/';
    }

    size_t leaf_len = strlen(leaf);
    if (base_len + leaf_len >= buffer_size) {
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                    "basename append exceeds buffer"));
    }

    memcpy(base_io + base_len, leaf, leaf_len);
    base_len += leaf_len;
    base_io[base_len] = '\0';
    *inout_len = base_len;
    return gm_ok_void();
}
