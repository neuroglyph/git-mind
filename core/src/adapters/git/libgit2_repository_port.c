/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#define _POSIX_C_SOURCE 200809L
#if !defined(__APPLE__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include "gitmind/adapters/git/libgit2_repository_port.h"

#include <dirent.h>
#include <errno.h>
#include <git2/blob.h>
#include <git2/commit.h>
#include <git2/odb.h>
#include <git2/oid.h>
#include <git2/refs.h>
#include <git2/repository.h>
#include <git2/revwalk.h>
#include <git2/signature.h>
#include <git2/tree.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>

#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/security/memory.h"
#include "gitmind/security/string.h"
#include "gitmind/types.h"

struct gm_libgit2_repository_port_state {
    git_repository *repo;
};

static gm_result_void_t repository_path_impl(gm_libgit2_repository_port_state_t *state,
                                             gm_git_repository_path_kind_t kind,
                                             char *out_buffer, size_t buffer_size) {
    if (out_buffer == NULL || buffer_size == 0U) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "repository path output buffer missing"));
    }

    const char *source = NULL;
    switch (kind) {
    case GM_GIT_REPOSITORY_PATH_GITDIR:
        source = git_repository_path(state->repo);
        break;
    case GM_GIT_REPOSITORY_PATH_WORKDIR:
        source = git_repository_workdir(state->repo);
        break;
    default:
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, "unknown repository path kind"));
    }

    if (source == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_NOT_FOUND,
                                    "requested repo path kind unavailable"));
    }

    if (gm_strcpy_safe(out_buffer, buffer_size, source) != GM_OK) {
        return gm_err_void(
            GM_ERROR(GM_ERR_PATH_TOO_LONG, "repository path exceeds buffer"));
    }

    return gm_ok_void();
}

static gm_result_void_t head_branch_impl(gm_libgit2_repository_port_state_t *state,
                                         char *out_name, size_t out_name_size) {
    if (out_name == NULL || out_name_size == 0U) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "head branch requires buffer"));
    }

    git_reference *head = NULL;
    if (git_repository_head(&head, state->repo) != 0) {
        return gm_err_void(
            GM_ERROR(GM_ERR_NOT_FOUND, "failed to resolve repository head"));
    }

    const char *name = git_reference_shorthand(head);
    if (name == NULL) {
        git_reference_free(head);
        return gm_err_void(
            GM_ERROR(GM_ERR_NOT_FOUND, "head reference lacks shorthand"));
    }

    int copy_status = gm_strcpy_safe(out_name, out_name_size, name);
    git_reference_free(head);
    if (copy_status != GM_OK) {
        return gm_err_void(
            GM_ERROR(GM_ERR_PATH_TOO_LONG, "head branch name exceeds buffer"));
    }

    return gm_ok_void();
}

static gm_result_void_t write_blob_from_file(git_repository *repo,
                                             const char *file_path,
                                             git_oid *out_oid) {
    if (file_path == NULL || out_oid == NULL) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, "blob write requires path/oid"));
    }

    if (git_blob_create_from_disk(out_oid, repo, file_path) < 0) {
        return gm_err_void(GM_ERROR(GM_ERR_IO_FAILED,
                                    "failed to create blob for %s", file_path));
    }

    return gm_ok_void();
}

static bool should_skip_entry(const char *name) {
    return name == NULL || strcmp(name, ".") == 0 || strcmp(name, "..") == 0;
}

static gm_result_void_t add_entry_to_builder(git_repository *repo,
                                             git_treebuilder *builder,
                                             const char *dir_path,
                                             const char *entry_name);

