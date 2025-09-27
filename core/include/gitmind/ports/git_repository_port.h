/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_PORTS_GIT_REPOSITORY_PORT_H
#define GITMIND_PORTS_GIT_REPOSITORY_PORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file git_repository_port.h
 * @brief Outbound port for repository-scoped Git operations used by cache/journal services.
 */

typedef enum {
    GM_GIT_REPOSITORY_PATH_GITDIR,
    GM_GIT_REPOSITORY_PATH_WORKDIR,
} gm_git_repository_path_kind_t;

typedef struct {
    bool has_target;
    gm_oid_t oid;
    uint64_t commit_time;
    char oid_hex[GM_OID_HEX_CHARS + 1];
} gm_git_reference_tip_t;

typedef struct {
    const gm_oid_t *tree_oid;
    const char *message;
    const gm_oid_t *parents;
    size_t parent_count;
} gm_git_commit_spec_t;

typedef struct {
    const char *ref_name;
    const gm_oid_t *target_oid;
    const char *log_message;
    bool force;
} gm_git_reference_update_spec_t;

typedef int (*gm_git_commit_visit_cb)(const gm_oid_t *commit_oid, void *userdata);

typedef struct gm_git_repository_port_vtbl gm_git_repository_port_vtbl_t;
typedef struct gm_git_repository_port {
    const gm_git_repository_port_vtbl_t *vtbl;
    void *self;
} gm_git_repository_port_t;

typedef struct gm_git_repository_port_vtbl {
    gm_result_void_t (*repository_path)(void *self,
                                        gm_git_repository_path_kind_t kind,
                                        char *out_buffer, size_t buffer_size);
    gm_result_void_t (*head_branch)(void *self, char *out_name,
                                    size_t out_name_size);
    gm_result_void_t (*build_tree_from_directory)(void *self, const char *dir_path,
                                                  gm_oid_t *out_tree_oid);
    gm_result_void_t (*reference_tip)(void *self, const char *ref_name,
                                      gm_git_reference_tip_t *out_tip);
    gm_result_void_t (*reference_glob_latest)(void *self, const char *pattern,
                                              gm_git_reference_tip_t *out_tip);
    gm_result_void_t (*commit_read_blob)(void *self, const gm_oid_t *commit_oid,
                                         const char *path, uint8_t **out_data,
                                         size_t *out_size);
    gm_result_void_t (*commit_read_message)(void *self,
                                            const gm_oid_t *commit_oid,
                                            char **out_message);
    void (*commit_message_dispose)(void *self, char *message);
    gm_result_void_t (*walk_commits)(void *self, const char *ref_name,
                                     gm_git_commit_visit_cb visit_callback,
                                     void *userdata);
    gm_result_void_t (*commit_tree_size)(void *self, const gm_oid_t *commit_oid,
                                         uint64_t *out_size_bytes);
    gm_result_void_t (*commit_create)(void *self,
                                      const gm_git_commit_spec_t *spec,
                                      gm_oid_t *out_commit_oid);
    gm_result_void_t (*reference_update)(void *self,
                                         const gm_git_reference_update_spec_t *spec);
    gm_result_void_t (*resolve_blob_at_head)(void *self, const char *path,
                                             gm_oid_t *out_blob_oid);
    gm_result_void_t (*resolve_blob_at_commit)(void *self,
                                               const gm_oid_t *commit_oid,
                                               const char *path,
                                               gm_oid_t *out_blob_oid);
} gm_git_repository_port_vtbl_t;

GM_NODISCARD static inline gm_result_void_t
    gm_git_repository_port_repository_path(const gm_git_repository_port_t *port,
                                           gm_git_repository_path_kind_t kind,
                                           char *out_buffer, size_t buffer_size) {
    if (port == NULL || port->vtbl == NULL ||
        port->vtbl->repository_path == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_STATE,
                                    "git repository port missing repository_path"));
    }
    return port->vtbl->repository_path(port->self, kind, out_buffer, buffer_size);
}

GM_NODISCARD static inline gm_result_void_t
    gm_git_repository_port_head_branch(const gm_git_repository_port_t *port,
                                       char *out_name, size_t out_name_size) {
    if (port == NULL || port->vtbl == NULL || port->vtbl->head_branch == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_STATE,
                                    "git repository port missing head_branch"));
    }
    return port->vtbl->head_branch(port->self, out_name, out_name_size);
}

GM_NODISCARD static inline gm_result_void_t
    gm_git_repository_port_build_tree_from_directory(
        const gm_git_repository_port_t *port, const char *dir_path,
        gm_oid_t *out_tree_oid) {
    if (port == NULL || port->vtbl == NULL ||
        port->vtbl->build_tree_from_directory == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_STATE,
                                    "git repository port missing build_tree_from_directory"));
    }
    return port->vtbl->build_tree_from_directory(port->self, dir_path,
                                                 out_tree_oid);
}

