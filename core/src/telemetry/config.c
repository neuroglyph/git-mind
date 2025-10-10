/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/telemetry/internal/config.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#if !defined(_WIN32)
#include <strings.h>
#endif

#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/security/string.h"
#include "gitmind/util/memory.h"
#include "gitmind/security/memory.h"
#include "gitmind/ports/fs_temp_port.h"
#include "gitmind/types.h"
#include "gitmind/crypto/backend.h"
#include "gitmind/crypto/sha256.h"
#include "gitmind/constants_internal.h"

#define MAX_TAGS_TOTAL 5

#if defined(_WIN32)
static int gm_ascii_casecmp(const char *a, const char *b) {
    return _stricmp(a, b);
}
#else
static int gm_ascii_casecmp(const char *a, const char *b) {
    return strcasecmp(a, b);
}
#endif

static bool parse_bool_default_true(const char *s) {
    if (s == NULL || s[0] == '\0') return true;
    if (strcmp(s, "0") == 0) return false;
    if (gm_ascii_casecmp(s, "false") == 0) return false;
    if (gm_ascii_casecmp(s, "off") == 0) return false;
    if (gm_ascii_casecmp(s, "no") == 0) return false;
    return true;
}

static gm_repo_tag_mode_t parse_repo_tag_mode(const char *s) {
    if (s == NULL || s[0] == '\0') return GM_REPO_TAG_OFF;
    if (gm_ascii_casecmp(s, "off") == 0) return GM_REPO_TAG_OFF;
    if (gm_ascii_casecmp(s, "hash") == 0) return GM_REPO_TAG_HASH;
    if (gm_ascii_casecmp(s, "plain") == 0) return GM_REPO_TAG_PLAIN;
    return GM_REPO_TAG_OFF;
}

static gm_log_level_t parse_log_level(const char *s) {
    if (s == NULL || s[0] == '\0') return GM_LOG_INFO;
    if (gm_ascii_casecmp(s, "DEBUG") == 0) return GM_LOG_DEBUG;
    if (gm_ascii_casecmp(s, "INFO") == 0) return GM_LOG_INFO;
    if (gm_ascii_casecmp(s, "WARN") == 0) return GM_LOG_WARN;
    if (gm_ascii_casecmp(s, "ERROR") == 0) return GM_LOG_ERROR;
    return GM_LOG_INFO;
}

static gm_log_format_t parse_log_format(const char *s) {
    if (s == NULL || s[0] == '\0') return GM_LOG_FMT_TEXT;
    if (gm_ascii_casecmp(s, "text") == 0) return GM_LOG_FMT_TEXT;
    if (gm_ascii_casecmp(s, "json") == 0) return GM_LOG_FMT_JSON;
    return GM_LOG_FMT_TEXT;
}

static bool parse_hash_algo_sha256(const char *s) {
    if (s == NULL || s[0] == '\0') return false; /* default fnv */
    if (gm_ascii_casecmp(s, "sha256") == 0) return true;
    if (gm_ascii_casecmp(s, "fnv") == 0) return false;
    return false;
}

static void fnv1a64_hex12(const uint8_t *data, size_t len, char *out12);
static void sha256_hex12(const uint8_t *data, size_t len, char *out12);

static void format_repo_hash_bytes(const gm_telemetry_cfg_t *cfg,
                                   const uint8_t *src, size_t len,
                                   char *out) {
    if (src == NULL || len == 0 || out == NULL) {
        if (out != NULL) out[0] = '\0';
        return;
    }
    if (cfg != NULL && cfg->repo_hash_sha256) {
        sha256_hex12(src, len, out);
    } else {
        fnv1a64_hex12(src, len, out);
    }
}

static void format_repo_hash_from_str(const gm_telemetry_cfg_t *cfg,
                                      const char *str, char *out) {
    if (str == NULL) {
        if (out != NULL) out[0] = '\0';
        return;
    }
    format_repo_hash_bytes(cfg, (const uint8_t *)str, strlen(str), out);
}

