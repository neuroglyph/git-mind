/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/telemetry/internal/config.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if !defined(_WIN32)
#include <strings.h>
#endif

#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/security/string.h"
#include "gitmind/util/memory.h"
#include "gitmind/ports/env_port.h"
#include "gitmind/ports/logger_port.h"
#include "gitmind/security/memory.h"
#include "gitmind/ports/fs_temp_port.h"
#include "gitmind/types.h"
#include "gitmind/crypto/backend.h"
#include "gitmind/crypto/sha256.h"

static const size_t TelemetryMaxTags = 5U;
static const size_t TelemetryRepoIdHexPartLen = 16U;
static const size_t TelemetryRepoIdHexStrLen =
    TelemetryRepoIdHexPartLen * 2U;
static const size_t TelemetryRepoIdBufferLen =
    TelemetryRepoIdHexStrLen + 1U;
static const size_t TelemetryExtraBufferLen = 256U;
static const size_t TelemetryHashHexDigits = 12U;
static const size_t TelemetryTotalFnvNibbles = 16U;
static const unsigned TelemetryHexDigitMask = 0x0FU;
static const size_t TelemetryNibbleWidth = 4U;
static const size_t TelemetrySha256PrefixBytes = 6U;
static const uint64_t TelemetryFnvOffsetBasis = 0xcbf29ce484222325ULL;
static const uint64_t TelemetryFnvPrime = 0x100000001b3ULL;
static const char TelemetryHexDigits[] = "0123456789abcdef";

#if defined(_WIN32)
static int gm_ascii_casecmp(const char *lhs, const char *rhs) {
    return _stricmp(lhs, rhs);
}
#else
static int gm_ascii_casecmp(const char *lhs, const char *rhs) {
    return strcasecmp(lhs, rhs);
}
#endif

static bool parse_bool_default_true(const char *value) {
    if (value == NULL || value[0] == '\0') {
        return true;
    }
    if (strcmp(value, "0") == 0) {
        return false;
    }
    if (gm_ascii_casecmp(value, "false") == 0) {
        return false;
    }
    if (gm_ascii_casecmp(value, "off") == 0) {
        return false;
    }
    if (gm_ascii_casecmp(value, "no") == 0) {
        return false;
    }
    return true;
}

static gm_repo_tag_mode_t parse_repo_tag_mode(const char *mode_value) {
    if (mode_value == NULL || mode_value[0] == '\0') {
        return GM_REPO_TAG_OFF;
    }
    if (gm_ascii_casecmp(mode_value, "off") == 0) {
        return GM_REPO_TAG_OFF;
    }
    if (gm_ascii_casecmp(mode_value, "hash") == 0) {
        return GM_REPO_TAG_HASH;
    }
    if (gm_ascii_casecmp(mode_value, "plain") == 0) {
        return GM_REPO_TAG_PLAIN;
    }
    return GM_REPO_TAG_OFF;
}

static gm_log_level_t parse_log_level(const char *level_value) {
    if (level_value == NULL || level_value[0] == '\0') {
        return GM_LOG_INFO;
    }
    if (gm_ascii_casecmp(level_value, "DEBUG") == 0) {
        return GM_LOG_DEBUG;
    }
    if (gm_ascii_casecmp(level_value, "INFO") == 0) {
        return GM_LOG_INFO;
    }
    if (gm_ascii_casecmp(level_value, "WARN") == 0) {
        return GM_LOG_WARN;
    }
    if (gm_ascii_casecmp(level_value, "ERROR") == 0) {
        return GM_LOG_ERROR;
    }
    return GM_LOG_INFO;
}

static gm_log_format_t parse_log_format(const char *format_value) {
    if (format_value == NULL || format_value[0] == '\0') {
        return GM_LOG_FMT_TEXT;
    }
    if (gm_ascii_casecmp(format_value, "text") == 0) {
        return GM_LOG_FMT_TEXT;
    }
    if (gm_ascii_casecmp(format_value, "json") == 0) {
        return GM_LOG_FMT_JSON;
    }
    return GM_LOG_FMT_TEXT;
}

static bool parse_hash_algo_sha256(const char *hash_value) {
    if (hash_value == NULL || hash_value[0] == '\0') {
        return false; /* default fnv */
    }
    if (gm_ascii_casecmp(hash_value, "sha256") == 0) {
        return true;
    }
    if (gm_ascii_casecmp(hash_value, "fnv") == 0) {
        return false;
    }
    return false;
}

static void fnv1a64_hex12(const uint8_t *data, size_t length, char *out_hex);
static void sha256_hex12(const uint8_t *data, size_t length, char *out_hex);

