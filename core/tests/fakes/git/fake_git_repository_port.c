/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "core/tests/fakes/git/fake_git_repository_port.h"

#include <stdlib.h>
#include <string.h>

#include "gitmind/constants.h"
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
                                          gm_git_commit_visit_cb visit_callback,
                                          void *userdata);
static gm_result_void_t fake_commit_tree_size(void *self,
                                              const gm_oid_t *commit_oid,
                                              uint64_t *out_size_bytes);
static gm_result_void_t fake_resolve_blob_at_head(void *self, const char *path,
                                                  gm_oid_t *out_blob_oid);
static gm_result_void_t fake_resolve_blob_at_commit(void *self,
                                                    const gm_oid_t *commit_oid,
                                                    const char *path,
                                                    gm_oid_t *out_blob_oid);
static gm_result_void_t fake_commit_parent_count(void *self,
                                                 const gm_oid_t *commit_oid,
                                                 size_t *out_parent_count);

static gm_result_void_t ensure_ref_entry(gm_fake_git_repository_port_t *fake,
                                         const char *ref_name,
                                         gm_fake_git_ref_entry_t **out_entry);

static const gm_fake_git_ref_entry_t *find_ref_entry_const(
    const gm_fake_git_repository_port_t *fake, const char *ref_name);

static const gm_fake_git_commit_entry_t *find_commit_entry_const(
    const gm_fake_git_repository_port_t *fake, const gm_oid_t *commit_oid);

static gm_fake_git_commit_entry_t *find_commit_entry(
    gm_fake_git_repository_port_t *fake, const gm_oid_t *commit_oid);

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
    .resolve_blob_at_head = fake_resolve_blob_at_head,
    .resolve_blob_at_commit = fake_resolve_blob_at_commit,
    .commit_parent_count = fake_commit_parent_count,
};

static gm_result_void_t ensure_ref_entry(gm_fake_git_repository_port_t *fake,
                                         const char *ref_name,
                                         gm_fake_git_ref_entry_t **out_entry) {
    if (fake == NULL || ref_name == NULL || out_entry == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "fake ref entry requires inputs"));
    }

    gm_fake_git_ref_entry_t *free_slot = NULL;
    for (size_t idx = 0; idx < GM_FAKE_GIT_MAX_REF_ENTRIES; ++idx) {
        gm_fake_git_ref_entry_t *entry = &fake->ref_entries[idx];
        if (entry->in_use) {
            if (strcmp(entry->ref_name, ref_name) == 0) {
                *out_entry = entry;
                return gm_ok_void();
            }
        } else if (free_slot == NULL) {
            free_slot = entry;
        }
    }

    if (free_slot == NULL) {
        return gm_err_void(
            GM_ERROR(GM_ERR_BUFFER_TOO_SMALL, "fake ref storage exhausted"));
    }

    gm_memset_safe(free_slot, sizeof(*free_slot), 0, sizeof(*free_slot));
    if (gm_strcpy_safe(free_slot->ref_name, sizeof(free_slot->ref_name),
                       ref_name) != GM_OK) {
        gm_memset_safe(free_slot, sizeof(*free_slot), 0, sizeof(*free_slot));
        return gm_err_void(
            GM_ERROR(GM_ERR_BUFFER_TOO_SMALL, "fake ref name exceeds buffer"));
    }

    free_slot->in_use = true;
    free_slot->commit_count = 0U;
    *out_entry = free_slot;
    return gm_ok_void();
}

static const gm_fake_git_ref_entry_t *find_ref_entry_const(
    const gm_fake_git_repository_port_t *fake, const char *ref_name) {
    if (fake == NULL || ref_name == NULL) {
        return NULL;
    }

    for (size_t idx = 0; idx < GM_FAKE_GIT_MAX_REF_ENTRIES; ++idx) {
        const gm_fake_git_ref_entry_t *entry = &fake->ref_entries[idx];
        if (!entry->in_use) {
            continue;
        }
        if (strcmp(entry->ref_name, ref_name) == 0) {
            return entry;
        }
    }

    return NULL;
}