static void format_repo_hash_from_id(const gm_telemetry_cfg_t *cfg,
                                     const gm_repo_id_t *repo_id,
                                     char *out) {
    if (repo_id == NULL) {
        if (out != NULL) out[0] = '\0';
        return;
    }
    char idbuf[33 + 33];
    int wrote = gm_snprintf(idbuf, sizeof(idbuf), "%016" PRIx64 "%016" PRIx64,
                            repo_id->hi, repo_id->lo);
    if (wrote > 0 && (size_t)wrote < sizeof(idbuf)) {
        format_repo_hash_bytes(cfg, (const uint8_t *)idbuf, (size_t)wrote, out);
    } else if (out != NULL) {
        out[0] = '\0';
    }
}

static bool is_key_char(char c) {
    return (bool)(c == '_' || c == '-' || (c >= 'a' && c <= 'z') ||
                  (c >= '0' && c <= '9'));
}

static bool is_val_char(char c) {
    /* Permit a conservative set */
    return (bool)((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                  (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.' ||
                  c == ':' || c == '@' || c == '/');
}

static bool validate_key(const char *k) {
    if (k == NULL || k[0] == '\0') return false;
    size_t n = strlen(k);
    if (n > sizeof(((gm_kv_pair_t *)0)->key) - 1) return false;
    for (size_t i = 0; i < n; ++i) {
        if (!is_key_char(k[i])) return false;
    }
    return true;
}

static bool validate_val(const char *v) {
    if (v == NULL) return false;
    size_t n = strlen(v);
    if (n == 0 || n > sizeof(((gm_kv_pair_t *)0)->value) - 1) return false;
    for (size_t i = 0; i < n; ++i) {
        if (!is_val_char(v[i])) return false;
    }
    return true;
}

static void add_extra_if_valid(gm_telemetry_cfg_t *cfg, const char *k,
                               const char *v, bool *dropped) {
    const size_t cap = sizeof(cfg->extras) / sizeof(cfg->extras[0]);
    if (cfg->extra_count >= cap) {
        *dropped = true;
        return;
    }
    if (!validate_key(k) || !validate_val(v)) {
        *dropped = true;
        return;
    }
    gm_kv_pair_t *p = &cfg->extras[cfg->extra_count];
    if (gm_strcpy_safe(p->key, sizeof(p->key), k) != GM_OK) {
        gm_memset_safe(p, sizeof(*p), 0, sizeof(*p));
        *dropped = true;
        return;
    }
    if (gm_strcpy_safe(p->value, sizeof(p->value), v) != GM_OK) {
        gm_memset_safe(p, sizeof(*p), 0, sizeof(*p));
        *dropped = true;
        return;
    }
    cfg->extra_count++;
}

static void parse_extras(gm_telemetry_cfg_t *cfg, const char *csv) {
    if (csv == NULL || csv[0] == '\0') return;
    char buf[256];
    if (gm_strcpy_safe(buf, sizeof(buf), csv) != GM_OK) {
        cfg->extras_dropped = true;
        return;
    }
    char *saveptr = NULL;
    char *tok = strtok_r(buf, ",", &saveptr);
    while (tok != NULL) {
        while (*tok == ' ') tok++;
        char *eq = strchr(tok, '=');
        if (eq == NULL) {
            cfg->extras_dropped = true;
        } else {
            *eq = '\0';
            const char *k = tok;
            const char *v = eq + 1;
            bool dropped = false;
            add_extra_if_valid(cfg, k, v, &dropped);
            if (dropped) cfg->extras_dropped = true;
        }
        tok = strtok_r(NULL, ",", &saveptr);
    }
}

GM_NODISCARD gm_result_void_t gm_telemetry_cfg_load(gm_telemetry_cfg_t *out,
                                                    const gm_env_port_t *env) {
    if (out == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "cfg output is null"));
    }
    gm_memset_safe(out, sizeof(*out), 0, sizeof(*out));
    out->metrics_enabled = true;
    out->tag_branch = true;
    out->tag_mode = true;
    out->repo_tag = GM_REPO_TAG_OFF;
    out->repo_hash_sha256 = false;
    out->log_level = GM_LOG_INFO;
    out->log_format = GM_LOG_FMT_TEXT;

    const gm_env_port_t *src = (env != NULL) ? env : gm_env_port_system();
    char buf[256];

    if (gm_env_get(src, "GITMIND_METRICS_ENABLED", buf, sizeof buf).ok) {
        out->metrics_enabled = parse_bool_default_true(buf);
    }
    if (gm_env_get(src, "GITMIND_METRICS_BRANCH_TAG", buf, sizeof buf).ok) {
        out->tag_branch = parse_bool_default_true(buf);
    }
    if (gm_env_get(src, "GITMIND_METRICS_MODE_TAG", buf, sizeof buf).ok) {
        out->tag_mode = parse_bool_default_true(buf);
    }
    if (gm_env_get(src, "GITMIND_METRICS_REPO_TAG", buf, sizeof buf).ok) {
        out->repo_tag = parse_repo_tag_mode(buf);
    }
    if (gm_env_get(src, "GITMIND_METRICS_REPO_HASH_ALGO", buf, sizeof buf).ok) {
        out->repo_hash_sha256 = parse_hash_algo_sha256(buf);
    }
    if (gm_env_get(src, "GITMIND_METRICS_EXTRA_TAGS", buf, sizeof buf).ok) {
        parse_extras(out, buf);
    }
    if (gm_env_get(src, "GITMIND_LOG_LEVEL", buf, sizeof buf).ok) {
        out->log_level = parse_log_level(buf);
    }
    if (gm_env_get(src, "GITMIND_LOG_FORMAT", buf, sizeof buf).ok) {
        out->log_format = parse_log_format(buf);
    }

    return gm_ok_void();
}

