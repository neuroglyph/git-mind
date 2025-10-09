/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "core/tests/fakes/fs/fake_fs_temp_port.h"

#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "gitmind/error.h"
#include "gitmind/fs/path_utils.h"
#include "gitmind/result.h"
#include "gitmind/security/string.h"
#include "gitmind/types.h"
#include "gitmind/util/memory.h"
static bool fake_path_exists(const gm_fake_fs_temp_port_t *fake,
                             const char *path) {
    for (size_t i = 0; i < fake->created_count; ++i) {
        if (strcmp(fake->created_paths[i], path) == 0) {
            return true;
        }
    }
    if (strcmp(path, fake->temp_root) == 0) {
        return true;
    }
    if (strcmp(path, fake->state_root) == 0) {
        return true;
    }
    return false;
}

static gm_result_void_t fake_dup_path(const char *src,
                                      const char **out_abs_path) {
    if (src == NULL || out_abs_path == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "fake canonicalize requires buffers"));
    }

    size_t len = strlen(src) + 1U;
    char *copy = (char *)malloc(len);
    if (copy == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_OUT_OF_MEMORY,
                                    "fake canonicalize allocation failed"));
    }

    if (gm_strcpy_safe(copy, len, src) != 0) {
        free(copy);
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                    "fake canonicalize copy truncated"));
    }

    *out_abs_path = copy;
    return gm_ok_void();
}

static gm_result_void_t fake_base_dir(void *self, gm_fs_base_t base, bool ensure,
                                      const char **out_abs_path) {
    gm_fake_fs_temp_port_t *fake = (gm_fake_fs_temp_port_t *)self;
    (void)ensure;

    if (base == GM_FS_BASE_TEMP) {
        *out_abs_path = fake->temp_root;
    } else {
        *out_abs_path = fake->state_root;
    }
    return gm_ok_void();
}

static gm_result_void_t fake_make_temp_dir(void *self, gm_repo_id_t repo,
                                           const char *component,
                                           bool suffix_random,
                                           gm_tempdir_t *out_dir) {
    gm_fake_fs_temp_port_t *fake = (gm_fake_fs_temp_port_t *)self;
    if (out_dir == NULL || component == NULL || component[0] == '\0') {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "fake temp dir requires component"));
    }

    char repo_segment[33];
    if (gm_snprintf(repo_segment, sizeof(repo_segment), "%016" PRIx64 "%016" PRIx64,
                    repo.hi, repo.lo) < 0) {
        return gm_err_void(GM_ERROR(GM_ERR_UNKNOWN, "failed to format repo id"));
    }

    if (fake->created_count >= (sizeof(fake->created_paths) /
                                sizeof(fake->created_paths[0]))) {
        return gm_err_void(GM_ERROR(GM_ERR_UNKNOWN, "fake temp dir table full"));
    }

    unsigned int suffix = suffix_random ? ++fake->counter : 0U;
    if (suffix_random) {
        if (gm_snprintf(fake->scratch, sizeof(fake->scratch),
                        "%s/%s/%s-%06u", fake->temp_root, repo_segment, component,
                        suffix) < 0) {
            return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                        "fake temp dir path too long"));
        }
    } else {
        if (gm_snprintf(fake->scratch, sizeof(fake->scratch),
                        "%s/%s/%s", fake->temp_root, repo_segment, component) < 0) {
            return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                        "fake temp dir path too long"));
        }
    }

    if (gm_strcpy_safe(fake->created_paths[fake->created_count],
                       sizeof(fake->created_paths[fake->created_count]),
                       fake->scratch) != 0) {
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                    "fake temp dir store overflow"));
    }
    fake->created_count++;
    out_dir->path = fake->created_paths[fake->created_count - 1];
    return gm_ok_void();
}

static gm_result_void_t fake_remove_tree(void *self, const char *abs_path) {
    gm_fake_fs_temp_port_t *fake = (gm_fake_fs_temp_port_t *)self;
    if (abs_path == NULL) {
        return gm_ok_void();
    }

    for (size_t i = 0; i < fake->created_count; ++i) {
        if (strcmp(fake->created_paths[i], abs_path) == 0) {
            for (size_t j = i + 1; j < fake->created_count; ++j) {
                if (gm_strcpy_safe(fake->created_paths[j - 1],
                                   sizeof(fake->created_paths[j - 1]),
                                   fake->created_paths[j]) != 0) {
                    break;
                }
            }
            fake->created_count--;
            break;
        }
    }
    return gm_ok_void();
}