static const gm_fake_git_commit_entry_t *find_commit_entry_const(
    const gm_fake_git_repository_port_t *fake, const gm_oid_t *commit_oid) {
    if (fake == NULL || commit_oid == NULL) {
        return NULL;
    }

    for (size_t idx = 0; idx < GM_FAKE_GIT_MAX_REF_ENTRIES; ++idx) {
        const gm_fake_git_ref_entry_t *entry = &fake->ref_entries[idx];
        if (!entry->in_use) {
            continue;
        }
        for (size_t commit_idx = 0; commit_idx < entry->commit_count;
             ++commit_idx) {
            const gm_fake_git_commit_entry_t *commit =
                &entry->commits[commit_idx];
            if (memcmp(commit->oid.id, commit_oid->id, GM_OID_RAWSZ) == 0) {
                return commit;
            }
        }
    }

    return NULL;
}

static gm_fake_git_commit_entry_t *find_commit_entry(
    gm_fake_git_repository_port_t *fake, const gm_oid_t *commit_oid) {
    if (fake == NULL || commit_oid == NULL) {
        return NULL;
    }

    for (size_t idx = 0; idx < GM_FAKE_GIT_MAX_REF_ENTRIES; ++idx) {
        gm_fake_git_ref_entry_t *entry = &fake->ref_entries[idx];
        if (!entry->in_use) {
            continue;
        }
        for (size_t commit_idx = 0; commit_idx < entry->commit_count;
             ++commit_idx) {
            gm_fake_git_commit_entry_t *commit = &entry->commits[commit_idx];
            if (memcmp(commit->oid.id, commit_oid->id, GM_OID_RAWSZ) == 0) {
                return commit;
            }
        }
    }

    return NULL;
}

gm_result_void_t gm_fake_git_repository_port_init(
    gm_fake_git_repository_port_t *fake, const char *gitdir,
    const char *workdir) {
    if (fake == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "fake repository port requires storage"));
    }

    gm_memset_safe(fake, sizeof(*fake), 0, sizeof(*fake));
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

gm_result_void_t gm_fake_git_repository_port_set_head_branch(
    gm_fake_git_repository_port_t *fake, const char *branch_name) {
    if (fake == NULL || branch_name == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "fake head branch requires inputs"));
    }

    if (gm_strcpy_safe(fake->head_branch, sizeof(fake->head_branch),
                       branch_name) != GM_OK) {
        return gm_err_void(
            GM_ERROR(GM_ERR_BUFFER_TOO_SMALL, "fake head branch too long"));
    }

    return gm_ok_void();
}

void gm_fake_git_repository_port_clear_ref_commits(
    gm_fake_git_repository_port_t *fake) {
    if (fake == NULL) {
        return;
    }
    gm_memset_safe(fake->ref_entries, sizeof(fake->ref_entries), 0,
                   sizeof(fake->ref_entries));
}

gm_result_void_t gm_fake_git_repository_port_add_ref_commit(
    gm_fake_git_repository_port_t *fake, const char *ref_name,
    const gm_oid_t *commit_oid, const char *message) {
    if (fake == NULL || ref_name == NULL || commit_oid == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "fake add ref commit requires inputs"));
    }

    gm_fake_git_ref_entry_t *entry = NULL;
    GM_TRY(ensure_ref_entry(fake, ref_name, &entry));

    if (entry->commit_count >= GM_FAKE_GIT_MAX_COMMITS_PER_REF) {
        return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL,
                                    "fake commit list full"));
    }

    gm_fake_git_commit_entry_t *commit =
        &entry->commits[entry->commit_count];
    gm_memset_safe(commit, sizeof(*commit), 0, sizeof(*commit));
    commit->oid = *commit_oid;
    commit->parent_count = 0U;

    if (message != NULL) {
        if (gm_strcpy_safe(commit->message, sizeof(commit->message), message) !=
            GM_OK) {
            gm_memset_safe(commit, sizeof(*commit), 0, sizeof(*commit));
            return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL,
                                        "fake commit message too long"));
        }
        commit->has_message = true;
    }

    entry->commit_count += 1U;
    return gm_ok_void();
}

