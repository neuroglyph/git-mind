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

/**
 * Acquire a repository-scoped temporary directory using the filesystem port.
 *
 * The helper derives the repo identifier from the current working directory so
 * that unit tests can reuse the same location schema that production paths use.
 *
 * @param port          Filesystem temp port providing the underlying adapter.
 * @param component     Short component name (no path separators).
 * @param out_path      Buffer that receives the absolute temp directory path.
 * @param out_size      Size of @p out_path in bytes.
 *
 * @return `gm_result_void_t` success/error result.
 */
GM_NODISCARD static inline gm_result_void_t
gm_test_make_temp_repo_dir(const gm_fs_temp_port_t *port,
                           const char *component,
                           char *out_path,
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