static gm_result_void_t add_directory_tree(git_repository *repo,
                                           git_treebuilder *parent_builder,
                                           const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_IO_FAILED,
                                    "opendir failed for %s: %s", dir_path,
                                    strerror(errno)));
    }

    git_treebuilder *local_builder = NULL;
    if (git_treebuilder_new(&local_builder, repo, NULL) < 0) {
        closedir(dir);
        return gm_err_void(
            GM_ERROR(GM_ERR_UNKNOWN, "treebuilder allocation failed"));
    }

    gm_result_void_t result = gm_ok_void();
    struct dirent *entry = NULL;
    while ((entry = readdir(dir)) != NULL && result.ok) {
        result = add_entry_to_builder(repo, local_builder, dir_path, entry->d_name);
    }

    closedir(dir);

    if (!result.ok) {
        git_treebuilder_free(local_builder);
        return result;
    }

    git_oid tree_oid;
    if (git_treebuilder_write(&tree_oid, local_builder) < 0) {
        git_treebuilder_free(local_builder);
        return gm_err_void(
            GM_ERROR(GM_ERR_UNKNOWN, "unable to write tree for %s", dir_path));
    }

    if (parent_builder != NULL) {
        const char *basename = strrchr(dir_path, '/');
        basename = (basename == NULL) ? dir_path : basename + 1;
        if (git_treebuilder_insert(NULL, parent_builder, basename, &tree_oid,
                                   GIT_FILEMODE_TREE) < 0) {
            git_treebuilder_free(local_builder);
            return gm_err_void(
                GM_ERROR(GM_ERR_UNKNOWN, "unable to insert tree %s", basename));
        }
    }

    git_treebuilder_free(local_builder);
    return gm_ok_void();
}

static gm_result_void_t add_entry_to_builder(git_repository *repo,
                                             git_treebuilder *builder,
                                             const char *dir_path,
                                             const char *entry_name) {
    if (should_skip_entry(entry_name)) {
        return gm_ok_void();
    }

    char full_path[GM_PATH_MAX];
    if (gm_snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry_name) <
        0) {
        return gm_err_void(
            GM_ERROR(GM_ERR_PATH_TOO_LONG, "entry path exceeds buffer"));
    }

    struct stat st = {0};
    if (lstat(full_path, &st) != 0) {
        if (errno == ENOENT) {
            return gm_ok_void();
        }
        return gm_err_void(
            GM_ERROR(GM_ERR_IO_FAILED, "lstat failed for %s: %s", full_path,
                     strerror(errno)));
    }

    if (S_ISDIR(st.st_mode)) {
        return add_directory_tree(repo, builder, full_path);
    }

    if (!S_ISREG(st.st_mode)) {
        return gm_ok_void();
    }

    git_oid blob_oid;
    GM_TRY(write_blob_from_file(repo, full_path, &blob_oid));

    if (git_treebuilder_insert(NULL, builder, entry_name, &blob_oid,
                               GIT_FILEMODE_BLOB) < 0) {
        return gm_err_void(
            GM_ERROR(GM_ERR_UNKNOWN, "unable to insert blob for %s", entry_name));
    }

    return gm_ok_void();
}

static gm_result_void_t build_tree_from_directory_impl(
    gm_libgit2_repository_port_state_t *state, const char *dir_path,
    gm_oid_t *out_tree_oid) {
    if (dir_path == NULL || out_tree_oid == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "tree build requires directory and output"));
    }

    git_treebuilder *root_builder = NULL;
    if (git_treebuilder_new(&root_builder, state->repo, NULL) < 0) {
        return gm_err_void(
            GM_ERROR(GM_ERR_UNKNOWN, "unable to create root treebuilder"));
    }

    gm_result_void_t result = add_directory_tree(state->repo, root_builder, dir_path);
    if (!result.ok) {
        git_treebuilder_free(root_builder);
        return result;
    }

    if (git_treebuilder_write(out_tree_oid, root_builder) < 0) {
        git_treebuilder_free(root_builder);
        return gm_err_void(
            GM_ERROR(GM_ERR_UNKNOWN, "unable to write root tree"));
    }

    git_treebuilder_free(root_builder);
    return gm_ok_void();
}