void gm_fake_git_repository_port_clear_blob_mappings(
    gm_fake_git_repository_port_t *fake) {
    if (fake == NULL) {
        return;
    }
    gm_memset_safe(fake->blob_entries, sizeof(fake->blob_entries), 0,
                   sizeof(fake->blob_entries));
}

gm_result_void_t gm_fake_git_repository_port_add_blob_mapping(
    gm_fake_git_repository_port_t *fake, const char *path,
    const gm_oid_t *blob_oid) {
    if (fake == NULL || path == NULL || blob_oid == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "fake blob mapping requires inputs"));
    }

    gm_fake_git_blob_entry_t *slot = NULL;
    for (size_t idx = 0; idx < GM_FAKE_GIT_MAX_BLOB_PATHS; ++idx) {
        gm_fake_git_blob_entry_t *entry = &fake->blob_entries[idx];
        if (entry->in_use) {
            if (strcmp(entry->path, path) == 0) {
                slot = entry;
                break;
            }
        } else if (slot == NULL) {
            slot = entry;
        }
    }

    if (slot == NULL) {
        return gm_err_void(
            GM_ERROR(GM_ERR_BUFFER_TOO_SMALL, "fake blob mapping full"));
    }

    gm_memset_safe(slot, sizeof(*slot), 0, sizeof(*slot));
    if (gm_strcpy_safe(slot->path, sizeof(slot->path), path) != GM_OK) {
        gm_memset_safe(slot, sizeof(*slot), 0, sizeof(*slot));
        return gm_err_void(
            GM_ERROR(GM_ERR_BUFFER_TOO_SMALL, "fake blob path too long"));
    }

    slot->oid = *blob_oid;
    slot->has_commit = false;
    gm_memset_safe(&slot->commit_oid, sizeof(slot->commit_oid), 0,
                   sizeof(slot->commit_oid));
    slot->in_use = true;
    return gm_ok_void();
}

gm_result_void_t gm_fake_git_repository_port_add_commit_blob_mapping(
    gm_fake_git_repository_port_t *fake, const gm_oid_t *commit_oid,
    const char *path, const gm_oid_t *blob_oid) {
    if (fake == NULL || commit_oid == NULL || path == NULL || blob_oid == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "fake commit blob mapping requires inputs"));
    }

    gm_fake_git_blob_entry_t *slot = NULL;
    for (size_t idx = 0; idx < GM_FAKE_GIT_MAX_BLOB_PATHS; ++idx) {
        gm_fake_git_blob_entry_t *entry = &fake->blob_entries[idx];
        if (entry->in_use) {
            if (entry->has_commit &&
                memcmp(entry->commit_oid.id, commit_oid->id, GM_OID_RAWSZ) == 0 &&
                strcmp(entry->path, path) == 0) {
                slot = entry;
                break;
            }
        } else if (slot == NULL) {
            slot = entry;
        }
    }

    if (slot == NULL) {
        return gm_err_void(
            GM_ERROR(GM_ERR_BUFFER_TOO_SMALL, "fake blob mapping full"));
    }

    gm_memset_safe(slot, sizeof(*slot), 0, sizeof(*slot));
    if (gm_strcpy_safe(slot->path, sizeof(slot->path), path) != GM_OK) {
        gm_memset_safe(slot, sizeof(*slot), 0, sizeof(*slot));
        return gm_err_void(
            GM_ERROR(GM_ERR_BUFFER_TOO_SMALL, "fake blob path too long"));
    }

    slot->oid = *blob_oid;
    slot->commit_oid = *commit_oid;
    slot->has_commit = true;
    slot->in_use = true;
    return gm_ok_void();
}

