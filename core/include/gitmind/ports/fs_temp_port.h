/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_PORTS_FS_TEMP_PORT_H
#define GITMIND_PORTS_FS_TEMP_PORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "gitmind/error.h"
#include "gitmind/result.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file fs_temp_port.h
 * @brief Filesystem port focused on repo-scoped temp directories and paths.
 */

typedef struct {
    uint64_t hi;
    uint64_t lo;
} gm_repo_id_t;

GM_NODISCARD gm_result_void_t gm_repo_id_from_path(const char *abs_repo_path,
                                                   gm_repo_id_t *out_id);

typedef enum {
    GM_FS_BASE_TEMP,
    GM_FS_BASE_STATE,
} gm_fs_base_t;

typedef struct {
    const char *path;
} gm_tempdir_t;

typedef enum {
    GM_FS_CANON_LOGICAL = 0,
    GM_FS_CANON_PHYSICAL_EXISTING = 1,
    GM_FS_CANON_PHYSICAL_CREATE_OK = 2,
} gm_fs_canon_mode_t;

typedef struct {
    gm_fs_canon_mode_t mode;
} gm_fs_canon_opts_t;

typedef struct gm_fs_temp_port_vtbl gm_fs_temp_port_vtbl_t;
typedef struct gm_fs_temp_port {
    const gm_fs_temp_port_vtbl_t *vtbl;
    void *self;
} gm_fs_temp_port_t;

typedef struct gm_fs_temp_port_vtbl {
    gm_result_void_t (*base_dir)(void *self, gm_fs_base_t base, bool ensure,
                                 const char **out_abs_path);
    gm_result_void_t (*make_temp_dir)(void *self, gm_repo_id_t repo,
                                      const char *component, bool suffix_random,
                                      gm_tempdir_t *out_dir);
    gm_result_void_t (*remove_tree)(void *self, const char *abs_path);
    gm_result_void_t (*path_join_under_base)(void *self, gm_fs_base_t base,
                                             gm_repo_id_t repo,
                                             const char *s1, const char *s2,
                                             const char *s3, const char *s4,
                                             const char *s5,
                                             const char **out_abs_path);
    gm_result_void_t (*canonicalize_ex)(void *self, const char *abs_path_in,
                                        gm_fs_canon_opts_t opts,
                                        const char **out_abs_path);
} gm_fs_temp_port_vtbl_t;

GM_NODISCARD static inline gm_result_void_t
    gm_fs_temp_port_base_dir(const gm_fs_temp_port_t *port, gm_fs_base_t base,
                             bool ensure, const char **out_abs_path) {
    if (port == NULL || port->vtbl == NULL || port->vtbl->base_dir == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_STATE,
                                    "filesystem port missing base_dir"));
    }
    return port->vtbl->base_dir(port->self, base, ensure, out_abs_path);
}

GM_NODISCARD static inline gm_result_void_t
    gm_fs_temp_port_make_temp_dir(const gm_fs_temp_port_t *port,
                                  gm_repo_id_t repo, const char *component,
                                  bool suffix_random, gm_tempdir_t *out_dir) {
    if (port == NULL || port->vtbl == NULL ||
        port->vtbl->make_temp_dir == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_STATE,
                                    "filesystem port missing make_temp_dir"));
    }
    return port->vtbl->make_temp_dir(port->self, repo, component, suffix_random,
                                     out_dir);
}

GM_NODISCARD static inline gm_result_void_t
    gm_fs_temp_port_remove_tree(const gm_fs_temp_port_t *port,
                                const char *abs_path) {
    if (port == NULL || port->vtbl == NULL ||
        port->vtbl->remove_tree == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_STATE,
                                    "filesystem port missing remove_tree"));
    }
    return port->vtbl->remove_tree(port->self, abs_path);
}

GM_NODISCARD static inline gm_result_void_t
    gm_fs_temp_port_path_join(const gm_fs_temp_port_t *port, gm_fs_base_t base,
                              gm_repo_id_t repo, const char *s1,
                              const char *s2, const char *s3, const char *s4,
                              const char *s5, const char **out_abs_path) {
    if (port == NULL || port->vtbl == NULL ||
        port->vtbl->path_join_under_base == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_STATE,
                                    "filesystem port missing path_join"));
    }
    return port->vtbl->path_join_under_base(port->self, base, repo, s1, s2, s3,
                                            s4, s5, out_abs_path);
}

GM_NODISCARD static inline gm_result_void_t
    gm_fs_temp_port_canonicalize_ex(const gm_fs_temp_port_t *port,
                                    const char *abs_path_in,
                                    gm_fs_canon_opts_t opts,
                                    const char **out_abs_path) {
    if (port == NULL || port->vtbl == NULL ||
        port->vtbl->canonicalize_ex == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_STATE,
                                    "filesystem port missing canonicalize"));
    }
    return port->vtbl->canonicalize_ex(port->self, abs_path_in, opts,
                                       out_abs_path);
}

GM_NODISCARD static inline gm_result_void_t
    gm_fs_temp_port_canonicalize(const gm_fs_temp_port_t *port,
                                 const char *abs_path_in,
                                 const char **out_abs_path) {
    gm_fs_canon_opts_t opts = {.mode = GM_FS_CANON_LOGICAL};
    return gm_fs_temp_port_canonicalize_ex(port, abs_path_in, opts,
                                           out_abs_path);
}

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_PORTS_FS_TEMP_PORT_H */
