/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_TESTS_UNIT_SUPPORT_TEMP_REPO_HELPERS_H
#define GITMIND_TESTS_UNIT_SUPPORT_TEMP_REPO_HELPERS_H

#include <errno.h>
#include <stddef.h>
#include <string.h>

#ifdef _WIN32
#include <direct.h>
#define gm_test_getcwd _getcwd
#else
#include <unistd.h>
#define gm_test_getcwd getcwd
#endif

#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/util/memory.h"
#include "gitmind/types.h"
#include "gitmind/ports/fs_temp_port.h"

typedef gm_result_void_t (*gm_test_temp_repo_provider_fn)(const gm_fs_temp_port_t *port,
                                                         const char *component,
                                                         char *out_path,
                                                         size_t out_size);

static gm_result_void_t gm_test_default_temp_repo_provider(
    const gm_fs_temp_port_t *port, const char *component, char *out_path,
    size_t out_size);

static inline gm_test_temp_repo_provider_fn *
gm_test_temp_repo_provider_slot(void) {
    static gm_test_temp_repo_provider_fn provider = NULL;
    if (provider == NULL) {
        provider = gm_test_default_temp_repo_provider;
    }
    return &provider;
}

/**
 * Override the temporary repository provider used by unit tests.
 *
 * Passing `NULL` restores the default implementation, which uses the on-disk
 * filesystem port to make a temp directory derived from the current working
 * directory.
 */
static inline void
gm_test_set_temp_repo_dir_provider(gm_test_temp_repo_provider_fn provider) {
    gm_test_temp_repo_provider_fn *slot = gm_test_temp_repo_provider_slot();
    *slot = provider != NULL ? provider : gm_test_default_temp_repo_provider;
}

/**
 * Acquire a repository-scoped temporary directory using the filesystem port.
 *
 * Tests can override the implementation via
 * `gm_test_set_temp_repo_dir_provider` to route the helper through fakes or
 * in-memory adapters.
 */
GM_NODISCARD static inline gm_result_void_t
gm_test_make_temp_repo_dir(const gm_fs_temp_port_t *port,
                           const char *component,
                           char *out_path,
                           size_t out_size) {
    gm_test_temp_repo_provider_fn provider =
        *gm_test_temp_repo_provider_slot();
    return provider(port, component, out_path, out_size);
}

static gm_result_void_t gm_test_default_temp_repo_provider(
    const gm_fs_temp_port_t *port, const char *component, char *out_path,
    size_t out_size) {
    if (port == NULL || component == NULL || component[0] == '\0' ||
        out_path == NULL || out_size == 0) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "temp repo helper called with invalid arguments"));
    }

    char cwd[GM_PATH_MAX];
    if (gm_test_getcwd(cwd, sizeof(cwd)) == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_IO_FAILED,
                                    "getcwd failed: %s", strerror(errno)));
    }

    gm_repo_id_t repo_id = {0};
    GM_TRY(gm_repo_id_from_path(cwd, &repo_id));

    gm_tempdir_t temp_dir = {0};
    GM_TRY(gm_fs_temp_port_make_temp_dir(port, repo_id, component, true, &temp_dir));

    if (gm_strcpy_safe(out_path, out_size, temp_dir.path) != GM_OK) {
        return gm_err_void(
            GM_ERROR(GM_ERR_PATH_TOO_LONG, "temp dir path exceeds buffer size"));
    }

    return gm_ok_void();
}

#endif /* GITMIND_TESTS_UNIT_SUPPORT_TEMP_REPO_HELPERS_H */
