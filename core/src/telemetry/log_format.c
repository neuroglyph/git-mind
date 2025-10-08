/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/telemetry/internal/log_format.h"

#include <stddef.h>
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
        char c = *p;
        if (c == '"' || c == '\\') {
            if (*idx + 2 >= out_size) return GM_ERR_BUFFER_TOO_SMALL;
            out[(*idx)++] = '\\';
            out[(*idx)++] = c;
        } else if ((unsigned char)c < 0x20) {
            /* control chars -> \u00XX */
            if (*idx + 6 >= out_size) return GM_ERR_BUFFER_TOO_SMALL;
            int n = gm_snprintf(out + *idx, out_size - *idx, "\\u%04x",
                                (unsigned)(unsigned char)c);
            if (n < 0) return GM_ERR_BUFFER_TOO_SMALL;
            *idx += (size_t)n;
        } else {
            if (*idx + 1 >= out_size) return GM_ERR_BUFFER_TOO_SMALL;
            out[(*idx)++] = c;
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
    if (*idx + 2 >= out_size) return GM_ERR_BUFFER_TOO_SMALL; /* for quotes */
    out[(*idx)++] = '"';
    int rc = append_json_escaped(out, out_size, idx, key);
    if (rc != GM_OK) return rc;
    if (*idx + 3 >= out_size) return GM_ERR_BUFFER_TOO_SMALL;
    out[(*idx)++] = '"';
    out[(*idx)++] = ':';
    out[(*idx)++] = '"';
    rc = append_json_escaped(out, out_size, idx, val);
    if (rc != GM_OK) return rc;
    if (*idx + 1 >= out_size) return GM_ERR_BUFFER_TOO_SMALL;
    out[(*idx)++] = '"';
    return GM_OK;
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

    size_t idx = 0;
    if (json) {
        if (idx + 1 >= out_size) return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL, "buffer too small"));
        out[idx++] = '{';
        bool first = true;
        for (size_t i = 0; i < kv_count; ++i) {
            int rc = append_json_kv(out, out_size, &idx, kvs[i].key, kvs[i].value, &first);
            if (rc != GM_OK) return gm_err_void(GM_ERROR(rc, "json format overflow"));
        }
        if (idx + 2 >= out_size) return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL, "buffer too small"));
        out[idx++] = '}';
        out[idx] = '\0';
        return gm_ok_void();
    }

    for (size_t i = 0; i < kv_count; ++i) {
        int rc = append_text(out, out_size, &idx, kvs[i].key, kvs[i].value);
        if (rc != GM_OK) return gm_err_void(GM_ERROR(rc, "text format overflow"));
    }
    if (idx < out_size) out[idx] = '\0';
    return gm_ok_void();
}

