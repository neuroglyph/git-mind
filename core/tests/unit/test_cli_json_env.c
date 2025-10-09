/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "gitmind/telemetry/internal/config.h"
#include "gitmind/ports/env_port.h"
#include "gitmind/util/memory.h"

typedef struct {
    const char *key;
    const char *value;
} test_env_entry_t;

typedef struct {
    const test_env_entry_t *entries;
    size_t count;
} test_env_ctx_t;

static gm_result_bool_t test_env_get(void *ctx, const char *key, char *buffer,
                                     size_t buffer_size) {
    test_env_ctx_t *env = (test_env_ctx_t *)ctx;
    if (env == NULL || key == NULL || buffer == NULL || buffer_size == 0) {
        return gm_err_bool(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "invalid env arguments"));
    }
    for (size_t i = 0; i < env->count; ++i) {
        if (env->entries[i].key != NULL &&
            strcmp(env->entries[i].key, key) == 0) {
            if (gm_strcpy_safe(buffer, buffer_size,
                               env->entries[i].value ? env->entries[i].value : "") != GM_OK) {
                return gm_err_bool(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL,
                                            "env buffer too small"));
            }
            return gm_ok_bool(true);
        }
    }
    return gm_ok_bool(false);
}

int main(void) {
    printf("test_cli_json_env... ");

    static const test_env_entry_t entries[] = {
        {.key = "GITMIND_LOG_FORMAT", .value = "json"},
    };
    test_env_ctx_t env_ctx = {
        .entries = entries,
        .count = sizeof(entries) / sizeof(entries[0]),
    };
    static const gm_env_port_vtbl_t TEST_ENV_VTBL = {
        .get = test_env_get,
    };
    gm_env_port_t fake_env = {
        .context = &env_ctx,
        .vtbl = &TEST_ENV_VTBL,
    };

    gm_telemetry_cfg_t cfg = {0};
    gm_result_void_t load_rc = gm_telemetry_cfg_load(&cfg, &fake_env);
    assert(load_rc.ok);
    assert(cfg.log_format == GM_LOG_FMT_JSON);
    printf("OK\n");
    return 0;
}
