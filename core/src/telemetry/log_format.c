/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/telemetry/log_format.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/security/string.h"

static int append_text(char *out, size_t out_size, size_t *idx,
                       const char *key, const char *val) {
    int wrote = gm_snprintf(out + *idx, out_size - *idx, "%s%s=%s",
                            (*idx > 0) ? " " : "", key ? key : "",
                            val ? val : "");
    if (wrote < 0) {
        return GM_ERR_BUFFER_TOO_SMALL;
    }
    *idx += (size_t)wrote;
    if (*idx >= out_size) {
        return GM_ERR_BUFFER_TOO_SMALL;
    }
    return GM_OK;
}

static const size_t JsonEscapePairLength = 2U;
static const size_t JsonLiteralLength = 1U;
static const size_t JsonHexEscapeLength = 6U;
static const unsigned char JsonControlThreshold = 0x20U;

static bool lacks_capacity(size_t idx, size_t out_size, size_t needed) {
    return (idx + needed) > out_size;
}

static int append_escape_pair(char *out, size_t out_size, size_t *idx,
                              unsigned char character) {
    if (lacks_capacity(*idx, out_size, JsonEscapePairLength)) {
        return GM_ERR_BUFFER_TOO_SMALL;
    }
    out[(*idx)++] = '\\';
    out[(*idx)++] = (char)character;
    return GM_OK;
}

static int append_literal_character(char *out, size_t out_size, size_t *idx,
                                    unsigned char character) {
    if (lacks_capacity(*idx, out_size, JsonLiteralLength)) {
        return GM_ERR_BUFFER_TOO_SMALL;
    }
    out[(*idx)++] = (char)character;
    return GM_OK;
}

static int append_control_escape(char *out, size_t out_size, size_t *idx,
                                 unsigned char character) {
    /* Need space for visible characters and the terminating null written by
     * gm_snprintf. */
    if (lacks_capacity(*idx, out_size, JsonHexEscapeLength + 1U)) {
        return GM_ERR_BUFFER_TOO_SMALL;
    }
    int hex_written = gm_snprintf(out + *idx, out_size - *idx, "\\u%04x",
                                  (unsigned)character);
    if (hex_written < 0 ||
        (size_t)hex_written >= (out_size - *idx)) {
        return GM_ERR_BUFFER_TOO_SMALL;
    }
    *idx += (size_t)hex_written;
    return GM_OK;
}

static int append_one_json_character(char *out, size_t out_size, size_t *idx,
                                     unsigned char character) {
    if (character == '"' || character == '\\') {
        return append_escape_pair(out, out_size, idx, character);
    }
    if (character < JsonControlThreshold) {
        return append_control_escape(out, out_size, idx, character);
    }
    return append_literal_character(out, out_size, idx, character);
}

static int append_json_escaped(char *out, size_t out_size, size_t *idx,
                               const char *input) {
    const char *value = (input == NULL) ? "" : input;
    for (const char *cursor = value; *cursor != '\0'; ++cursor) {
        unsigned char character = (unsigned char)*cursor;
        int status = append_one_json_character(out, out_size, idx, character);
        if (status != GM_OK) {
            return status;
        }
    }
    return GM_OK;
}

static int append_json_kv(char *out, size_t out_size, size_t *idx,
                          const char *key, const char *val, bool *first) {
    if (!*first) {
        if (*idx + 1 >= out_size) {
            return GM_ERR_BUFFER_TOO_SMALL;
        }
        out[(*idx)++] = ',';
    } else {
        *first = false;
    }
    if (*idx + 1 >= out_size) {
        return GM_ERR_BUFFER_TOO_SMALL;
    }
    out[(*idx)++] = '"';
    int escape_status = append_json_escaped(out, out_size, idx, key);
    if (escape_status != GM_OK) {
        return escape_status;
    }
    if (*idx + 2 >= out_size) {
        return GM_ERR_BUFFER_TOO_SMALL;
    }
    out[(*idx)++] = '"';
    out[(*idx)++] = ':';
    if (*idx + 1 >= out_size) {
        return GM_ERR_BUFFER_TOO_SMALL;
    }
    out[(*idx)++] = '"';
    escape_status = append_json_escaped(out, out_size, idx, val);
    if (escape_status != GM_OK) {
        return escape_status;
    }
    if (*idx + 1 >= out_size) {
        return GM_ERR_BUFFER_TOO_SMALL;
    }
    out[(*idx)++] = '"';
    return GM_OK;
}

