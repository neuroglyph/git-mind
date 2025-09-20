/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/ports/env_port.h"

GM_NODISCARD gm_result_bool_t gm_env_get(const gm_env_port_t *port, const char *key,
                                         char *buffer, size_t buffer_size) {
    if (port == NULL || port->vtbl == NULL || port->vtbl->get == NULL) {
        return gm_err_bool(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "env port missing get"));
    }
    return port->vtbl->get(port->context, key, buffer, buffer_size);
}
