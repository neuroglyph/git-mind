/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/telemetry/log_format.h"

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
    if (wrote < 0) return GM_ERR_BUFFER_TOO_SMALL;
    *idx += (size_t)wrote;
    if (*idx >= out_size) return GM_ERR_BUFFER_TOO_SMALL;
    return GM_OK;
}

static int append_json_escaped(char *out, size_t out_size, size_t *idx,
                               const char *s) {
    if (s == NULL) s = "";
    for (const char *p = s; *p != '\0'; ++p) {
        unsigned char c = (unsigned char)*p;
        if (c == '"' || c == '\\') {
            if (*idx + 2 >= out_size) return GM_ERR_BUFFER_TOO_SMALL;
            out[(*idx)++] = '\\';
            out[(*idx)++] = (char)c;
        } else if (c < 0x20U) {
            if (*idx + 6 >= out_size) return GM_ERR_BUFFER_TOO_SMALL;
            int n = gm_snprintf(out + *idx, out_size - *idx, "\\u%04x",
                                (unsigned)c);
            if (n < 0) return GM_ERR_BUFFER_TOO_SMALL;
            *idx += (size_t)n;
        } else {
            if (*idx + 1 >= out_size) return GM_ERR_BUFFER_TOO_SMALL;
            out[(*idx)++] = (char)c;
        }
    }
    return GM_OK;
}

static int append_json_kv(char *out, size_t out_size, size_t *idx,
                          const char *key, const char *val, bool *first) {
    if (!*first) {
        if (*idx + 1 >= out_size) return GM_ERR_BUFFER_TOO_SMALL;
        out[(*idx)++] = ',';
    } else {
        *first = false;
    }
    if (*idx + 1 >= out_size) return GM_ERR_BUFFER_TOO_SMALL;
    out[(*idx)++] = '"';
    int rc = append_json_escaped(out, out_size, idx, key);
    if (rc != GM_OK) return rc;
    if (*idx + 2 >= out_size) return GM_ERR_BUFFER_TOO_SMALL;
    out[(*idx)++] = '"';
    out[(*idx)++] = ':';
    if (*idx + 1 >= out_size) return GM_ERR_BUFFER_TOO_SMALL;
    out[(*idx)++] = '"';
    rc = append_json_escaped(out, out_size, idx, val);
    if (rc != GM_OK) return rc;
    if (*idx + 1 >= out_size) return GM_ERR_BUFFER_TOO_SMALL;
    out[(*idx)++] = '"';
    return GM_OK;
}

static int compare_kv(const gm_log_kv_t *lhs, const gm_log_kv_t *rhs) {
    const char *lk = lhs && lhs->key ? lhs->key : "";
    const char *rk = rhs && rhs->key ? rhs->key : "";
    int cmp = strcmp(lk, rk);
    if (cmp != 0) return cmp;
    const char *lv = lhs && lhs->value ? lhs->value : "";
    const char *rv = rhs && rhs->value ? rhs->value : "";
    return strcmp(lv, rv);
}

static void sort_kvs(const gm_log_kv_t **ordered, size_t count) {
    for (size_t i = 1; i < count; ++i) {
        const gm_log_kv_t *cur = ordered[i];
        size_t j = i;
        while (j > 0 && compare_kv(ordered[j - 1], cur) > 0) {
            ordered[j] = ordered[j - 1];
            --j;
        }
        ordered[j] = cur;
    }
}

gm_result_void_t gm_log_format_render_default(const gm_log_kv_t *kvs,
                                              size_t kv_count,
                                              bool json,
                                              char *out,
                                              size_t out_size) {
    if (out == NULL || out_size == 0) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "log formatter requires buffer"));
    }
    out[0] = '\0';
    if (kvs == NULL && kv_count > 0) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "key/values missing for formatter"));
    }

    const gm_log_kv_t **ordered = NULL;
    if (json && kv_count > 0) {
        ordered = malloc(sizeof(*ordered) * kv_count);
        if (ordered == NULL) {
            return gm_err_void(GM_ERROR(GM_ERR_OUT_OF_MEMORY,
                                        "log formatter allocation failed"));
        }
        for (size_t i = 0; i < kv_count; ++i) {
            ordered[i] = &kvs[i];
        }
        sort_kvs(ordered, kv_count);
    }

    size_t idx = 0;
    if (json) {
        if (idx + 1 >= out_size) {
            if (ordered != NULL) {
                free((void *)ordered);
            }
            return gm_err_void(
                GM_ERROR(GM_ERR_BUFFER_TOO_SMALL, "buffer too small"));
        }
        out[idx++] = '{';
        bool first = true;
        for (size_t i = 0; i < kv_count; ++i) {
            const gm_log_kv_t *entry = ordered ? ordered[i] : &kvs[i];
            int rc = append_json_kv(out, out_size, &idx, entry->key,
                                    entry->value, &first);
            if (rc != GM_OK) {
                out[0] = '\0';
                if (ordered != NULL) {
                    free((void *)ordered);
                }
                return gm_err_void(GM_ERROR(rc, "json format overflow"));
            }
        }
        if (idx + 2 >= out_size) {
            out[0] = '\0';
            if (ordered != NULL) {
                free((void *)ordered);
            }
            return gm_err_void(
                GM_ERROR(GM_ERR_BUFFER_TOO_SMALL, "buffer too small"));
        }
        out[idx++] = '}';
        out[idx] = '\0';
        if (ordered != NULL) {
            free((void *)ordered);
        }
        return gm_ok_void();
    }

    for (size_t i = 0; i < kv_count; ++i) {
        const gm_log_kv_t *entry = &kvs[i];
        int append_status =
            append_text(out, out_size, &idx, entry->key, entry->value);
        if (append_status != GM_OK) {
            out[0] = '\0';
            if (ordered != NULL) {
                free((void *)ordered);
            }
            return gm_err_void(
                GM_ERROR(append_status, "text format overflow"));
        }
    }
    if (idx < out_size) {
        out[idx] = '\0';
    }
    if (ordered != NULL) {
        free((void *)ordered);
    }
    return gm_ok_void();
}
