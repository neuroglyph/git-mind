/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/ports/env_port.h"
#include "gitmind/error.h"

#include <stdlib.h>

#include "gitmind/util/memory.h"

static gm_result_bool_t gm_env_system_get(void *ctx, const char *key, char *buffer,
                                          size_t buffer_size) {
    (void)ctx;

    if (key == NULL || buffer == NULL) {
        return gm_err_bool(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "env get requires key/buffer"));
    }
    if (buffer_size == 0) {
        return gm_err_bool(GM_ERROR(GM_ERR_INVALID_LENGTH, "env buffer must be non-zero"));
    }

    const char *value = getenv(key);
    if (value == NULL) {
        buffer[0] = '\0';
        return gm_ok_bool(false);
    }

    if (gm_strcpy_safe(buffer, buffer_size, value) != 0) {
        return gm_err_bool(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL, "env value truncated for %s", key));
    }

    return gm_ok_bool(true);
}

static const gm_env_port_vtbl_t GM_ENV_PORT_SYSTEM_VTBL = {
    .get = gm_env_system_get,
};

GM_NODISCARD const gm_env_port_t *gm_env_port_system(void) {
    static const gm_env_port_t port = {
        .context = NULL,
        .vtbl = &GM_ENV_PORT_SYSTEM_VTBL,
    };
    return &port;
}