static int compare_kv(const gm_log_kv_t *lhs, const gm_log_kv_t *rhs) {
    const char *lhs_key = (lhs != NULL && lhs->key != NULL) ? lhs->key : "";
    const char *rhs_key = (rhs != NULL && rhs->key != NULL) ? rhs->key : "";
    int key_compare = strcmp(lhs_key, rhs_key);
    if (key_compare != 0) {
        return key_compare;
    }
    const char *lhs_value =
        (lhs != NULL && lhs->value != NULL) ? lhs->value : "";
    const char *rhs_value =
        (rhs != NULL && rhs->value != NULL) ? rhs->value : "";
    return strcmp(lhs_value, rhs_value);
}

static void sort_kvs(const gm_log_kv_t **ordered, size_t count) {
    for (size_t entry_index = 1; entry_index < count; ++entry_index) {
        const gm_log_kv_t *current_entry = ordered[entry_index];
        size_t insert_index = entry_index;
        while (insert_index > 0 &&
               compare_kv(ordered[insert_index - 1], current_entry) > 0) {
            ordered[insert_index] = ordered[insert_index - 1];
            --insert_index;
        }
        ordered[insert_index] = current_entry;
    }
}

static gm_result_void_t create_sorted_entries(const gm_log_kv_t *kvs,
                                              size_t kv_count,
                                              const gm_log_kv_t ***out_sorted) {
    if (out_sorted == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "sorted output pointer missing"));
    }
    if (kv_count == 0U) {
        *out_sorted = NULL;
        return gm_ok_void();
    }

    const gm_log_kv_t **sorted =
        (const gm_log_kv_t **)malloc(sizeof(*sorted) * kv_count);
    if (sorted == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_OUT_OF_MEMORY,
                                    "log formatter allocation failed"));
    }

    for (size_t index = 0; index < kv_count; ++index) {
        sorted[index] = &kvs[index];
    }
    sort_kvs(sorted, kv_count);
    *out_sorted = sorted;
    return gm_ok_void();
}

static void release_sorted_entries(const gm_log_kv_t **sorted) {
    if (sorted != NULL) {
        free((void *)sorted);
    }
}

static gm_result_void_t render_as_text(const gm_log_kv_t *kvs,
                                       size_t kv_count,
                                       char *out,
                                       size_t out_size) {
    size_t idx = 0U;
    for (size_t entry_index = 0; entry_index < kv_count; ++entry_index) {
        const gm_log_kv_t *entry = &kvs[entry_index];
        int append_status =
            append_text(out, out_size, &idx, entry->key, entry->value);
        if (append_status != GM_OK) {
            out[0] = '\0';
            return gm_err_void(
                GM_ERROR(append_status, "text format overflow"));
        }
    }

    if (idx >= out_size) {
        out[0] = '\0';
        return gm_err_void(
            GM_ERROR(GM_ERR_BUFFER_TOO_SMALL, "buffer too small"));
    }

    out[idx] = '\0';
    return gm_ok_void();
}

static gm_result_void_t render_as_json(const gm_log_kv_t *kvs,
                                       size_t kv_count,
                                       char *out,
                                       size_t out_size) {
    const gm_log_kv_t **sorted = NULL;
    gm_result_void_t sort_result = create_sorted_entries(kvs, kv_count, &sorted);
    if (!sort_result.ok) {
        return sort_result;
    }

    size_t idx = 0U;
    if (out_size <= 1U) {
        release_sorted_entries(sorted);
        return gm_err_void(
            GM_ERROR(GM_ERR_BUFFER_TOO_SMALL, "buffer too small"));
    }

    out[idx++] = '{';
    bool is_first = true;
    for (size_t entry_index = 0; entry_index < kv_count; ++entry_index) {
        const gm_log_kv_t *entry =
            (sorted != NULL) ? sorted[entry_index] : &kvs[entry_index];
        int append_status =
            append_json_kv(out, out_size, &idx, entry->key, entry->value,
                           &is_first);
        if (append_status != GM_OK) {
            out[0] = '\0';
            release_sorted_entries(sorted);
            return gm_err_void(
                GM_ERROR(append_status, "json format overflow"));
        }
    }

    if (idx + 2U > out_size) {
        out[0] = '\0';
        release_sorted_entries(sorted);
        return gm_err_void(
            GM_ERROR(GM_ERR_BUFFER_TOO_SMALL, "buffer too small"));
    }

    out[idx++] = '}';
    out[idx] = '\0';
    release_sorted_entries(sorted);
    return gm_ok_void();
}

gm_result_void_t gm_log_format_render_default(const gm_log_kv_t *kvs,
                                              size_t kv_count,
                                              bool json,
                                              char *out,
                                              size_t out_size) {
    if (out == NULL || out_size == 0U) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "log formatter requires buffer"));
    }
    out[0] = '\0';

    if (kvs == NULL && kv_count > 0U) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "key/values missing for formatter"));
    }

    if (json) {
        return render_as_json(kvs, kv_count, out, out_size);
    }

    return render_as_text(kvs, kv_count, out, out_size);
}
