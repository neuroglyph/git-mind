/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_TESTS_FAKES_FS_FAKE_FS_TEMP_PORT_H
#define GITMIND_TESTS_FAKES_FS_FAKE_FS_TEMP_PORT_H

#include <stddef.h>

#include "gitmind/ports/fs_temp_port.h"
#include "gitmind/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gm_fake_fs_temp_port {
    gm_fs_temp_port_t port;
    char temp_root[GM_PATH_MAX];
    char state_root[GM_PATH_MAX];
    unsigned int counter;
    char created_paths[64][GM_PATH_MAX];
    size_t created_count;
    char scratch[GM_PATH_MAX];
} gm_fake_fs_temp_port_t;

GM_NODISCARD gm_result_void_t gm_fake_fs_temp_port_init(
    gm_fake_fs_temp_port_t *fake, const char *temp_root, const char *state_root);

void gm_fake_fs_temp_port_dispose(gm_fake_fs_temp_port_t *fake);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_TESTS_FAKES_FS_FAKE_FS_TEMP_PORT_H */