gm_result_void_t gm_fake_git_repository_port_set_commit_parents(
    gm_fake_git_repository_port_t *fake, const gm_oid_t *commit_oid,
    const gm_oid_t *parents, size_t parent_count) {
    if (fake == NULL || commit_oid == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "fake commit parents require inputs"));
    }

    if (parent_count > GM_FAKE_GIT_MAX_PARENTS) {
        return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL,
                                    "fake commit parent list too large"));
    }

    gm_fake_git_commit_entry_t *commit = find_commit_entry(fake, commit_oid);
    if (commit == NULL) {
        return gm_err_void(
            GM_ERROR(GM_ERR_NOT_FOUND, "fake commit missing for parent setup"));
    }

    commit->parent_count = parent_count;
    if (parent_count > 0U && parents != NULL) {
        for (size_t idx = 0; idx < parent_count; ++idx) {
            commit->parents[idx] = parents[idx];
        }
    }

    for (size_t idx = parent_count; idx < GM_FAKE_GIT_MAX_PARENTS; ++idx) {
        gm_memset_safe(&commit->parents[idx], sizeof(commit->parents[idx]), 0,
                       sizeof(commit->parents[idx]));
    }

    return gm_ok_void();
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
    gm_fake_git_repository_port_t *fake =
        (gm_fake_git_repository_port_t *)self;
    if (fake == NULL || out_name == NULL || out_name_size == 0U) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "fake head branch requires buffers"));
    }

    if (fake->head_branch[0] == '\0') {
        return gm_err_void(
            GM_ERROR(GM_ERR_NOT_FOUND, "fake head branch unset"));
    }

    if (gm_strcpy_safe(out_name, out_name_size, fake->head_branch) != GM_OK) {
        return gm_err_void(
            GM_ERROR(GM_ERR_BUFFER_TOO_SMALL, "fake head branch too long"));
    }

    return gm_ok_void();
}

static gm_result_void_t fake_build_tree(void *self, const char *dir_path,
                                        gm_oid_t *out_tree_oid) {
    gm_fake_git_repository_port_t *fake =
        (gm_fake_git_repository_port_t *)self;
    (void)dir_path;
    if (!fake->tree_result.ok) {
        return fake->tree_result;
    }

    if (out_tree_oid == NULL) {
        return gm_ok_void();
    }

    if (fake->next_tree_oid.id[0] != 0U) {
        *out_tree_oid = fake->next_tree_oid;
        gm_memset_safe(&fake->next_tree_oid, sizeof(fake->next_tree_oid), 0,
                       sizeof(fake->next_tree_oid));
        return gm_ok_void();
    }

    fake->counter += 1U;
    gm_memset_safe(out_tree_oid, sizeof(*out_tree_oid), 0, sizeof(*out_tree_oid));
    out_tree_oid->id[0] = (unsigned char)(fake->counter & 0xFFU);

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
    gm_fake_git_repository_port_t *fake =
        (gm_fake_git_repository_port_t *)self;
    if (fake == NULL || commit_oid == NULL || out_message == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "fake commit message requires inputs"));
    }

    const gm_fake_git_commit_entry_t *commit =
        find_commit_entry_const(fake, commit_oid);
    if (commit == NULL || !commit->has_message) {
        *out_message = NULL;
        return gm_err_void(
            GM_ERROR(GM_ERR_NOT_FOUND, "fake commit message unavailable"));
    }

    const size_t message_len = strlen(commit->message);
    char *copy = (char *)malloc(message_len + 1U);
    if (copy == NULL) {
        *out_message = NULL;
        return gm_err_void(
            GM_ERROR(GM_ERR_OUT_OF_MEMORY, "allocating fake commit message"));
    }

    gm_strcpy_safe(copy, message_len + 1U, commit->message);
    *out_message = copy;
    return gm_ok_void();
}

static void fake_commit_message_dispose(void *self, char *message) {
    (void)self;
    free(message);
}