static void format_repo_hash_bytes(const gm_telemetry_cfg_t *cfg,
                                   const uint8_t *src, size_t length,
                                   char *out) {
    if (src == NULL || length == 0U || out == NULL) {
        if (out != NULL) {
            out[0] = '\0';
        }
        return;
    }
    bool use_sha256 = false;
    if (cfg != NULL) {
        use_sha256 = cfg->repo_hash_sha256;
    }
    if (use_sha256) {
        sha256_hex12(src, length, out);
    } else {
        fnv1a64_hex12(src, length, out);
    }
}

static void format_repo_hash_from_str(const gm_telemetry_cfg_t *cfg,
                                      const char *str, char *out) {
    if (str == NULL) {
        if (out != NULL) {
            out[0] = '\0';
        }
        return;
    }
    format_repo_hash_bytes(cfg, (const uint8_t *)str, strlen(str), out);
}

static void format_repo_hash_from_id(const gm_telemetry_cfg_t *cfg,
                                     const gm_repo_id_t *repo_id,
                                     char *out) {
    if (repo_id == NULL) {
        if (out != NULL) {
            out[0] = '\0';
        }
        return;
    }
    char idbuf[TelemetryRepoIdBufferLen];
    int wrote = gm_snprintf(idbuf, sizeof(idbuf), "%016" PRIx64 "%016" PRIx64,
                            repo_id->hi, repo_id->lo);
    if (wrote >= 0 && (size_t)wrote < sizeof(idbuf)) {
        format_repo_hash_bytes(cfg, (const uint8_t *)idbuf,
                               (size_t)wrote, out);
        return;
    }
    if (out != NULL) {
        out[0] = '\0';
    }
}

static bool is_key_char(char character) {
    return (bool)((character == '_') || (character == '-') ||
                  (character >= 'a' && character <= 'z') ||
                  (character >= '0' && character <= '9'));
}

static bool is_value_char(char character) {
    return (bool)((character >= 'a' && character <= 'z') ||
                  (character >= 'A' && character <= 'Z') ||
                  (character >= '0' && character <= '9') ||
                  (character == '_') || (character == '-') ||
                  (character == '.') || (character == ':') ||
                  (character == '@') || (character == '/'));
}

static bool validate_key(const char *key) {
    if (key == NULL || key[0] == '\0') {
        return false;
    }
    size_t key_length = strlen(key);
    size_t key_capacity = sizeof(((gm_kv_pair_t *)0)->key) - 1U;
    if (key_length > key_capacity) {
        return false;
    }
    for (size_t index = 0; index < key_length; ++index) {
        if (!is_key_char(key[index])) {
            return false;
        }
    }
    return true;
}

static bool validate_value(const char *value) {
    if (value == NULL) {
        return false;
    }
    size_t value_length = strlen(value);
    size_t value_capacity = sizeof(((gm_kv_pair_t *)0)->value) - 1U;
    if (value_length == 0U || value_length > value_capacity) {
        return false;
    }
    for (size_t index = 0; index < value_length; ++index) {
        if (!is_value_char(value[index])) {
            return false;
        }
    }
    return true;
}

static void add_extra_if_valid(gm_telemetry_cfg_t *cfg, const char *key,
                               const char *value, bool *dropped) {
    if (cfg == NULL || dropped == NULL) {
        return;
    }
    size_t capacity = sizeof(cfg->extras) / sizeof(cfg->extras[0]);
    if (cfg->extra_count >= capacity) {
        *dropped = true;
        return;
    }
    if (!validate_key(key) || !validate_value(value)) {
        *dropped = true;
        return;
    }
    gm_kv_pair_t *pair = &cfg->extras[cfg->extra_count];
    if (gm_strcpy_safe(pair->key, sizeof(pair->key), key) != GM_OK) {
        gm_memset_safe(pair, sizeof(*pair), 0, sizeof(*pair));
        *dropped = true;
        return;
    }
    if (gm_strcpy_safe(pair->value, sizeof(pair->value), value) != GM_OK) {
        gm_memset_safe(pair, sizeof(*pair), 0, sizeof(*pair));
        *dropped = true;
        return;
    }
    cfg->extra_count++;
}

