/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_FS_PATH_UTILS_H
#define GITMIND_FS_PATH_UTILS_H

#include <stddef.h>

#include "gitmind/result.h"

#ifdef __cplusplus
extern "C" {
#endif

GM_NODISCARD gm_result_void_t gm_fs_path_normalize_logical(const char *input,
                                                           char *output,
                                                           size_t output_size);

GM_NODISCARD gm_result_void_t gm_fs_path_dirname(const char *input,
                                                 char *output,
                                                 size_t output_size);

GM_NODISCARD gm_result_void_t gm_fs_path_basename_append(char *base_io,
                                                         size_t buffer_size,
                                                         size_t *inout_len,
                                                         const char *source_path);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_FS_PATH_UTILS_H */
