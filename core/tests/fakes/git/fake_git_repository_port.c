/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "core/tests/fakes/git/fake_git_repository_port.h"

#include <string.h>

#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/security/memory.h"
#include "gitmind/security/string.h"
#include "gitmind/types.h"

static gm_result_void_t fake_repository_path(void *self,
                                             gm_git_repository_path_kind_t kind,
                                             char *out_buffer,
                                             size_t buffer_size);
static gm_result_void_t fake_head_branch(void *self, char *out_name,
                                         size_t out_name_size);
static gm_result_void_t fake_build_tree(void *self, const char *dir_path,
                                        gm_oid_t *out_tree_oid);
static gm_result_void_t fake_reference_tip(void *self, const char *ref_name,
                                           gm_git_reference_tip_t *out_tip);
static gm_result_void_t fake_commit_create(void *self,
                                           const gm_git_commit_spec_t *spec,
                                           gm_oid_t *out_commit_oid);
static gm_result_void_t fake_reference_update(
    void *self, const gm_git_reference_update_spec_t *spec);
static gm_result_void_t fake_reference_glob_latest(
    void *self, const char *pattern, gm_git_reference_tip_t *out_tip);
static gm_result_void_t fake_commit_read_blob(void *self,
                                              const gm_oid_t *commit_oid,
                                              const char *path,
                                              uint8_t **out_data,
                                              size_t *out_size);
static gm_result_void_t fake_commit_read_message(void *self,
                                                 const gm_oid_t *commit_oid,
                                                 char **out_message);
static void fake_commit_message_dispose(void *self, char *message);
static gm_result_void_t fake_walk_commits(void *self, const char *ref_name,
                                          gm_git_commit_visit_cb cb,
                                          void *userdata);
static gm_result_void_t fake_commit_tree_size(void *self,
                                              const gm_oid_t *commit_oid,
                                              uint64_t *out_size_bytes);

static const gm_git_repository_port_vtbl_t FAKE_GIT_REPOSITORY_PORT_VTBL = {
    .repository_path = fake_repository_path,
    .head_branch = fake_head_branch,
    .build_tree_from_directory = fake_build_tree,
    .reference_tip = fake_reference_tip,
    .reference_glob_latest = fake_reference_glob_latest,
    .commit_read_blob = fake_commit_read_blob,
    .commit_read_message = fake_commit_read_message,
    .commit_message_dispose = fake_commit_message_dispose,
    .walk_commits = fake_walk_commits,
    .commit_tree_size = fake_commit_tree_size,
    .commit_create = fake_commit_create,
    .reference_update = fake_reference_update,
};

gm_result_void_t gm_fake_git_repository_port_init(
    gm_fake_git_repository_port_t *fake, const char *gitdir,
    const char *workdir) {
    if (fake == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "fake repository port requires storage"));
    }

    memset(fake, 0, sizeof(*fake));
    fake->port.vtbl = &FAKE_GIT_REPOSITORY_PORT_VTBL;
    fake->port.self = fake;
    fake->tree_result = gm_ok_void();
    fake->commit_result = gm_ok_void();
    fake->update_result = gm_ok_void();
    fake->tip.has_target = false;

    if (gitdir != NULL &&
        gm_strcpy_safe(fake->gitdir, sizeof(fake->gitdir), gitdir) != GM_OK) {
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                    "fake gitdir exceeds buffer"));
    }
    if (workdir != NULL &&
        gm_strcpy_safe(fake->workdir, sizeof(fake->workdir), workdir) != GM_OK) {
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG,
                                    "fake workdir exceeds buffer"));
    }

    fake->last_commit_message[0] = '\0';
    fake->last_update_ref[0] = '\0';
    fake->last_update_log[0] = '\0';
    gm_memset_safe(&fake->last_update_target, sizeof(fake->last_update_target), 0,
                   sizeof(fake->last_update_target));

    return gm_ok_void();
}

void gm_fake_git_repository_port_dispose(gm_fake_git_repository_port_t *fake) {
    if (fake == NULL) {
        return;
    }
    fake->port.vtbl = NULL;
    fake->port.self = NULL;
}

void gm_fake_git_repository_port_set_tip(gm_fake_git_repository_port_t *fake,
                                         const gm_git_reference_tip_t *tip) {
    if (fake == NULL || tip == NULL) {
        return;
    }
    fake->tip = *tip;
}