GM_NODISCARD static inline gm_result_void_t
    gm_git_repository_port_reference_tip(const gm_git_repository_port_t *port,
                                         const char *ref_name,
                                         gm_git_reference_tip_t *out_tip) {
    if (port == NULL || port->vtbl == NULL || port->vtbl->reference_tip == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_STATE,
                                    "git repository port missing reference_tip"));
    }
    return port->vtbl->reference_tip(port->self, ref_name, out_tip);
}

GM_NODISCARD static inline gm_result_void_t
    gm_git_repository_port_reference_glob_latest(
        const gm_git_repository_port_t *port, const char *pattern,
        gm_git_reference_tip_t *out_tip) {
    if (port == NULL || port->vtbl == NULL ||
        port->vtbl->reference_glob_latest == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_STATE,
                                    "git repository port missing reference_glob_latest"));
    }
    return port->vtbl->reference_glob_latest(port->self, pattern, out_tip);
}

GM_NODISCARD static inline gm_result_void_t
    gm_git_repository_port_commit_read_blob(const gm_git_repository_port_t *port,
                                            const gm_oid_t *commit_oid,
                                            const char *path, uint8_t **out_data,
                                            size_t *out_size) {
    if (port == NULL || port->vtbl == NULL ||
        port->vtbl->commit_read_blob == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_STATE,
                                    "git repository port missing commit_read_blob"));
    }
    return port->vtbl->commit_read_blob(port->self, commit_oid, path, out_data,
                                        out_size);
}

GM_NODISCARD static inline gm_result_void_t
    gm_git_repository_port_commit_read_message(
        const gm_git_repository_port_t *port, const gm_oid_t *commit_oid,
        char **out_message) {
    if (port == NULL || port->vtbl == NULL ||
        port->vtbl->commit_read_message == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_STATE,
                                    "git repository port missing commit_read_message"));
    }
    return port->vtbl->commit_read_message(port->self, commit_oid, out_message);
}

static inline void gm_git_repository_port_commit_message_dispose(
    const gm_git_repository_port_t *port, char *message) {
    if (port == NULL || port->vtbl == NULL ||
        port->vtbl->commit_message_dispose == NULL) {
        free(message);
        return;
    }
    port->vtbl->commit_message_dispose(port->self, message);
}

GM_NODISCARD static inline gm_result_void_t
    gm_git_repository_port_walk_commits(const gm_git_repository_port_t *port,
                                        const char *ref_name,
                                        gm_git_commit_visit_cb visit_callback,
                                        void *userdata) {
    if (port == NULL || port->vtbl == NULL ||
        port->vtbl->walk_commits == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_STATE,
                                    "git repository port missing walk_commits"));
    }
    return port->vtbl->walk_commits(port->self, ref_name, visit_callback,
                                    userdata);
}

GM_NODISCARD static inline gm_result_void_t
    gm_git_repository_port_commit_tree_size(const gm_git_repository_port_t *port,
                                            const gm_oid_t *commit_oid,
                                            uint64_t *out_size_bytes) {
    if (port == NULL || port->vtbl == NULL ||
        port->vtbl->commit_tree_size == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_STATE,
                                    "git repository port missing commit_tree_size"));
    }
    return port->vtbl->commit_tree_size(port->self, commit_oid, out_size_bytes);
}

GM_NODISCARD static inline gm_result_void_t
    gm_git_repository_port_commit_create(const gm_git_repository_port_t *port,
                                         const gm_git_commit_spec_t *spec,
                                         gm_oid_t *out_commit_oid) {
    if (port == NULL || port->vtbl == NULL || port->vtbl->commit_create == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_STATE,
                                    "git repository port missing commit_create"));
    }
    return port->vtbl->commit_create(port->self, spec, out_commit_oid);
}

GM_NODISCARD static inline gm_result_void_t
    gm_git_repository_port_reference_update(const gm_git_repository_port_t *port,
                                            const gm_git_reference_update_spec_t *spec) {
    if (port == NULL || port->vtbl == NULL ||
        port->vtbl->reference_update == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_STATE,
                                    "git repository port missing reference_update"));
    }
    return port->vtbl->reference_update(port->self, spec);
}

GM_NODISCARD static inline gm_result_void_t
    gm_git_repository_port_resolve_blob_at_head(
        const gm_git_repository_port_t *port, const char *path,
        gm_oid_t *out_blob_oid) {
    if (port == NULL || port->vtbl == NULL ||
        port->vtbl->resolve_blob_at_head == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_STATE,
                                    "git repository port missing resolve_blob_at_head"));
    }
    return port->vtbl->resolve_blob_at_head(port->self, path, out_blob_oid);
}

GM_NODISCARD static inline gm_result_void_t
    gm_git_repository_port_resolve_blob_at_commit(
        const gm_git_repository_port_t *port, const gm_oid_t *commit_oid,
        const char *path, gm_oid_t *out_blob_oid) {
    if (port == NULL || port->vtbl == NULL ||
        port->vtbl->resolve_blob_at_commit == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_STATE,
                                    "git repository port missing resolve_blob_at_commit"));
    }
    return port->vtbl->resolve_blob_at_commit(port->self, commit_oid, path,
                                              out_blob_oid);
}

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_PORTS_GIT_REPOSITORY_PORT_H */
