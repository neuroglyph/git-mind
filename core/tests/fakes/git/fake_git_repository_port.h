/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_TESTS_FAKES_GIT_FAKE_GIT_REPOSITORY_PORT_H
#define GITMIND_TESTS_FAKES_GIT_FAKE_GIT_REPOSITORY_PORT_H

#include "gitmind/ports/git_repository_port.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GM_FAKE_GIT_MAX_REF_ENTRIES 8U
#define GM_FAKE_GIT_MAX_COMMITS_PER_REF 16U
#define GM_FAKE_GIT_MAX_BLOB_PATHS 32U

typedef struct gm_fake_git_commit_entry {
    gm_oid_t oid;
    char message[GM_FORMAT_BUFFER_SIZE];
    bool has_message;
} gm_fake_git_commit_entry_t;

typedef struct gm_fake_git_ref_entry {
    char ref_name[GM_FORMAT_BUFFER_SIZE];
    gm_fake_git_commit_entry_t commits[GM_FAKE_GIT_MAX_COMMITS_PER_REF];
    size_t commit_count;
    bool in_use;
} gm_fake_git_ref_entry_t;

typedef struct gm_fake_git_blob_entry {
    char path[GM_PATH_MAX];
    gm_oid_t oid;
    gm_oid_t commit_oid;
    bool has_commit;
    bool in_use;
} gm_fake_git_blob_entry_t;

typedef struct gm_fake_git_repository_port {
    gm_git_repository_port_t port;
    gm_git_reference_tip_t tip;
    char gitdir[GM_PATH_MAX];
    char workdir[GM_PATH_MAX];
    gm_oid_t next_tree_oid;
    gm_oid_t next_commit_oid;
    gm_result_void_t tree_result;
    gm_result_void_t commit_result;
    gm_result_void_t update_result;
    unsigned int counter;
    char last_commit_message[GM_FORMAT_BUFFER_SIZE];
    gm_oid_t last_commit_tree_oid;
    char last_update_ref[GM_PATH_MAX];
    char last_update_log[GM_FORMAT_BUFFER_SIZE];
    gm_oid_t last_update_target;
    char head_branch[GM_FORMAT_BUFFER_SIZE];
    gm_fake_git_ref_entry_t ref_entries[GM_FAKE_GIT_MAX_REF_ENTRIES];
    gm_fake_git_blob_entry_t blob_entries[GM_FAKE_GIT_MAX_BLOB_PATHS];
} gm_fake_git_repository_port_t;

GM_NODISCARD gm_result_void_t gm_fake_git_repository_port_init(
    gm_fake_git_repository_port_t *fake, const char *gitdir, const char *workdir);

void gm_fake_git_repository_port_dispose(gm_fake_git_repository_port_t *fake);

void gm_fake_git_repository_port_set_tip(gm_fake_git_repository_port_t *fake,
                                         const gm_git_reference_tip_t *tip);

GM_NODISCARD gm_result_void_t gm_fake_git_repository_port_set_head_branch(
    gm_fake_git_repository_port_t *fake, const char *branch_name);

void gm_fake_git_repository_port_clear_ref_commits(
    gm_fake_git_repository_port_t *fake);

GM_NODISCARD gm_result_void_t gm_fake_git_repository_port_add_ref_commit(
    gm_fake_git_repository_port_t *fake, const char *ref_name,
    const gm_oid_t *commit_oid, const char *message);

void gm_fake_git_repository_port_clear_blob_mappings(
    gm_fake_git_repository_port_t *fake);

GM_NODISCARD gm_result_void_t gm_fake_git_repository_port_add_blob_mapping(
    gm_fake_git_repository_port_t *fake, const char *path,
    const gm_oid_t *blob_oid);

GM_NODISCARD gm_result_void_t gm_fake_git_repository_port_add_commit_blob_mapping(
    gm_fake_git_repository_port_t *fake, const gm_oid_t *commit_oid,
    const char *path, const gm_oid_t *blob_oid);

void gm_fake_git_repository_port_set_next_tree(
    gm_fake_git_repository_port_t *fake, const gm_oid_t *oid,
    gm_result_void_t result);

void gm_fake_git_repository_port_set_next_commit(
    gm_fake_git_repository_port_t *fake, const gm_oid_t *oid,
    gm_result_void_t result);

void gm_fake_git_repository_port_set_update_result(
    gm_fake_git_repository_port_t *fake, gm_result_void_t result);

const char *gm_fake_git_repository_port_last_commit_message(
    const gm_fake_git_repository_port_t *fake);

const gm_oid_t *gm_fake_git_repository_port_last_commit_tree(
    const gm_fake_git_repository_port_t *fake);

const char *gm_fake_git_repository_port_last_update_ref(
    const gm_fake_git_repository_port_t *fake);

const char *gm_fake_git_repository_port_last_update_log(
    const gm_fake_git_repository_port_t *fake);

const gm_oid_t *gm_fake_git_repository_port_last_update_target(
    const gm_fake_git_repository_port_t *fake);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_TESTS_FAKES_GIT_FAKE_GIT_REPOSITORY_PORT_H */
