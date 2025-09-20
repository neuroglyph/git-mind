/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_PORTS_ENV_PORT_H
#define GITMIND_PORTS_ENV_PORT_H

#include <stddef.h>

#include "gitmind/error.h"
#include "gitmind/result.h"

#ifdef __cplusplus
extern "C" {
#endif

struct gm_env_port_vtbl;

typedef struct gm_env_port {
    void *context;
    const struct gm_env_port_vtbl *vtbl;
} gm_env_port_t;

typedef gm_result_bool_t (*gm_env_get_fn)(void *ctx, const char *key, char *buffer,
                                          size_t buffer_size);

typedef struct gm_env_port_vtbl {
    gm_env_get_fn get;
} gm_env_port_vtbl_t;

GM_NODISCARD gm_result_bool_t gm_env_get(const gm_env_port_t *port, const char *key,
                                         char *buffer, size_t buffer_size);

GM_NODISCARD const gm_env_port_t *gm_env_port_system(void);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_PORTS_ENV_PORT_H */