static gm_result_void_t fake_path_join(void *self, gm_fs_base_t base,
                                       gm_repo_id_t repo,
                                       const char *component1,
                                       const char *component2,
                                       const char *component3,
                                       const char *component4,
                                       const char *component5,
                                       const char **out_abs_path) {
    gm_fake_fs_temp_port_t *fake = (gm_fake_fs_temp_port_t *)self;
    const char *root = (base == GM_FS_BASE_TEMP) ? fake->temp_root : fake->state_root;

    if (gm_strcpy_safe(fake->scratch, sizeof(fake->scratch), root) != 0) {
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                    "fake path join base overflow"));
    }

    char repo_segment[33];
    if (gm_snprintf(repo_segment, sizeof(repo_segment), "%016" PRIx64 "%016" PRIx64,
                    repo.hi, repo.lo) < 0) {
        return gm_err_void(GM_ERROR(GM_ERR_UNKNOWN, "failed to format repo id"));
    }

    size_t len = strlen(fake->scratch);
    GM_TRY(gm_fs_path_basename_append(fake->scratch, sizeof(fake->scratch), &len,
                                      repo_segment));

    const char *segments[] = {component1, component2, component3, component4,
                              component5};
    for (size_t idx = 0; idx < sizeof(segments) / sizeof(segments[0]); ++idx) {
        const char *seg = segments[idx];
        if (seg == NULL || seg[0] == '\0') {
            continue;
        }
        GM_TRY(gm_fs_path_basename_append(fake->scratch, sizeof(fake->scratch), &len,
                                          seg));
    }

    *out_abs_path = fake->scratch;
    return gm_ok_void();
}

static gm_result_void_t fake_canonicalize(void *self, const char *abs_path_in,
                                          gm_fs_canon_opts_t opts,
                                          const char **out_abs_path) {
    gm_fake_fs_temp_port_t *fake = (gm_fake_fs_temp_port_t *)self;
    if (abs_path_in == NULL || out_abs_path == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "fake canonicalize requires buffers"));
    }

    switch (opts.mode) {
    case GM_FS_CANON_PHYSICAL_EXISTING: {
        GM_TRY(gm_fs_path_normalize_logical(abs_path_in, fake->scratch,
                                            sizeof(fake->scratch)));
        if (!fake_path_exists(fake, fake->scratch)) {
            return gm_err_void(GM_ERROR(GM_ERR_NOT_FOUND,
                                        "fake path not found"));
        }
        return fake_dup_path(fake->scratch, out_abs_path);
    }
    case GM_FS_CANON_PHYSICAL_CREATE_OK: {
        char normalized[GM_PATH_MAX];
        GM_TRY(gm_fs_path_normalize_logical(abs_path_in, normalized,
                                            sizeof(normalized)));
        if (normalized[0] != '/') {
            return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                        "create-ok canonicalize requires absolute path"));
        }
        char parent[GM_PATH_MAX];
        GM_TRY(gm_fs_path_dirname(normalized, parent, sizeof(parent)));
        if (!fake_path_exists(fake, parent)) {
            return gm_err_void(GM_ERROR(GM_ERR_NOT_FOUND,
                                        "fake parent not found"));
        }
        const char *leaf = normalized;
        const char *slash = strrchr(normalized, '/');
        if (slash != NULL && slash[1] != '\0') {
            leaf = slash + 1;
        }
        char leaf_copy[GM_PATH_MAX];
        if (gm_strcpy_safe(leaf_copy, sizeof(leaf_copy), leaf) != 0) {
            return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                        "fake canonical basename overflow"));
        }
        size_t len = strlen(parent);
        if (gm_strcpy_safe(fake->scratch, sizeof(fake->scratch), parent) != 0) {
            return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                        "fake canonical path overflow"));
        }
        GM_TRY(gm_fs_path_basename_append(fake->scratch, sizeof(fake->scratch), &len,
                                          leaf_copy));
        return fake_dup_path(fake->scratch, out_abs_path);
    }
    case GM_FS_CANON_LOGICAL:
    default: {
        GM_TRY(gm_fs_path_normalize_logical(abs_path_in, fake->scratch,
                                            sizeof(fake->scratch)));
        return fake_dup_path(fake->scratch, out_abs_path);
    }
    }
}

static const gm_fs_temp_port_vtbl_t FAKE_VTBL = {
    .base_dir = fake_base_dir,
    .make_temp_dir = fake_make_temp_dir,
    .remove_tree = fake_remove_tree,
    .path_join_under_base = fake_path_join,
    .canonicalize_ex = fake_canonicalize,
};

GM_NODISCARD gm_result_void_t gm_fake_fs_temp_port_init(
    gm_fake_fs_temp_port_t *fake, const char *temp_root, const char *state_root) {
    if (fake == NULL) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, "fake fs port requires storage"));
    }

    memset(fake, 0, sizeof(*fake));
    fake->port.vtbl = &FAKE_VTBL;
    fake->port.self = fake;

    const char *temp_default = temp_root ? temp_root : "/fake/tmp";
    const char *state_default = state_root ? state_root : "/fake/state";

    if (gm_strcpy_safe(fake->temp_root, sizeof(fake->temp_root), temp_default) != 0) {
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                    "fake temp root overflow"));
    }
    if (gm_strcpy_safe(fake->state_root, sizeof(fake->state_root), state_default) != 0) {
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                    "fake state root overflow"));
    }

    return gm_ok_void();
}

void gm_fake_fs_temp_port_dispose(gm_fake_fs_temp_port_t *fake) {
    if (fake == NULL) {
        return;
    }
    fake->port.vtbl = NULL;
    fake->port.self = NULL;
    fake->created_count = 0;
}
