/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_TESTS_SUPPORT_TEMP_REPO_HELPERS_H
#define GITMIND_TESTS_SUPPORT_TEMP_REPO_HELPERS_H

#include <stddef.h>

#include "gitmind/error.h"
#include "gitmind/ports/fs_temp_port.h"
#include "gitmind/result.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef gm_result_void_t (*gm_test_temp_repo_provider_fn)(const gm_fs_temp_port_t *port,
                                                          const char *component,
                                                          char *out_path,
                                                          size_t out_size);

extern gm_test_temp_repo_provider_fn gm_test_temp_repo_provider_storage;

gm_result_void_t gm_test_default_temp_repo_provider(const gm_fs_temp_port_t *port,
                                                     const char *component,
                                                     char *out_path,
                                                     size_t out_size);

void gm_test_set_temp_repo_dir_provider(gm_test_temp_repo_provider_fn provider);

gm_result_void_t gm_test_make_temp_repo_dir(const gm_fs_temp_port_t *port,
                                             const char *component,
                                             char *out_path,
                                             size_t out_size);

gm_result_void_t gm_test_cleanup_temp_repo_dir(const gm_fs_temp_port_t *port,
                                                const char *path);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_TESTS_SUPPORT_TEMP_REPO_HELPERS_H */
