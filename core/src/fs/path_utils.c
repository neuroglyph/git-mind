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

#define MAX_PATH_SEGMENTS (GM_PATH_MAX / 2)

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

static char *next_segment(char **cursor) {
    if (cursor == NULL || *cursor == NULL) {
        return NULL;
    }

    while (**cursor == '/') {
        (*cursor)++;
    }
    if (**cursor == '\0') {
        return NULL;
    }

    char *start = *cursor;
    while (**cursor != '\0' && **cursor != '/') {
        (*cursor)++;
    }
    if (**cursor == '/') {
        **cursor = '\0';
        (*cursor)++;
    }
    return start;
}

static gm_result_void_t push_segment(char **segments, size_t *count,
                                     size_t max_segments, char *segment) {
    if (*count >= max_segments) {
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                    "too many path segments"));
    }
    segments[*count] = segment;
    (*count)++;
    return gm_ok_void();
}

static gm_result_void_t handle_segment(char *segment, bool is_absolute,
                                       char **segments, size_t *count,
                                       size_t max_segments) {
    if (strcmp(segment, ".") == 0) {
        return gm_ok_void();
    }

    if (strcmp(segment, "..") == 0) {
        if (*count > 0) {
            (*count)--;
            return gm_ok_void();
        }
        if (!is_absolute) {
            return push_segment(segments, count, max_segments, segment);
        }
        return gm_ok_void();
    }

    return push_segment(segments, count, max_segments, segment);
}

static const char *basename_from_normalized(const char *normalized,
                                            size_t *leaf_len) {
    size_t len = strlen(normalized);
    size_t idx = len;
    while (idx > 0 && normalized[idx - 1] != '/') {
        idx--;
    }

    *leaf_len = len - idx;
    return normalized + idx;
}

static gm_result_void_t select_dirname_result(const char *normalized,
                                              const char **out_str,
                                              size_t *out_len) {
    if (strcmp(normalized, ".") == 0) {
        *out_str = ".";
        *out_len = 1U;
        return gm_ok_void();
    }

    if (strcmp(normalized, "/") == 0) {
        *out_str = "/";
        *out_len = 1U;
        return gm_ok_void();
    }

    size_t slash_index = find_last_separator(normalized);
    if (slash_index == SIZE_MAX) {
        *out_str = ".";
        *out_len = 1U;
        return gm_ok_void();
    }
    if (slash_index == 0U) {
        *out_str = "/";
        *out_len = 1U;
        return gm_ok_void();
    }

    *out_str = normalized;
    *out_len = slash_index;
    return gm_ok_void();
}

static gm_result_void_t copy_dirname_result(const char *result_str,
                                            size_t result_len,
                                            const char *normalized,
                                            char *output, size_t output_size) {
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

static gm_result_void_t parse_segments(char *scratch, bool is_absolute,
                                       char **segments, size_t *segment_count,
                                       size_t max_segments) {
    char *cursor = scratch;
    char *segment = NULL;
    size_t count = 0U;

    while ((segment = next_segment(&cursor)) != NULL) {
        gm_result_void_t segment_result =
            handle_segment(segment, is_absolute, segments, &count,
                           max_segments);
        if (!segment_result.ok) {
            *segment_count = count;
            return segment_result;
        }
    }

    *segment_count = count;
    return gm_ok_void();
}

static gm_result_void_t append_to_path(char *output, size_t output_size,
                                       size_t *cursor, bool add_separator,
                                       const char *segment_value,
                                       size_t seg_len) {
    size_t required = seg_len;
    if (add_separator) {
        required += 1U;
    }
    if (*cursor + required >= output_size) {
        gm_memset_safe(output, output_size, 0, output_size);
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                    "normalized path exceeds buffer"));
    }

    if (add_separator) {
        output[(*cursor)++] = '/';
    }

    if (seg_len == 0U) {
        return gm_ok_void();
    }

    if (gm_memcpy_span(output + *cursor, output_size - *cursor, segment_value,
                       seg_len) != 0) {
        gm_memset_safe(output, output_size, 0, output_size);
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                    "normalized path exceeds buffer"));
    }

    *cursor += seg_len;
    return gm_ok_void();
}