static void parse_extras(gm_telemetry_cfg_t *cfg, const char *csv) {
    if (cfg == NULL || csv == NULL || csv[0] == '\0') {
        return;
    }
    char buffer[TelemetryExtraBufferLen];
    int copy_status = gm_strcpy_safe(buffer, sizeof(buffer), csv);
    if (copy_status != GM_OK) {
        cfg->extras_dropped = true;
        return;
    }

    char *save_state = NULL;
    for (char *token = strtok_r(buffer, ",", &save_state); token != NULL;
         token = strtok_r(NULL, ",", &save_state)) {
        while (*token == ' ') {
            ++token;
        }
        char *equals_sign = strchr(token, '=');
        if (equals_sign == NULL) {
            cfg->extras_dropped = true;
            continue;
        }
        *equals_sign = '\0';
        const char *key = token;
        const char *value = equals_sign + 1;
        bool dropped = false;
        add_extra_if_valid(cfg, key, value, &dropped);
        if (dropped) {
            cfg->extras_dropped = true;
        }
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
    char env_buffer[TelemetryExtraBufferLen];

    if (gm_env_get(src, "GITMIND_METRICS_ENABLED", env_buffer,
                   sizeof env_buffer)
            .ok) {
        out->metrics_enabled = parse_bool_default_true(env_buffer);
    }
    if (gm_env_get(src, "GITMIND_METRICS_BRANCH_TAG", env_buffer,
                   sizeof env_buffer)
            .ok) {
        out->tag_branch = parse_bool_default_true(env_buffer);
    }
    if (gm_env_get(src, "GITMIND_METRICS_MODE_TAG", env_buffer,
                   sizeof env_buffer)
            .ok) {
        out->tag_mode = parse_bool_default_true(env_buffer);
    }
    if (gm_env_get(src, "GITMIND_METRICS_REPO_TAG", env_buffer,
                   sizeof env_buffer)
            .ok) {
        out->repo_tag = parse_repo_tag_mode(env_buffer);
    }
    if (gm_env_get(src, "GITMIND_METRICS_REPO_HASH_ALGO", env_buffer,
                   sizeof env_buffer)
            .ok) {
        out->repo_hash_sha256 = parse_hash_algo_sha256(env_buffer);
    }
    if (gm_env_get(src, "GITMIND_METRICS_EXTRA_TAGS", env_buffer,
                   sizeof env_buffer)
            .ok) {
        parse_extras(out, env_buffer);
    }
    if (gm_env_get(src, "GITMIND_LOG_LEVEL", env_buffer,
                   sizeof env_buffer)
            .ok) {
        out->log_level = parse_log_level(env_buffer);
    }
    if (gm_env_get(src, "GITMIND_LOG_FORMAT", env_buffer,
                   sizeof env_buffer)
            .ok) {
        out->log_format = parse_log_format(env_buffer);
    }

    return gm_ok_void();
}

static int append_kv(char *out, size_t out_size, size_t *index,
                     const char *key, const char *value) {
    if (out == NULL || index == NULL) {
        return GM_ERR_INVALID_ARGUMENT;
    }
    if (key == NULL || value == NULL || key[0] == '\0' || value[0] == '\0') {
        return GM_OK;
    }
    if (*index >= out_size) {
        return GM_ERR_BUFFER_TOO_SMALL;
    }
    size_t available = out_size - *index;
    int wrote = gm_snprintf(out + *index, available, "%s%s=%s",
                            (*index > 0U) ? "," : "", key, value);
    if (wrote < 0) {
        return GM_ERR_BUFFER_TOO_SMALL;
    }
    size_t converted = (size_t)wrote;
    if (converted >= available) {
        return GM_ERR_BUFFER_TOO_SMALL;
    }
    *index += converted;
    return GM_OK;
}

static void fnv1a64_hex12(const uint8_t *data, size_t length, char *out_hex) {
    if (out_hex == NULL) {
        return;
    }
    if (data == NULL || length == 0U) {
        out_hex[0] = '\0';
        return;
    }

    uint64_t hash = TelemetryFnvOffsetBasis;
    for (size_t index = 0; index < length; ++index) {
        hash ^= (uint64_t)data[index];
        hash *= TelemetryFnvPrime;
    }

    for (size_t digit = 0; digit < TelemetryHashHexDigits; ++digit) {
        size_t nibble_index = TelemetryTotalFnvNibbles - 1U - digit;
        size_t shift = nibble_index * TelemetryNibbleWidth;
        unsigned nibble =
            (unsigned)((hash >> shift) & (uint64_t)TelemetryHexDigitMask);
        out_hex[digit] = TelemetryHexDigits[nibble];
    }
    out_hex[TelemetryHashHexDigits] = '\0';
}

static void free_error_if_present(gm_error_t *error) {
    if (error != NULL) {
        gm_error_free(error);
    }
}

static void dispose_crypto_context(gm_crypto_context_t *context) {
    gm_result_void_t dispose_result = gm_crypto_context_dispose(context);
    if (!dispose_result.ok) {
        free_error_if_present(dispose_result.u.err);
    }
}

static void sha256_hex12(const uint8_t *data, size_t length, char *out_hex) {
    if (out_hex == NULL) {
        return;
    }
    uint8_t digest[GM_SHA256_DIGEST_SIZE];
    gm_result_crypto_context_t context_result =
        gm_crypto_context_create(gm_crypto_backend_libsodium());
    if (!context_result.ok) {
        free_error_if_present(context_result.u.err);
        /* Fallback to FNV on any error */
        fnv1a64_hex12(data, length, out_hex);
        return;
    }
    gm_result_void_t hash_result =
        gm_sha256_with_context(&context_result.u.val, data, length, digest);
    if (!hash_result.ok) {
        free_error_if_present(hash_result.u.err);
        dispose_crypto_context(&context_result.u.val);
        fnv1a64_hex12(data, length, out_hex);
        return;
    }
    dispose_crypto_context(&context_result.u.val);
    for (size_t index = 0; index < TelemetrySha256PrefixBytes; ++index) {
        size_t offset = index * 2U;
        unsigned high_nibble =
            (unsigned)((digest[index] >> TelemetryNibbleWidth) &
                       TelemetryHexDigitMask);
        unsigned low_nibble =
            (unsigned)(digest[index] & TelemetryHexDigitMask);
        out_hex[offset] = TelemetryHexDigits[high_nibble];
        out_hex[offset + 1U] = TelemetryHexDigits[low_nibble];
    }
    out_hex[TelemetryHashHexDigits] = '\0';
}

typedef struct {
    char *buffer;
    size_t capacity;
    size_t index;
    size_t count;
} gm_tag_builder_t;

static bool is_present(const char *value) {
    return (bool)(value != NULL && value[0] != '\0');
}

static bool can_append_tag(const gm_tag_builder_t *builder) {
    return builder->count < TelemetryMaxTags;
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

static gm_result_void_t assign_plain_repo_tag(
    const gm_telemetry_cfg_t *cfg, const char *canonical_path,
    const gm_repo_id_t *repo_id, char *repo_value,
    size_t repo_value_size) {
    if (is_present(canonical_path)) {
        int copy_status =
            gm_strcpy_safe(repo_value, repo_value_size, canonical_path);
        if (copy_status == GM_OK) {
            return gm_ok_void();
        }
        format_repo_hash_from_str(cfg, canonical_path, repo_value);
        return gm_ok_void();
    }
    if (repo_id != NULL) {
        format_repo_hash_from_id(cfg, repo_id, repo_value);
    }
    return gm_ok_void();
}

static gm_result_void_t assign_hash_repo_tag(const gm_telemetry_cfg_t *cfg,
                                            const char *canonical_path,
                                            const gm_repo_id_t *repo_id,
                                            char *repo_value) {
    if (is_present(canonical_path)) {
        format_repo_hash_from_str(cfg, canonical_path, repo_value);
        return gm_ok_void();
    }
    if (repo_id != NULL) {
        format_repo_hash_from_id(cfg, repo_id, repo_value);
    }
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
        return assign_plain_repo_tag(cfg, canonical_path, repo_id,
                                     repo_value, repo_value_size);
    case GM_REPO_TAG_HASH:
        return assign_hash_repo_tag(cfg, canonical_path, repo_id, repo_value);
    case GM_REPO_TAG_OFF:
    default:
        return gm_ok_void();
    }
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

static gm_result_void_t append_branch_and_mode_tags(
    const gm_telemetry_cfg_t *cfg, const char *branch,
    const char *mode, gm_tag_builder_t *builder) {
    if (cfg == NULL) {
        return gm_ok_void();
    }
    if (cfg->tag_branch) {
        gm_result_void_t branch_result =
            builder_append_tag(builder, "branch", branch);
        if (!branch_result.ok) {
            return branch_result;
        }
    }
    if (cfg->tag_mode) {
        gm_result_void_t mode_result =
            builder_append_tag(builder, "mode", mode);
        if (!mode_result.ok) {
            return mode_result;
        }
    }
    return gm_ok_void();
}

static gm_result_void_t append_repo_tag_if_enabled(
    const gm_telemetry_cfg_t *cfg, const gm_telemetry_tag_context_t *ctx,
    gm_tag_builder_t *builder) {
    if (cfg == NULL || cfg->repo_tag == GM_REPO_TAG_OFF) {
        return gm_ok_void();
    }
    char repo_value[GM_PATH_MAX] = {0};
    gm_result_void_t repo_value_result =
        compute_repo_tag_value(cfg, ctx, repo_value, sizeof(repo_value));
    if (!repo_value_result.ok) {
        return repo_value_result;
    }
    return builder_append_tag(builder, "repo", repo_value);
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

    gm_result_void_t branch_mode_result =
        append_branch_and_mode_tags(cfg, branch, mode, &builder);
    if (!branch_mode_result.ok) {
        return branch_mode_result;
    }

    gm_result_void_t repo_result =
        append_repo_tag_if_enabled(cfg, ctx, &builder);
    if (!repo_result.ok) {
        return repo_result;
    }

    gm_result_void_t extras_result = append_extra_tags(cfg, &builder);
    if (!extras_result.ok) {
        return extras_result;
    }

    if (builder.index < builder.capacity) {
        builder.buffer[builder.index] = '\0';
    }

    return gm_ok_void();
}