static gm_result_void_t reference_tip_impl(gm_libgit2_repository_port_state_t *state,
                                           const char *ref_name,
                                           gm_git_reference_tip_t *out_tip) {
    if (ref_name == NULL || out_tip == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "reference tip requires inputs"));
    }

    git_reference *ref = NULL;
    if (git_reference_lookup(&ref, state->repo, ref_name) != 0) {
        out_tip->has_target = false;
        gm_memset_safe(&out_tip->oid, sizeof(out_tip->oid), 0,
                       sizeof(out_tip->oid));
        out_tip->commit_time = 0;
        out_tip->oid_hex[0] = '\0';
        return gm_ok_void();
    }

    const git_oid *target = git_reference_target(ref);
    if (target == NULL) {
        git_reference_free(ref);
        out_tip->has_target = false;
        gm_memset_safe(&out_tip->oid, sizeof(out_tip->oid), 0,
                       sizeof(out_tip->oid));
        out_tip->commit_time = 0;
        out_tip->oid_hex[0] = '\0';
        return gm_ok_void();
    }

    out_tip->has_target = true;
    out_tip->oid = *target;
    git_oid_tostr(out_tip->oid_hex, sizeof(out_tip->oid_hex), target);

    git_commit *commit = NULL;
    if (git_commit_lookup(&commit, state->repo, target) == 0) {
        out_tip->commit_time = (uint64_t)git_commit_time(commit);
        git_commit_free(commit);
    } else {
        out_tip->commit_time = 0;
    }

    git_reference_free(ref);
    return gm_ok_void();
}

static gm_result_void_t reference_glob_latest_impl(
    gm_libgit2_repository_port_state_t *state, const char *pattern,
    gm_git_reference_tip_t *out_tip) {
    if (pattern == NULL || out_tip == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "reference glob requires inputs"));
    }

    git_reference_iterator *iter = NULL;
    if (git_reference_iterator_glob_new(&iter, state->repo, pattern) != 0) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, "invalid reference glob pattern"));
    }

    gm_memset_safe(out_tip, sizeof(*out_tip), 0, sizeof(*out_tip));
    git_reference *current = NULL;
    bool found = false;
    uint64_t best_time = 0;

    while (git_reference_next(&current, iter) == 0) {
        const git_oid *target = git_reference_target(current);
        if (target == NULL) {
            git_reference_free(current);
            current = NULL;
            continue;
        }

        git_commit *commit = NULL;
        if (git_commit_lookup(&commit, state->repo, target) != 0) {
            git_reference_free(current);
            current = NULL;
            continue;
        }

        uint64_t commit_time = (uint64_t)git_commit_time(commit);
        git_commit_free(commit);

        if (!found || commit_time > best_time) {
            found = true;
            best_time = commit_time;
            out_tip->has_target = true;
            out_tip->oid = *target;
            out_tip->commit_time = commit_time;
            git_oid_tostr(out_tip->oid_hex, sizeof(out_tip->oid_hex), target);
        }

        git_reference_free(current);
        current = NULL;
    }

    git_reference_iterator_free(iter);
    if (current != NULL) {
        git_reference_free(current);
    }

    if (!found) {
        gm_memset_safe(out_tip, sizeof(*out_tip), 0, sizeof(*out_tip));
    }

    return gm_ok_void();
}

static gm_result_void_t commit_read_message_impl(
    gm_libgit2_repository_port_state_t *state, const gm_oid_t *commit_oid,
    char **out_message) {
    if (commit_oid == NULL || out_message == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "commit read message requires inputs"));
    }

    git_commit *commit = NULL;
    if (git_commit_lookup(&commit, state->repo, commit_oid) != 0) {
        return gm_err_void(
            GM_ERROR(GM_ERR_NOT_FOUND, "commit not found while reading message"));
    }

    const char *message = git_commit_message_raw(commit);
    if (message == NULL) {
        git_commit_free(commit);
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_FORMAT, "commit message missing"));
    }

    char *copy = strdup(message);
    git_commit_free(commit);
    if (copy == NULL) {
        return gm_err_void(
            GM_ERROR(GM_ERR_OUT_OF_MEMORY, "allocating commit message copy failed"));
    }

    *out_message = copy;
    return gm_ok_void();
}