static gm_result_void_t write_fallback(bool is_absolute, char *output,
                                       size_t output_size) {
    if (is_absolute) {
        if (output_size < 2U) {
            gm_memset_safe(output, output_size, 0, output_size);
            return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                        "normalized path exceeds buffer"));
        }
        output[0] = '/';
        output[1] = '\0';
        return gm_ok_void();
    }

    if (output_size < 2U) {
        gm_memset_safe(output, output_size, 0, output_size);
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                    "normalized path exceeds buffer"));
    }
    output[0] = '.';
    output[1] = '\0';
    return gm_ok_void();
}

static gm_result_void_t append_segment_entry(char *output, size_t output_size,
                                             char *const *segments,
                                             size_t index, size_t *cursor) {
    const char *segment_value = segments[index];
    size_t seg_len = strlen(segment_value);
    if (seg_len == 0U) {
        return gm_ok_void();
    }

    bool need_separator = false;
    if (*cursor > 0U && output[*cursor - 1U] != '/') {
        need_separator = true;
    }
    return append_to_path(output, output_size, cursor, need_separator,
                          segment_value, seg_len);
}

static gm_result_void_t emit_segments(bool is_absolute, char *output,
                                      size_t output_size, char *const *segments,
                                      size_t segment_count) {
    size_t dst = 0U;

    if (is_absolute) {
        gm_result_void_t slash_result =
            append_to_path(output, output_size, &dst, true, "", 0U);
        if (!slash_result.ok) {
            return slash_result;
        }
    }

    for (size_t idx = 0; idx < segment_count; ++idx) {
        gm_result_void_t append_result =
            append_segment_entry(output, output_size, segments, idx, &dst);
        if (!append_result.ok) {
            return append_result;
        }
    }

    if (dst == 0U) {
        return write_fallback(is_absolute, output, output_size);
    }

    if (dst >= output_size) {
        gm_memset_safe(output, output_size, 0, output_size);
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                    "normalized path exceeds buffer"));
    }
    output[dst] = '\0';
    return gm_ok_void();
}

GM_NODISCARD gm_result_void_t gm_fs_path_normalize_logical(const char *input,
                                                           char *output,
                                                           size_t output_size) {
    if (input == NULL || output == NULL || output_size == 0U) {
        if (output != NULL && output_size > 0U) {
            gm_memset_safe(output, output_size, 0, output_size);
        }
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "logical normalize requires buffers"));
    }

    if (output_size < 2U) {
        gm_memset_safe(output, output_size, 0, output_size);
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                    "normalized path requires at least 2 bytes"));
    }

    char scratch[GM_PATH_MAX];
    if (gm_strcpy_safe(scratch, sizeof(scratch), input) != 0) {
        gm_memset_safe(output, output_size, 0, output_size);
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                    "path exceeds buffer"));
    }

    bool is_absolute = (scratch[0] == '/');
    char *segments[MAX_PATH_SEGMENTS];
    const size_t max_segments = sizeof(segments) / sizeof(segments[0]);
    size_t segment_count = 0U;

    gm_result_void_t parse_result =
        parse_segments(scratch, is_absolute, segments, &segment_count,
                       max_segments);
    if (!parse_result.ok) {
        gm_memset_safe(output, output_size, 0, output_size);
        return parse_result;
    }

    return emit_segments(is_absolute, output, output_size, segments,
                         segment_count);
}

GM_NODISCARD gm_result_void_t gm_fs_path_dirname(const char *input,
                                                 char *output,
                                                 size_t output_size) {
    if (input == NULL || output == NULL || output_size == 0U) {
        if (output != NULL && output_size > 0U) {
            gm_memset_safe(output, output_size, 0, output_size);
        }
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "dirname requires buffers"));
    }

    char normalized[GM_PATH_MAX];
    GM_TRY(gm_fs_path_normalize_logical(input, normalized, sizeof(normalized)));

    trim_trailing_slashes(normalized);

    const char *result_str = normalized;
    size_t result_len = 0U;
    gm_result_void_t select_result =
        select_dirname_result(normalized, &result_str, &result_len);
    if (!select_result.ok) {
        return select_result;
    }

    return copy_dirname_result(result_str, result_len, normalized, output,
                               output_size);
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

    size_t leaf_len = 0;
    const char *leaf = basename_from_normalized(normalized, &leaf_len);
    if (leaf_len == 0U) {
        return gm_ok_void();
    }

    size_t base_len = *inout_len;
    if (base_len > buffer_size) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "basename append length exceeds buffer"));
    }

    const bool need_sep = needs_path_separator(base_io, base_len);
    size_t separator_len = 0U;
    if (need_sep) {
        separator_len = 1U;
    }
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