void gm_fake_git_repository_port_set_next_tree(
    gm_fake_git_repository_port_t *fake, const gm_oid_t *oid,
    gm_result_void_t result) {
    if (fake == NULL) {
        return;
    }
    fake->tree_result = result;
    if (oid != NULL) {
        fake->next_tree_oid = *oid;
    } else {
        gm_memset_safe(&fake->next_tree_oid, sizeof(fake->next_tree_oid), 0,
                       sizeof(fake->next_tree_oid));
    }
}

void gm_fake_git_repository_port_set_next_commit(
    gm_fake_git_repository_port_t *fake, const gm_oid_t *oid,
    gm_result_void_t result) {
    if (fake == NULL) {
        return;
    }
    fake->commit_result = result;
    if (oid != NULL) {
        fake->next_commit_oid = *oid;
    } else {
        gm_memset_safe(&fake->next_commit_oid, sizeof(fake->next_commit_oid), 0,
                       sizeof(fake->next_commit_oid));
    }
}

void gm_fake_git_repository_port_set_update_result(
    gm_fake_git_repository_port_t *fake, gm_result_void_t result) {
    if (fake == NULL) {
        return;
    }
    fake->update_result = result;
}

const char *gm_fake_git_repository_port_last_commit_message(
    const gm_fake_git_repository_port_t *fake) {
    return (fake == NULL) ? NULL : fake->last_commit_message;
}

const gm_oid_t *gm_fake_git_repository_port_last_commit_tree(
    const gm_fake_git_repository_port_t *fake) {
    return (fake == NULL) ? NULL : &fake->last_commit_tree_oid;
}

const char *gm_fake_git_repository_port_last_update_ref(
    const gm_fake_git_repository_port_t *fake) {
    return (fake == NULL) ? NULL : fake->last_update_ref;
}

const char *gm_fake_git_repository_port_last_update_log(
    const gm_fake_git_repository_port_t *fake) {
    return (fake == NULL) ? NULL : fake->last_update_log;
}

const gm_oid_t *gm_fake_git_repository_port_last_update_target(
    const gm_fake_git_repository_port_t *fake) {
    return (fake == NULL) ? NULL : &fake->last_update_target;
}

static gm_result_void_t fake_repository_path(void *self,
                                             gm_git_repository_path_kind_t kind,
                                             char *out_buffer,
                                             size_t buffer_size) {
    gm_fake_git_repository_port_t *fake =
        (gm_fake_git_repository_port_t *)self;
    if (fake == NULL || out_buffer == NULL || buffer_size == 0U) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "fake repo path requires buffers"));
    }

    const char *source = NULL;
    if (kind == GM_GIT_REPOSITORY_PATH_GITDIR) {
        source = fake->gitdir;
    } else if (kind == GM_GIT_REPOSITORY_PATH_WORKDIR) {
        source = fake->workdir;
    }

    if (source == NULL || source[0] == '\0') {
        return gm_err_void(GM_ERROR(GM_ERR_NOT_FOUND,
                                    "fake repo path unset for requested kind"));
    }

    if (gm_strcpy_safe(out_buffer, buffer_size, source) != GM_OK) {
        return gm_err_void(
            GM_ERROR(GM_ERR_PATH_TOO_LONG, "fake repo path exceeds buffer"));
    }

    return gm_ok_void();
}

static gm_result_void_t fake_head_branch(void *self, char *out_name,
                                         size_t out_name_size) {
    (void)self;
    (void)out_name;
    (void)out_name_size;
    return gm_err_void(GM_ERROR(GM_ERR_NOT_IMPLEMENTED,
                                "fake head branch not implemented"));
}

static gm_result_void_t fake_build_tree(void *self, const char *dir_path,
                                        gm_oid_t *out_tree_oid) {
    gm_fake_git_repository_port_t *fake =
        (gm_fake_git_repository_port_t *)self;
    (void)dir_path;
    if (!fake->tree_result.ok) {
        return fake->tree_result;
    }

    if (out_tree_oid != NULL) {
        if (fake->next_tree_oid.id[0] == 0) {
            fake->counter += 1U;
            memset(out_tree_oid, 0, sizeof(*out_tree_oid));
            out_tree_oid->id[0] = (unsigned char)(fake->counter & 0xFFU);
            fake->next_tree_oid = *out_tree_oid;
        } else {
            *out_tree_oid = fake->next_tree_oid;
        }
    }

    return gm_ok_void();
}

static gm_result_void_t fake_reference_tip(void *self, const char *ref_name,
                                           gm_git_reference_tip_t *out_tip) {
    (void)ref_name;
    gm_fake_git_repository_port_t *fake =
        (gm_fake_git_repository_port_t *)self;
    if (out_tip != NULL) {
        *out_tip = fake->tip;
    }
    return gm_ok_void();
}