static gm_result_void_t commit_read_blob_impl(
    gm_libgit2_repository_port_state_t *state, const gm_oid_t *commit_oid,
    const char *path, uint8_t **out_data, size_t *out_size) {
    if (commit_oid == NULL || path == NULL || out_data == NULL ||
        out_size == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "commit read blob requires inputs"));
    }

    git_commit *commit = NULL;
    if (git_commit_lookup(&commit, state->repo, commit_oid) != 0) {
        return gm_err_void(
            GM_ERROR(GM_ERR_NOT_FOUND, "commit not found for blob read"));
    }

    git_tree *tree = NULL;
    if (git_commit_tree(&tree, commit) != 0) {
        git_commit_free(commit);
        return gm_err_void(
            GM_ERROR(GM_ERR_UNKNOWN, "unable to read commit tree"));
    }

    git_tree_entry *entry = NULL;
    if (git_tree_entry_bypath(&entry, tree, path) != 0) {
        git_tree_free(tree);
        git_commit_free(commit);
        return gm_err_void(
            GM_ERROR(GM_ERR_NOT_FOUND, "path %s not found in commit", path));
    }

    git_blob *blob = NULL;
    if (git_blob_lookup(&blob, state->repo, git_tree_entry_id(entry)) != 0) {
        git_tree_entry_free(entry);
        git_tree_free(tree);
        git_commit_free(commit);
        return gm_err_void(
            GM_ERROR(GM_ERR_UNKNOWN, "unable to load blob for %s", path));
    }

    size_t size = (size_t)git_blob_rawsize(blob);
    const void *raw = git_blob_rawcontent(blob);

    uint8_t *buffer = NULL;
    if (size > 0U) {
        buffer = (uint8_t *)malloc(size);
        if (buffer == NULL) {
            git_blob_free(blob);
            git_tree_entry_free(entry);
            git_tree_free(tree);
            git_commit_free(commit);
            return gm_err_void(
                GM_ERROR(GM_ERR_OUT_OF_MEMORY, "allocating blob buffer failed"));
        }
        memcpy(buffer, raw, size);
    }

    *out_data = buffer;
    *out_size = size;

    git_blob_free(blob);
    git_tree_entry_free(entry);
    git_tree_free(tree);
    git_commit_free(commit);

    return gm_ok_void();
}

/* NOLINTNEXTLINE(misc-no-recursion) */
static gm_result_void_t tree_size_recursive(git_repository *repo,
                                            const git_oid *tree_oid,
                                            uint64_t *total_size) {
    git_tree *tree = NULL;
    if (git_tree_lookup(&tree, repo, tree_oid) != 0) {
        return gm_err_void(
            GM_ERROR(GM_ERR_UNKNOWN, "unable to lookup tree while sizing"));
    }

    git_odb *odb = NULL;
    if (git_repository_odb(&odb, repo) == 0) {
        size_t tree_size = 0;
        git_object_t tree_type = GIT_OBJECT_INVALID;
        if (git_odb_read_header(&tree_size, &tree_type, odb, tree_oid) == 0) {
            *total_size += tree_size;
        }
        git_odb_free(odb);
    }

    size_t entry_count = git_tree_entrycount(tree);
    for (size_t idx = 0; idx < entry_count; ++idx) {
        const git_tree_entry *entry = git_tree_entry_byindex(tree, idx);
        if (entry == NULL) {
            continue;
        }

        const git_oid *entry_oid = git_tree_entry_id(entry);
        git_filemode_t mode = git_tree_entry_filemode(entry);

        if (mode == GIT_FILEMODE_TREE) {
            gm_result_void_t sub_result =
                tree_size_recursive(repo, entry_oid, total_size);
            if (!sub_result.ok) {
                git_tree_free(tree);
                return sub_result;
            }
            continue;
        }

        if (mode == GIT_FILEMODE_BLOB) {
            git_odb *blob_odb = NULL;
            if (git_repository_odb(&blob_odb, repo) == 0) {
                size_t blob_size = 0;
                git_object_t blob_type = GIT_OBJECT_INVALID;
                if (git_odb_read_header(&blob_size, &blob_type, blob_odb,
                                        entry_oid) == 0) {
                    *total_size += blob_size;
                }
                git_odb_free(blob_odb);
            }
        }
    }

    git_tree_free(tree);
    return gm_ok_void();
}

