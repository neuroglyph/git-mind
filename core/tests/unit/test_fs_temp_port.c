/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "gitmind/adapters/fs/posix_temp_adapter.h"
#include "core/tests/fakes/fs/fake_fs_temp_port.h"
#include "gitmind/error.h"
#include "gitmind/ports/fs_temp_port.h"
#include "gitmind/types.h"
#include "gitmind/util/memory.h"

static bool path_exists(const char *path) {
    struct stat st = {0};
    return (path != NULL && stat(path, &st) == 0);
}

int main(void) {
    /* Real adapter contract */
    gm_fs_temp_port_t port = {0};
    gm_posix_fs_state_t *state = NULL;
    void (*dispose)(gm_fs_temp_port_t *) = NULL;

    gm_result_void_t create_result =
        gm_posix_fs_temp_port_create(&port, &state, &dispose);
    assert(create_result.ok && "port creation should succeed");
    assert(state != NULL);
    assert(port.vtbl != NULL);

    char cwd[GM_PATH_MAX];
    assert(getcwd(cwd, sizeof(cwd)) != NULL);

    const char *canon = NULL;
    gm_result_void_t canon_result =
        gm_fs_temp_port_canonicalize(&port, cwd, &canon);
    assert(canon_result.ok);
    assert(canon != NULL);

    gm_repo_id_t repo_id = {0};
    gm_result_void_t repo_id_result = gm_repo_id_from_path(canon, &repo_id);
    assert(repo_id_result.ok);
    free((void *)canon);
    canon = NULL;

    gm_tempdir_t tempdir = {0};
    gm_result_void_t temp_result = gm_fs_temp_port_make_temp_dir(
        &port, repo_id, "cache", true, &tempdir);
    if (!temp_result.ok) {
        char *msg = gm_error_format(temp_result.u.err);
        fprintf(stderr, "make_temp_dir error: %s\n", msg ? msg : "<unknown>");
        free(msg);
    }
    assert(temp_result.ok);
    assert(tempdir.path != NULL);
    assert(path_exists(tempdir.path));

    char tempdir_real[GM_PATH_MAX];
    assert(gm_strcpy_safe(tempdir_real, sizeof(tempdir_real), tempdir.path) == 0);

    char logical_input[GM_PATH_MAX];
    int logical_status = snprintf(logical_input, sizeof(logical_input),
                                  "%s//./nested/..", tempdir_real);
    assert(logical_status > 0 && logical_status < (int)sizeof(logical_input));
    const char *logical_out = NULL;
    gm_result_void_t logical_result =
        gm_fs_temp_port_canonicalize(&port, logical_input, &logical_out);
    assert(logical_result.ok);
    assert(strcmp(logical_out, tempdir_real) == 0);
    free((void *)logical_out);
    logical_out = NULL;

    gm_fs_canon_opts_t existing_opts = {.mode = GM_FS_CANON_PHYSICAL_EXISTING};
    const char *physical_out = NULL;
    gm_result_void_t physical_result = gm_fs_temp_port_canonicalize_ex(
        &port, tempdir_real, existing_opts, &physical_out);
    assert(physical_result.ok);
    assert(strcmp(physical_out, tempdir_real) == 0);
    free((void *)physical_out);
    physical_out = NULL;

    char create_target[GM_PATH_MAX];
    int create_status = snprintf(create_target, sizeof(create_target),
                                 "%s/new-entry", tempdir_real);
    assert(create_status > 0 && create_status < (int)sizeof(create_target));
    const char *create_out = NULL;
    gm_fs_canon_opts_t create_opts = {.mode = GM_FS_CANON_PHYSICAL_CREATE_OK};
    gm_result_void_t create_ok_result = gm_fs_temp_port_canonicalize_ex(
        &port, create_target, create_opts, &create_out);
    assert(create_ok_result.ok);
    assert(strstr(create_out, "/new-entry") != NULL);
    free((void *)create_out);
    create_out = NULL;

    const char *missing_out = NULL;
    gm_fs_canon_opts_t missing_opts = {.mode = GM_FS_CANON_PHYSICAL_EXISTING};
    gm_result_void_t missing_res = gm_fs_temp_port_canonicalize_ex(
        &port, "/definitely-not-present", missing_opts, &missing_out);
    assert(!missing_res.ok);
    if (missing_res.u.err != NULL) {
        assert(missing_res.u.err->code == GM_ERR_NOT_FOUND);
        gm_error_free(missing_res.u.err);
    }

    char marker_path[GM_PATH_MAX];
    int marker_status = snprintf(marker_path, sizeof(marker_path), "%s/marker",
                                 tempdir_real);
    assert(marker_status > 0 && marker_status < (int)sizeof(marker_path));
    FILE *marker = fopen(marker_path, "w");
    assert(marker != NULL);
    fputs("ok", marker);
    fclose(marker);
    assert(path_exists(marker_path));

    gm_result_void_t remove_result =
        gm_fs_temp_port_remove_tree(&port, tempdir_real);
    assert(remove_result.ok);
    assert(!path_exists(tempdir_real));

    if (dispose != NULL) {
        dispose(&port);
    }

    /* Fake adapter parity */
    gm_fake_fs_temp_port_t fake = {0};
    gm_result_void_t fake_init =
        gm_fake_fs_temp_port_init(&fake, "/fake/tmp", "/fake/state");
    assert(fake_init.ok);

    const char *fake_base = NULL;
    gm_result_void_t fake_base_res =
        gm_fs_temp_port_base_dir(&fake.port, GM_FS_BASE_TEMP, true, &fake_base);
    assert(fake_base_res.ok);
    assert(strcmp(fake_base, "/fake/tmp") == 0);

    gm_tempdir_t fake_dir = {0};
    gm_repo_id_t fake_repo = {.hi = 1, .lo = 2};
    gm_result_void_t fake_temp_res = gm_fs_temp_port_make_temp_dir(
        &fake.port, fake_repo, "component", true, &fake_dir);
    assert(fake_temp_res.ok);

    gm_fs_canon_opts_t fake_opts = {.mode = GM_FS_CANON_LOGICAL};
    const char *fake_logical = NULL;
    gm_result_void_t fake_logical_res = gm_fs_temp_port_canonicalize_ex(
        &fake.port, fake_dir.path, fake_opts, &fake_logical);
    assert(fake_logical_res.ok);
    assert(strcmp(fake_logical, fake_dir.path) == 0);
    free((void *)fake_logical);
    fake_logical = NULL;

    gm_fs_canon_opts_t fake_phys = {.mode = GM_FS_CANON_PHYSICAL_EXISTING};
    const char *fake_phys_out = NULL;
    gm_result_void_t fake_phys_res = gm_fs_temp_port_canonicalize_ex(
        &fake.port, fake_dir.path, fake_phys, &fake_phys_out);
    assert(fake_phys_res.ok);
    free((void *)fake_phys_out);
    fake_phys_out = NULL;

    gm_result_void_t fake_remove =
        gm_fs_temp_port_remove_tree(&fake.port, fake_dir.path);
    assert(fake_remove.ok);

    const char *fake_missing = NULL;
    gm_result_void_t fake_missing_res = gm_fs_temp_port_canonicalize_ex(
        &fake.port, "/fake/tmp/missing", fake_phys, &fake_missing);
    assert(!fake_missing_res.ok);
    if (fake_missing_res.u.err != NULL) {
        assert(fake_missing_res.u.err->code == GM_ERR_NOT_FOUND);
        gm_error_free(fake_missing_res.u.err);
    }

    gm_fake_fs_temp_port_dispose(&fake);
    return 0;
}
