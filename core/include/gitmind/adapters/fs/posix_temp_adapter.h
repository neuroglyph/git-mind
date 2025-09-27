/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_ADAPTERS_FS_POSIX_TEMP_ADAPTER_H
#define GITMIND_ADAPTERS_FS_POSIX_TEMP_ADAPTER_H

#include "gitmind/ports/fs_temp_port.h"
#include "gitmind/result.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gm_posix_fs_state gm_posix_fs_state_t;

gm_result_void_t gm_posix_fs_temp_port_create(gm_fs_temp_port_t *out_port,
                                              gm_posix_fs_state_t **out_state,
                                              void (**out_dispose)(gm_fs_temp_port_t *));

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_ADAPTERS_FS_POSIX_TEMP_ADAPTER_H */
