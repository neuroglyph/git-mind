/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_TESTS_FAKES_GIT_FAKE_GIT_REPOSITORY_PORT_H
#define GITMIND_TESTS_FAKES_GIT_FAKE_GIT_REPOSITORY_PORT_H

#include "gitmind/ports/git_repository_port.h"

#ifdef __cplusplus
extern "C" {
#endif

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
} gm_fake_git_repository_port_t;

GM_NODISCARD gm_result_void_t gm_fake_git_repository_port_init(
    gm_fake_git_repository_port_t *fake, const char *gitdir, const char *workdir);

void gm_fake_git_repository_port_dispose(gm_fake_git_repository_port_t *fake);

void gm_fake_git_repository_port_set_tip(gm_fake_git_repository_port_t *fake,
                                         const gm_git_reference_tip_t *tip);

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
