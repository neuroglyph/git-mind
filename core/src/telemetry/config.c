/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/telemetry/internal/config.h"

#include <ctype.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#if defined(__linux__) || defined(__APPLE__)
#include <strings.h> /* strcasecmp */
#endif

#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/security/string.h"
#include "gitmind/util/memory.h"
#include "gitmind/crypto/backend.h"
#include "gitmind/crypto/sha256.h"

#define MAX_TAGS_TOTAL 5

static bool parse_bool_default_true(const char *s) {
    if (s == NULL || s[0] == '\0') return true;
    if (strcmp(s, "0") == 0) return false;
    if (strcasecmp(s, "false") == 0) return false;
    if (strcasecmp(s, "off") == 0) return false;
    if (strcasecmp(s, "no") == 0) return false;
    return true;
}

static gm_repo_tag_mode_t parse_repo_tag_mode(const char *s) {
    if (s == NULL || s[0] == '\0') return GM_REPO_TAG_OFF;
    if (strcasecmp(s, "off") == 0) return GM_REPO_TAG_OFF;
    if (strcasecmp(s, "hash") == 0) return GM_REPO_TAG_HASH;
    if (strcasecmp(s, "plain") == 0) return GM_REPO_TAG_PLAIN;
    return GM_REPO_TAG_OFF;
}

static gm_log_level_t parse_log_level(const char *s) {
    if (s == NULL || s[0] == '\0') return GM_LOG_INFO;
    if (strcasecmp(s, "DEBUG") == 0) return GM_LOG_DEBUG;
    if (strcasecmp(s, "INFO") == 0) return GM_LOG_INFO;
    if (strcasecmp(s, "WARN") == 0) return GM_LOG_WARN;
    if (strcasecmp(s, "ERROR") == 0) return GM_LOG_ERROR;
    return GM_LOG_INFO;
}

static gm_log_format_t parse_log_format(const char *s) {
    if (s == NULL || s[0] == '\0') return GM_LOG_FMT_TEXT;
    if (strcasecmp(s, "text") == 0) return GM_LOG_FMT_TEXT;
    if (strcasecmp(s, "json") == 0) return GM_LOG_FMT_JSON;
    return GM_LOG_FMT_TEXT;
}

static bool parse_hash_algo_sha256(const char *s) {
    if (s == NULL || s[0] == '\0') return false; /* default fnv */
    if (strcasecmp(s, "sha256") == 0) return true;
    if (strcasecmp(s, "fnv") == 0) return false;
    return false;
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
    if (cfg->extra_count >= 3) {
        *dropped = true;
        return;
    }
    if (!validate_key(k) || !validate_val(v)) {
        *dropped = true;
        return;
    }
    gm_kv_pair_t *p = &cfg->extras[cfg->extra_count];
    if (gm_strcpy_safe(p->key, sizeof(p->key), k) != GM_OK) {
        memset(p, 0, sizeof(*p));
        *dropped = true;
        return;
    }
    if (gm_strcpy_safe(p->value, sizeof(p->value), v) != GM_OK) {
        memset(p, 0, sizeof(*p));
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
    memset(out, 0, sizeof(*out));
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
    if (k == NULL || v == NULL || k[0] == '\0' || v[0] == '\0') return GM_OK;
    int wrote = gm_snprintf(out + *idx, out_size - *idx, "%s%s=%s",
                            (*idx > 0) ? "," : "", k, v);
    if (wrote < 0) return GM_ERR_BUFFER_TOO_SMALL;
    *idx += (size_t)wrote;
    if (*idx >= out_size) return GM_ERR_BUFFER_TOO_SMALL;
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
        fnv1a64_hex12(data, len, out12);
        return;
    }
    static const char HEX[] = "0123456789abcdef";
    for (int i = 0; i < 6; ++i) {
        out12[2 * i] = HEX[(digest[i] >> 4) & 0xF];
        out12[2 * i + 1] = HEX[digest[i] & 0xF];
    }
    out12[12] = '\0';
}

GM_NODISCARD gm_result_void_t gm_telemetry_build_tags(
    const gm_telemetry_cfg_t *cfg, const char *branch, const char *mode,
    const char *repo_canon_path, const gm_repo_id_t *repo_id, char *out,
    size_t out_size) {
    if (out == NULL || out_size == 0) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "tags buffer missing"));
    }
    out[0] = '\0';
    size_t idx = 0;
    size_t count = 0;

    if (cfg != NULL && cfg->tag_branch && branch != NULL && branch[0] != '\0') {
        if (append_kv(out, out_size, &idx, "branch", branch) != GM_OK) {
            return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL, "tags overflow"));
        }
        ++count;
    }
    if (cfg != NULL && cfg->tag_mode && mode != NULL && mode[0] != '\0') {
        if (append_kv(out, out_size, &idx, "mode", mode) != GM_OK) {
            return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL, "tags overflow"));
        }
        ++count;
    }

    if (cfg != NULL && cfg->repo_tag != GM_REPO_TAG_OFF && count < MAX_TAGS_TOTAL) {
        char repo_val[65] = {0};
        if (cfg->repo_tag == GM_REPO_TAG_PLAIN) {
            if (repo_canon_path != NULL && repo_canon_path[0] != '\0') {
                int copy_status =
                    gm_strcpy_safe(repo_val, sizeof(repo_val), repo_canon_path);
                if (copy_status != GM_OK) {
                    memset(repo_val, 0, sizeof(repo_val));
                    return gm_err_void(GM_ERROR(copy_status,
                                                "repo tag truncated"));
                }
            }
        } else if (cfg->repo_tag == GM_REPO_TAG_HASH) {
            if (repo_canon_path != NULL && repo_canon_path[0] != '\0') {
                if (cfg->repo_hash_sha256) {
                    sha256_hex12((const uint8_t *)repo_canon_path,
                                 strlen(repo_canon_path), repo_val);
                } else {
                    fnv1a64_hex12((const uint8_t *)repo_canon_path,
                                   strlen(repo_canon_path), repo_val);
                }
            } else if (repo_id != NULL) {
                char idbuf[33 + 33];
                int wrote = gm_snprintf(idbuf, sizeof(idbuf), "%016" PRIx64 "%016" PRIx64,
                                        repo_id->hi, repo_id->lo);
                if (wrote > 0 && (size_t)wrote < sizeof(idbuf)) {
                    if (cfg->repo_hash_sha256) {
                        sha256_hex12((const uint8_t *)idbuf, (size_t)wrote,
                                     repo_val);
                    } else {
                        fnv1a64_hex12((const uint8_t *)idbuf, (size_t)wrote,
                                      repo_val);
                    }
                }
            }
        }
        if (repo_val[0] != '\0') {
            if (append_kv(out, out_size, &idx, "repo", repo_val) != GM_OK) {
                return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL, "tags overflow"));
            }
            ++count;
        }
    }

    if (cfg != NULL && cfg->extra_count > 0) {
        for (size_t i = 0; i < cfg->extra_count && count < MAX_TAGS_TOTAL; ++i) {
            if (append_kv(out, out_size, &idx, cfg->extras[i].key,
                          cfg->extras[i].value) != GM_OK) {
                return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL,
                                            "tags overflow"));
            }
            ++count;
        }
    }

    return gm_ok_void();
}