static gm_result_void_t commit_tree_size_impl(
    gm_libgit2_repository_port_state_t *state, const gm_oid_t *commit_oid,
    uint64_t *out_size_bytes) {
    if (commit_oid == NULL || out_size_bytes == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "commit tree size requires inputs"));
    }

    git_commit *commit = NULL;
    if (git_commit_lookup(&commit, state->repo, commit_oid) != 0) {
        return gm_err_void(
            GM_ERROR(GM_ERR_NOT_FOUND, "commit not found while sizing"));
    }

    const git_oid *tree_oid = git_commit_tree_id(commit);
    if (tree_oid == NULL) {
        git_commit_free(commit);
        return gm_err_void(
            GM_ERROR(GM_ERR_UNKNOWN, "commit missing tree while sizing"));
    }

    uint64_t total = 0;
    gm_result_void_t result = tree_size_recursive(state->repo, tree_oid, &total);
    git_commit_free(commit);

    if (!result.ok) {
        return result;
    }

    *out_size_bytes = total;
    return gm_ok_void();
}

static gm_result_void_t commit_walk_impl(gm_libgit2_repository_port_state_t *state,
                                         const char *ref_name,
                                         gm_git_commit_visit_cb visit_callback,
                                         void *userdata) {
    if (ref_name == NULL || visit_callback == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "commit walk requires ref and callback"));
    }

    git_revwalk *walk = NULL;
    int commit_count = 0;
    if (git_revwalk_new(&walk, state->repo) != 0) {
        return gm_err_void(
            GM_ERROR(GM_ERR_UNKNOWN, "unable to allocate revwalk"));
    }

    git_revwalk_sorting(walk, GIT_SORT_TIME);
    if (git_revwalk_push_ref(walk, ref_name) != 0) {
        git_revwalk_free(walk);
        return gm_err_void(
            GM_ERROR(GM_ERR_NOT_FOUND, "unable to push ref %s", ref_name));
    }

    git_oid oid;
    while (git_revwalk_next(&oid, walk) == 0) {
        int cb_result = visit_callback(&oid, userdata);
        commit_count++;
        if (cb_result != GM_OK) {
            git_revwalk_free(walk);
            return gm_err_void(GM_ERROR(cb_result, "commit walk callback stop"));
        }
    }

    git_revwalk_free(walk);
    if (commit_count == 0) {
        return gm_err_void(GM_ERROR(GM_ERR_NOT_FOUND, "no commits for ref"));
    }
    return gm_ok_void();
}