static int append_kv(char *out, size_t out_size, size_t *idx,
                     const char *k, const char *v) {
    if (k == NULL || v == NULL || k[0] == '\0' || v[0] == '\0') {
        return GM_OK;
    }
    int wrote = gm_snprintf(out + *idx, out_size - *idx, "%s%s=%s",
                            (*idx > 0) ? "," : "", k, v);
    if (wrote < 0) {
        return GM_ERR_BUFFER_TOO_SMALL;
    }
    *idx += (size_t)wrote;
    if (*idx >= out_size) {
        return GM_ERR_BUFFER_TOO_SMALL;
    }
    return GM_OK;
}

static void fnv1a64_hex12(const uint8_t *data, size_t len, char *out12) {
    const uint64_t FNV_OFFSET = 0xcbf29ce484222325ULL;
    const uint64_t FNV_PRIME = 0x100000001b3ULL;
    uint64_t h = FNV_OFFSET;
    for (size_t i = 0; i < len; ++i) { h ^= (uint64_t)data[i]; h *= FNV_PRIME; }
    static const char HEX[] = "0123456789abcdef";
    for (int i = 0; i < 12; ++i) {
        int shift = (15 - i) * 4;
        out12[i] = HEX[(int)((h >> shift) & 0xF)];
    }
    out12[12] = '\0';
}

static void sha256_hex12(const uint8_t *data, size_t len, char *out12) {
    uint8_t digest[GM_SHA256_DIGEST_SIZE];
    gm_result_crypto_context_t cr = gm_crypto_context_create(gm_crypto_backend_libsodium());
    if (!cr.ok) {
        /* Fallback to FNV on any error */
        fnv1a64_hex12(data, len, out12);
        return;
    }
    gm_result_void_t hr = gm_sha256_with_context(&cr.u.val, data, len, digest);
    if (!hr.ok) {
        if (hr.u.err) gm_error_free(hr.u.err);
        gm_result_void_t dispose_rc = gm_crypto_context_dispose(&cr.u.val);
        if (!dispose_rc.ok && dispose_rc.u.err) {
            gm_error_free(dispose_rc.u.err);
        }
        fnv1a64_hex12(data, len, out12);
        return;
    }
    gm_result_void_t dispose_rc = gm_crypto_context_dispose(&cr.u.val);
    if (!dispose_rc.ok && dispose_rc.u.err) {
        gm_error_free(dispose_rc.u.err);
    }
    static const char HEX[] = "0123456789abcdef";
    for (int i = 0; i < 6; ++i) {
        out12[2 * i] = HEX[(digest[i] >> 4) & 0xF];
        out12[2 * i + 1] = HEX[digest[i] & 0xF];
    }
    out12[12] = '\0';
}

typedef struct {
    char *buffer;
    size_t capacity;
    size_t index;
    size_t count;
} gm_tag_builder_t;

static bool is_present(const char *value) {
    return value != NULL && value[0] != '\0';
}

static bool can_append_tag(const gm_tag_builder_t *builder) {
    return builder->count < MAX_TAGS_TOTAL;
}

