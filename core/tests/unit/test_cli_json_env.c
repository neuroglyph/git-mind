/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#ifndef _WIN32
extern int putenv(char *);
#endif

#include "gitmind/telemetry/internal/config.h"
#include "gitmind/ports/env_port.h"

int main(void) {
    printf("test_cli_json_env... ");
    /* Simulate --json flag by setting env var */
    char kv[] = "GITMIND_LOG_FORMAT=json"; /* putenv takes ownership of the pointer */
    assert(putenv(kv) == 0);
    gm_telemetry_cfg_t cfg = {0};
    assert(gm_telemetry_cfg_load(&cfg, gm_env_port_system()).ok);
    assert(cfg.log_format == GM_LOG_FMT_JSON);
    printf("OK\n");
    return 0;
}