static gm_result_void_t commit_create_impl(
    gm_libgit2_repository_port_state_t *state, const gm_git_commit_spec_t *spec,
    gm_oid_t *out_commit_oid) {
    if (spec == NULL || spec->tree_oid == NULL || spec->message == NULL ||
        out_commit_oid == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "commit spec missing inputs"));
    }

    git_signature *sig = NULL;
    if (git_signature_default(&sig, state->repo) < 0) {
        return gm_err_void(
            GM_ERROR(GM_ERR_UNKNOWN, "unable to create default signature"));
    }

    git_tree *tree = NULL;
    if (git_tree_lookup(&tree, state->repo, spec->tree_oid) < 0) {
        git_signature_free(sig);
        return gm_err_void(
            GM_ERROR(GM_ERR_UNKNOWN, "unable to look up commit tree"));
    }

    const git_commit **parents = NULL;
    unsigned int parent_count = 0U;
    if (spec->parent_count > 0U) {
        if (spec->parents == NULL) {
            git_tree_free(tree);
            git_signature_free(sig);
            return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                        "parent array missing"));
        }
        if (spec->parent_count > UINT_MAX) {
            git_tree_free(tree);
            git_signature_free(sig);
            return gm_err_void(
                GM_ERROR(GM_ERR_INVALID_ARGUMENT, "too many parents"));
        }
        parent_count = (unsigned int)spec->parent_count;
        parents = calloc(parent_count, sizeof(*parents));
        if (parents == NULL) {
            git_tree_free(tree);
            git_signature_free(sig);
            return gm_err_void(
                GM_ERROR(GM_ERR_OUT_OF_MEMORY, "allocating parent list failed"));
        }
        for (unsigned int i = 0; i < parent_count; ++i) {
            git_commit *parent_commit = NULL;
            if (git_commit_lookup(&parent_commit, state->repo,
                                  &spec->parents[i]) != 0) {
                for (unsigned int j = 0; j < i; ++j) {
                    git_commit_free((git_commit *)parents[j]);
                }
                free((void *)parents);
                git_tree_free(tree);
                git_signature_free(sig);
                return gm_err_void(
                    GM_ERROR(GM_ERR_NOT_FOUND, "parent commit missing"));
            }
            parents[i] = parent_commit;
        }
    }

    git_buf buffer = {0};
    int git_status = git_commit_create_buffer(&buffer, state->repo, sig, sig, NULL,
                                              spec->message, tree, parent_count,
                                              parents);
    for (unsigned int i = 0; i < parent_count; ++i) {
        git_commit_free((git_commit *)parents[i]);
    }
    free((void *)parents);

    if (git_status < 0) {
        git_tree_free(tree);
        git_signature_free(sig);
        return gm_err_void(
            GM_ERROR(GM_ERR_UNKNOWN, "unable to create commit buffer"));
    }

    git_odb *odb = NULL;
    git_status = git_repository_odb(&odb, state->repo);
    if (git_status == 0) {
        git_status =
            git_odb_write(out_commit_oid, odb, buffer.ptr, buffer.size, GIT_OBJECT_COMMIT);
        git_odb_free(odb);
    }

    git_buf_dispose(&buffer);
    git_tree_free(tree);
    git_signature_free(sig);

    if (git_status < 0) {
        return gm_err_void(
            GM_ERROR(GM_ERR_UNKNOWN, "unable to write commit object"));
    }

    return gm_ok_void();
}

static gm_result_void_t reference_update_impl(
    gm_libgit2_repository_port_state_t *state,
    const gm_git_reference_update_spec_t *spec) {
    if (spec == NULL || spec->ref_name == NULL || spec->target_oid == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "reference update spec missing inputs"));
    }

    git_reference *ref = NULL;
    int git_status = git_reference_create(&ref, state->repo, spec->ref_name,
                                          spec->target_oid, spec->force,
                                          spec->log_message);
    if (ref != NULL) {
        git_reference_free(ref);
    }

    if (git_status < 0) {
        return gm_err_void(
            GM_ERROR(GM_ERR_UNKNOWN, "failed to update reference %s", spec->ref_name));
    }

    return gm_ok_void();
}

static gm_result_void_t repository_path_bridge(
    void *self, gm_git_repository_path_kind_t kind, char *out_buffer,
    size_t buffer_size) {
    return repository_path_impl((gm_libgit2_repository_port_state_t *)self, kind,
                                out_buffer, buffer_size);
}

static gm_result_void_t head_branch_bridge(void *self, char *out_name,
                                           size_t out_name_size) {
    return head_branch_impl((gm_libgit2_repository_port_state_t *)self, out_name,
                            out_name_size);
}

static gm_result_void_t build_tree_from_directory_bridge(void *self,
                                                         const char *dir_path,
                                                         gm_oid_t *out_tree_oid) {
    return build_tree_from_directory_impl((gm_libgit2_repository_port_state_t *)self,
                                          dir_path, out_tree_oid);
}

static gm_result_void_t reference_tip_bridge(void *self, const char *ref_name,
                                             gm_git_reference_tip_t *out_tip) {
    return reference_tip_impl((gm_libgit2_repository_port_state_t *)self, ref_name,
                              out_tip);
}