static gm_result_void_t builder_append_tag(gm_tag_builder_t *builder,
                                           const char *key,
                                           const char *value) {
    if (!is_present(key) || !is_present(value)) {
        return gm_ok_void();
    }
    if (!can_append_tag(builder)) {
        return gm_ok_void();
    }
    int append_status =
        append_kv(builder->buffer, builder->capacity, &builder->index,
                  key, value);
    if (append_status != GM_OK) {
        builder->buffer[0] = '\0';
        return gm_err_void(
            GM_ERROR(GM_ERR_BUFFER_TOO_SMALL, "tags overflow"));
    }
    builder->count += 1U;
    return gm_ok_void();
}

static gm_result_void_t compute_repo_tag_value(const gm_telemetry_cfg_t *cfg,
                                               const gm_telemetry_tag_context_t *ctx,
                                               char *repo_value,
                                               size_t repo_value_size) {
    if (repo_value == NULL || repo_value_size == 0U) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, "repo buffer missing"));
    }

    repo_value[0] = '\0';
    if (cfg == NULL || cfg->repo_tag == GM_REPO_TAG_OFF) {
        return gm_ok_void();
    }

    const char *canonical_path = NULL;
    const gm_repo_id_t *repo_id = NULL;
    if (ctx != NULL) {
        canonical_path = ctx->repo_canon_path;
        repo_id = ctx->repo_id;
    }

    switch (cfg->repo_tag) {
    case GM_REPO_TAG_PLAIN:
        if (is_present(canonical_path)) {
            int copy_status =
                gm_strcpy_safe(repo_value, repo_value_size, canonical_path);
            if (copy_status != GM_OK) {
                format_repo_hash_from_str(cfg, canonical_path, repo_value);
            }
        } else if (repo_id != NULL) {
            format_repo_hash_from_id(cfg, repo_id, repo_value);
        }
        break;
    case GM_REPO_TAG_HASH:
        if (is_present(canonical_path)) {
            format_repo_hash_from_str(cfg, canonical_path, repo_value);
        } else if (repo_id != NULL) {
            format_repo_hash_from_id(cfg, repo_id, repo_value);
        }
        break;
    case GM_REPO_TAG_OFF:
    default:
        break;
    }

    return gm_ok_void();
}

static gm_result_void_t append_extra_tags(const gm_telemetry_cfg_t *cfg,
                                          gm_tag_builder_t *builder) {
    if (cfg == NULL || cfg->extra_count == 0U) {
        return gm_ok_void();
    }

    for (size_t extra_index = 0; extra_index < cfg->extra_count; ++extra_index) {
        if (!can_append_tag(builder)) {
            break;
        }
        gm_result_void_t result = builder_append_tag(
            builder, cfg->extras[extra_index].key,
            cfg->extras[extra_index].value);
        if (!result.ok) {
            return result;
        }
    }

    return gm_ok_void();
}

GM_NODISCARD gm_result_void_t gm_telemetry_build_tags(
    const gm_telemetry_cfg_t *cfg, const gm_telemetry_tag_context_t *ctx,
    char *out, size_t out_size) {
    if (out == NULL || out_size == 0U) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, "tags buffer missing"));
    }

    out[0] = '\0';

    gm_tag_builder_t builder = {
        .buffer = out,
        .capacity = out_size,
        .index = 0U,
        .count = 0U,
    };

    const char *branch = NULL;
    const char *mode = NULL;
    if (ctx != NULL) {
        branch = ctx->branch;
        mode = ctx->mode;
    }

    bool has_config = (cfg != NULL);

    if (has_config && cfg->tag_branch) {
        GM_TRY(builder_append_tag(&builder, "branch", branch));
    }

    if (has_config && cfg->tag_mode) {
        GM_TRY(builder_append_tag(&builder, "mode", mode));
    }

    if (has_config && cfg->repo_tag != GM_REPO_TAG_OFF) {
        char repo_value[GM_PATH_MAX] = {0};
        GM_TRY(compute_repo_tag_value(cfg, ctx, repo_value, sizeof(repo_value)));
        if (can_append_tag(&builder)) {
            GM_TRY(builder_append_tag(&builder, "repo", repo_value));
        }
    }

    if (has_config) {
        GM_TRY(append_extra_tags(cfg, &builder));
    }

    if (builder.index < builder.capacity) {
        builder.buffer[builder.index] = '\0';
    }

    return gm_ok_void();
}