static gm_result_void_t fake_walk_commits(void *self, const char *ref_name,
                                          gm_git_commit_visit_cb visit_callback,
                                          void *userdata) {
    gm_fake_git_repository_port_t *fake =
        (gm_fake_git_repository_port_t *)self;
    if (fake == NULL || ref_name == NULL || visit_callback == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "fake commit walk requires inputs"));
    }

    const gm_fake_git_ref_entry_t *entry =
        find_ref_entry_const(fake, ref_name);
    if (entry == NULL || entry->commit_count == 0U) {
        return gm_err_void(
            GM_ERROR(GM_ERR_NOT_FOUND, "fake commit walk has no commits"));
    }

    for (size_t idx = 0; idx < entry->commit_count; ++idx) {
        const gm_fake_git_commit_entry_t *commit = &entry->commits[idx];
        int cb_result = visit_callback(&commit->oid, userdata);
        if (cb_result == GM_CALLBACK_STOP) {
            break;
        }
        if (cb_result != GM_OK) {
            return gm_err_void(
                GM_ERROR(cb_result, "fake commit walk callback stop"));
        }
    }

    return gm_ok_void();
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
            int msg_rc = gm_strcpy_safe(fake->last_commit_message,
                                        sizeof(fake->last_commit_message),
                                        spec->message);
            if (msg_rc != GM_OK) {
                fake->last_commit_message[0] = '\0';
                return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL,
                                            "fake commit message too long"));
            }
        } else {
            fake->last_commit_message[0] = '\0';
        }
    }

    if (out_commit_oid == NULL) {
        return gm_ok_void();
    }

    if (fake->next_commit_oid.id[0] != 0U) {
        *out_commit_oid = fake->next_commit_oid;
        gm_memset_safe(&fake->next_commit_oid, sizeof(fake->next_commit_oid), 0,
                       sizeof(fake->next_commit_oid));
        return gm_ok_void();
    }

    fake->counter += 1U;
    gm_memset_safe(out_commit_oid, sizeof(*out_commit_oid), 0,
                   sizeof(*out_commit_oid));
    out_commit_oid->id[0] =
        (unsigned char)((fake->counter >> 1U) & 0xFFU);

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
            int ref_rc = gm_strcpy_safe(fake->last_update_ref,
                                        sizeof(fake->last_update_ref),
                                        spec->ref_name);
            if (ref_rc != GM_OK) {
                fake->last_update_ref[0] = '\0';
                return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL,
                                            "fake update ref too long"));
            }
        } else {
            fake->last_update_ref[0] = '\0';
        }

        if (spec->log_message != NULL) {
            int log_rc = gm_strcpy_safe(fake->last_update_log,
                                        sizeof(fake->last_update_log),
                                        spec->log_message);
            if (log_rc != GM_OK) {
                fake->last_update_log[0] = '\0';
                return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL,
                                            "fake update log too long"));
            }
        } else {
            fake->last_update_log[0] = '\0';
        }

        if (spec->target_oid != NULL) {
            fake->last_update_target = *spec->target_oid;
            /* Record commit in the ref so walks can see it */
            gm_fake_git_ref_entry_t *entry = NULL;
            if (ensure_ref_entry(fake, spec->ref_name, &entry).ok) {
                if (entry->commit_count < GM_FAKE_GIT_MAX_COMMITS_PER_REF) {
                    gm_fake_git_commit_entry_t *commit =
                        &entry->commits[entry->commit_count++];
                    gm_memset_safe(commit, sizeof(*commit), 0, sizeof(*commit));
                    commit->oid = *spec->target_oid;
                    commit->has_message = (fake->last_commit_message[0] != '\0');
                    if (commit->has_message) {
                        (void)gm_strcpy_safe(commit->message,
                                             sizeof(commit->message),
                                             fake->last_commit_message);
                    }
                    commit->parent_count = 0U;
                }
            }
        } else {
            gm_memset_safe(&fake->last_update_target,
                           sizeof(fake->last_update_target), 0,
                           sizeof(fake->last_update_target));
        }
    }

    return gm_ok_void();
}