static gm_result_void_t fake_reference_glob_latest(
    void *self, const char *pattern, gm_git_reference_tip_t *out_tip) {
    gm_fake_git_repository_port_t *fake =
        (gm_fake_git_repository_port_t *)self;
    if (pattern == NULL || out_tip == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "fake glob requires pattern and output"));
    }
    if (fake->tip.has_target) {
        *out_tip = fake->tip;
    } else {
        gm_memset_safe(out_tip, sizeof(*out_tip), 0, sizeof(*out_tip));
    }
    (void)pattern;
    return gm_ok_void();
}

static gm_result_void_t fake_commit_read_blob(void *self,
                                              const gm_oid_t *commit_oid,
                                              const char *path,
                                              uint8_t **out_data,
                                              size_t *out_size) {
    (void)self;
    (void)commit_oid;
    (void)path;
    if (out_data == NULL || out_size == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "fake blob read requires outputs"));
    }
    *out_data = NULL;
    *out_size = 0;
    return gm_ok_void();
}

static gm_result_void_t fake_commit_read_message(void *self,
                                                 const gm_oid_t *commit_oid,
                                                 char **out_message) {
    (void)self;
    (void)commit_oid;
    if (out_message == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "fake commit message requires output"));
    }
    *out_message = NULL;
    return gm_err_void(GM_ERROR(GM_ERR_NOT_IMPLEMENTED,
                                "fake commit read message not implemented"));
}

static void fake_commit_message_dispose(void *self, char *message) {
    (void)self;
    free(message);
}

static gm_result_void_t fake_walk_commits(void *self, const char *ref_name,
                                          gm_git_commit_visit_cb cb,
                                          void *userdata) {
    (void)self;
    (void)ref_name;
    (void)cb;
    (void)userdata;
    return gm_err_void(GM_ERROR(GM_ERR_NOT_IMPLEMENTED,
                                "fake walk commits not implemented"));
}

static gm_result_void_t fake_commit_tree_size(void *self,
                                              const gm_oid_t *commit_oid,
                                              uint64_t *out_size_bytes) {
    (void)self;
    (void)commit_oid;
    if (out_size_bytes == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "fake tree size requires output"));
    }
    *out_size_bytes = 0;
    return gm_ok_void();
}

static gm_result_void_t fake_commit_create(void *self,
                                           const gm_git_commit_spec_t *spec,
                                           gm_oid_t *out_commit_oid) {
    gm_fake_git_repository_port_t *fake =
        (gm_fake_git_repository_port_t *)self;
    if (!fake->commit_result.ok) {
        return fake->commit_result;
    }

    if (spec != NULL) {
        fake->last_commit_tree_oid = (spec->tree_oid != NULL)
                                         ? *spec->tree_oid
                                         : (gm_oid_t){0};
        if (spec->message != NULL) {
            (void)gm_strcpy_safe(fake->last_commit_message,
                                 sizeof(fake->last_commit_message), spec->message);
        } else {
            fake->last_commit_message[0] = '\0';
        }
    }

    if (out_commit_oid != NULL) {
        if (fake->next_commit_oid.id[0] == 0) {
            fake->counter += 1U;
            memset(out_commit_oid, 0, sizeof(*out_commit_oid));
            out_commit_oid->id[0] = (unsigned char)((fake->counter >> 1U) & 0xFFU);
            fake->next_commit_oid = *out_commit_oid;
        } else {
            *out_commit_oid = fake->next_commit_oid;
        }
    }

    return gm_ok_void();
}

static gm_result_void_t fake_reference_update(
    void *self, const gm_git_reference_update_spec_t *spec) {
    gm_fake_git_repository_port_t *fake =
        (gm_fake_git_repository_port_t *)self;
    if (!fake->update_result.ok) {
        return fake->update_result;
    }

    if (spec != NULL) {
        if (spec->ref_name != NULL) {
            (void)gm_strcpy_safe(fake->last_update_ref, sizeof(fake->last_update_ref),
                                 spec->ref_name);
        } else {
            fake->last_update_ref[0] = '\0';
        }

        if (spec->log_message != NULL) {
            (void)gm_strcpy_safe(fake->last_update_log,
                                 sizeof(fake->last_update_log), spec->log_message);
        } else {
            fake->last_update_log[0] = '\0';
        }

        if (spec->target_oid != NULL) {
            fake->last_update_target = *spec->target_oid;
        } else {
            gm_memset_safe(&fake->last_update_target,
                           sizeof(fake->last_update_target), 0,
                           sizeof(fake->last_update_target));
        }
    }

    return gm_ok_void();
}