static gm_result_void_t reference_glob_latest_bridge(
    void *self, const char *pattern, gm_git_reference_tip_t *out_tip) {
    return reference_glob_latest_impl((gm_libgit2_repository_port_state_t *)self,
                                      pattern, out_tip);
}

static gm_result_void_t commit_create_bridge(void *self,
                                             const gm_git_commit_spec_t *spec,
                                             gm_oid_t *out_commit_oid) {
    return commit_create_impl((gm_libgit2_repository_port_state_t *)self, spec,
                              out_commit_oid);
}

static gm_result_void_t commit_read_blob_bridge(void *self,
                                                const gm_oid_t *commit_oid,
                                                const char *path,
                                                uint8_t **out_data,
                                                size_t *out_size) {
    return commit_read_blob_impl((gm_libgit2_repository_port_state_t *)self,
                                 commit_oid, path, out_data, out_size);
}

static gm_result_void_t commit_read_message_bridge(
    void *self, const gm_oid_t *commit_oid, char **out_message) {
    return commit_read_message_impl((gm_libgit2_repository_port_state_t *)self,
                                    commit_oid, out_message);
}

static void commit_message_dispose_bridge(void *self, char *message) {
    (void)self;
    free(message);
}

static gm_result_void_t commit_tree_size_bridge(void *self,
                                                const gm_oid_t *commit_oid,
                                                uint64_t *out_size_bytes) {
    return commit_tree_size_impl((gm_libgit2_repository_port_state_t *)self,
                                 commit_oid, out_size_bytes);
}

static gm_result_void_t walk_commits_bridge(void *self, const char *ref_name,
                                            gm_git_commit_visit_cb visit_callback,
                                            void *userdata) {
    return commit_walk_impl((gm_libgit2_repository_port_state_t *)self, ref_name,
                            visit_callback, userdata);
}

static gm_result_void_t reference_update_bridge(
    void *self, const gm_git_reference_update_spec_t *spec) {
    return reference_update_impl((gm_libgit2_repository_port_state_t *)self, spec);
}

static const gm_git_repository_port_vtbl_t GM_LIBGIT2_REPOSITORY_PORT_VTBL = {
    .repository_path = repository_path_bridge,
    .head_branch = head_branch_bridge,
    .build_tree_from_directory = build_tree_from_directory_bridge,
    .reference_tip = reference_tip_bridge,
    .reference_glob_latest = reference_glob_latest_bridge,
    .commit_read_blob = commit_read_blob_bridge,
    .commit_read_message = commit_read_message_bridge,
    .commit_message_dispose = commit_message_dispose_bridge,
    .walk_commits = walk_commits_bridge,
    .commit_tree_size = commit_tree_size_bridge,
    .commit_create = commit_create_bridge,
    .reference_update = reference_update_bridge,
};

static void dispose_repository_port(gm_git_repository_port_t *port) {
    if (port == NULL || port->self == NULL) {
        return;
    }

    gm_libgit2_repository_port_state_t *state =
        (gm_libgit2_repository_port_state_t *)port->self;
    port->self = NULL;
    port->vtbl = NULL;
    free(state);
}

gm_result_void_t gm_libgit2_repository_port_create(
    gm_git_repository_port_t *out_port,
    gm_libgit2_repository_port_state_t **out_state,
    void (**out_dispose)(gm_git_repository_port_t *), git_repository *repo) {
    if (out_port == NULL || repo == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "repository port requires output and repo"));
    }

    gm_libgit2_repository_port_state_t *state =
        calloc(1, sizeof(gm_libgit2_repository_port_state_t));
    if (state == NULL) {
        return gm_err_void(
            GM_ERROR(GM_ERR_OUT_OF_MEMORY, "allocating repository port state"));
    }

    state->repo = repo;
    out_port->vtbl = &GM_LIBGIT2_REPOSITORY_PORT_VTBL;
    out_port->self = state;

    if (out_state != NULL) {
        *out_state = state;
    }
    if (out_dispose != NULL) {
        *out_dispose = dispose_repository_port;
    }

    return gm_ok_void();
}