static gm_result_void_t fake_resolve_blob_at_head(void *self, const char *path,
                                                  gm_oid_t *out_blob_oid) {
    gm_fake_git_repository_port_t *fake =
        (gm_fake_git_repository_port_t *)self;
    if (fake == NULL || path == NULL || out_blob_oid == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "fake blob resolve requires inputs"));
    }

    if (fake->tip.has_target) {
        for (size_t idx = 0; idx < GM_FAKE_GIT_MAX_BLOB_PATHS; ++idx) {
            gm_fake_git_blob_entry_t *entry = &fake->blob_entries[idx];
            if (!entry->in_use || !entry->has_commit) {
                continue;
            }
            if (memcmp(entry->commit_oid.id, fake->tip.oid.id, GM_OID_RAWSZ) == 0 &&
                strcmp(entry->path, path) == 0) {
                *out_blob_oid = entry->oid;
                return gm_ok_void();
            }
        }
    }

    for (size_t idx = 0; idx < GM_FAKE_GIT_MAX_BLOB_PATHS; ++idx) {
        gm_fake_git_blob_entry_t *entry = &fake->blob_entries[idx];
        if (!entry->in_use || entry->has_commit) {
            continue;
        }
        if (strcmp(entry->path, path) == 0) {
            *out_blob_oid = entry->oid;
            return gm_ok_void();
        }
    }

    return gm_err_void(
        GM_ERROR(GM_ERR_NOT_FOUND, "fake blob mapping missing for %s", path));
}

static gm_result_void_t fake_resolve_blob_at_commit(void *self,
                                                    const gm_oid_t *commit_oid,
                                                    const char *path,
                                                    gm_oid_t *out_blob_oid) {
    gm_fake_git_repository_port_t *fake =
        (gm_fake_git_repository_port_t *)self;
    if (fake == NULL || commit_oid == NULL || path == NULL ||
        out_blob_oid == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "fake commit blob resolve requires inputs"));
    }

    for (size_t idx = 0; idx < GM_FAKE_GIT_MAX_BLOB_PATHS; ++idx) {
        gm_fake_git_blob_entry_t *entry = &fake->blob_entries[idx];
        if (!entry->in_use) {
            continue;
        }
        if (entry->has_commit &&
            memcmp(entry->commit_oid.id, commit_oid->id, GM_OID_RAWSZ) == 0 &&
            strcmp(entry->path, path) == 0) {
            *out_blob_oid = entry->oid;
            return gm_ok_void();
        }
    }

    for (size_t idx = 0; idx < GM_FAKE_GIT_MAX_BLOB_PATHS; ++idx) {
        gm_fake_git_blob_entry_t *entry = &fake->blob_entries[idx];
        if (!entry->in_use || entry->has_commit) {
            continue;
        }
        if (strcmp(entry->path, path) == 0) {
            *out_blob_oid = entry->oid;
            return gm_ok_void();
        }
    }

    return gm_err_void(
        GM_ERROR(GM_ERR_NOT_FOUND, "fake blob mapping missing for %s", path));
}

static gm_result_void_t fake_commit_parent_count(void *self,
                                                 const gm_oid_t *commit_oid,
                                                 size_t *out_parent_count) {
    gm_fake_git_repository_port_t *fake =
        (gm_fake_git_repository_port_t *)self;
    if (fake == NULL || commit_oid == NULL || out_parent_count == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "fake parent count requires inputs"));
    }

    gm_fake_git_commit_entry_t *commit =
        find_commit_entry(fake, commit_oid);
    if (commit == NULL) {
        return gm_err_void(
            GM_ERROR(GM_ERR_NOT_FOUND, "fake commit missing for parent count"));
    }

    *out_parent_count = commit->parent_count;
    return gm_ok_void();
}
