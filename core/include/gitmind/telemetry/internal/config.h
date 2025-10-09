/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_TELEMETRY_INTERNAL_CONFIG_H
#define GITMIND_TELEMETRY_INTERNAL_CONFIG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "gitmind/ports/env_port.h"
#include "gitmind/ports/logger_port.h"
#include "gitmind/ports/fs_temp_port.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GM_REPO_TAG_OFF = 0,
    GM_REPO_TAG_HASH = 1,
    GM_REPO_TAG_PLAIN = 2,
} gm_repo_tag_mode_t;

typedef enum {
    GM_LOG_FMT_TEXT = 0,
    GM_LOG_FMT_JSON = 1,
} gm_log_format_t;

typedef struct {
    char key[32];
    char value[64];
} gm_kv_pair_t;

typedef struct {
    /* Metrics toggles */
    bool metrics_enabled;
    bool tag_branch;
    bool tag_mode;
    gm_repo_tag_mode_t repo_tag;
    /* Repo hash algorithm when repo_tag==HASH */
    /* Values: "fnv" (default) or "sha256" */
    bool repo_hash_sha256; /* true => sha256, false => fnv */

    /* Up to 3 validated extras */
    size_t extra_count;
    gm_kv_pair_t extras[3];
    bool extras_dropped; /* true when some extras were invalid or over the cap */

    /* Logging preferences */
    gm_log_level_t log_level;
    gm_log_format_t log_format;
} gm_telemetry_cfg_t;

/*
 * Load telemetry configuration from the provided env port.
 * Defaults:
 *   - metrics_enabled=1, tag_branch=1, tag_mode=1, repo_tag=OFF
 *   - log_level=INFO, log_format=TEXT
 */
gm_result_void_t gm_telemetry_cfg_load(gm_telemetry_cfg_t *out,
                                       const gm_env_port_t *env);

/*
 * Build a comma-separated tag string using the configuration and provided
 * context. Hard-caps to 5 tags total. Keys/values are validated when loading
 * the configuration; this function only enforces capacity and formatting.
 *
 * - branch/mode are included when enabled in cfg and non-null.
 * - repo tag:
 *     HASH  => 12-hex short hash derived from canonical path or repo id
 *     PLAIN => canonical path string (bounded to value length)
 * - extras => appended up to remaining capacity
 */
gm_result_void_t gm_telemetry_build_tags(const gm_telemetry_cfg_t *cfg,
                                         const char *branch,
                                         const char *mode,
                                         const char *repo_canon_path,
                                         const gm_repo_id_t *repo_id,
                                         char *out, size_t out_size);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_TELEMETRY_INTERNAL_CONFIG_H */
