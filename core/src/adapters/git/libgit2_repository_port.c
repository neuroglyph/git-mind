/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/adapters/git/libgit2_repository_port.h"

#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <git2/blob.h> // IWYU pragma: keep
#include <git2/buffer.h> // IWYU pragma: keep
#include <git2/commit.h> // IWYU pragma: keep
#include <git2/errors.h> // IWYU pragma: keep
#include <git2/graph.h> // IWYU pragma: keep
#include <git2/object.h> // IWYU pragma: keep
#include <git2/odb.h> // IWYU pragma: keep
#include <git2/oid.h> // IWYU pragma: keep
#include <git2/refs.h> // IWYU pragma: keep
#include <git2/repository.h> // IWYU pragma: keep
#include <git2/revwalk.h> // IWYU pragma: keep
#include <git2/signature.h> // IWYU pragma: keep
#include <git2/tree.h> // IWYU pragma: keep
#include <git2/types.h> // IWYU pragma: keep

#include "gitmind/constants.h"
#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/ports/git_repository_port.h"
#include "gitmind/security/memory.h"
#include "gitmind/security/string.h"
#include "gitmind/util/memory.h"
#include "gitmind/types.h" // IWYU pragma: keep
#include "gitmind/util/oid.h"
#include "gitmind/util/errno_compat.h"

#ifndef GIT_PATH_MAX
#define GIT_PATH_MAX 4096
#endif

struct gm_libgit2_repository_port_state {
    git_repository *repo;
};

enum {
    DIR_STACK_INITIAL_CAPACITY = 8,
    TREE_STACK_INITIAL_CAPACITY = 8,
    REF_DIRECTORY_MODE = 0775,
};

typedef struct {
    char path[GIT_PATH_MAX];
    char entry_name[GIT_PATH_MAX];
    git_treebuilder *builder;
    DIR *dir;
    bool has_parent;
} gm_directory_builder_frame_t;

static gm_result_void_t ignore_error_code(gm_result_void_t result,
                                          int32_t code) {
    if (!result.ok && result.u.err != NULL && result.u.err->code == code) {
        gm_error_free(result.u.err);
        return gm_ok_void();
    }
    return result;
}

static gm_result_void_t normalize_reference_name(const char *ref_name,
                                                 char *out, size_t out_size) {
    if (git_reference_normalize_name(out, out_size, ref_name,
                                     GIT_REFERENCE_FORMAT_NORMAL) != 0) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "invalid reference name %s", ref_name));
    }

    if (strstr(out, "..") != NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "invalid traversal in reference name"));
    }

    return gm_ok_void();
}

static gm_result_void_t ensure_path_prefix_directories(char *path,
                                                       size_t path_length) {
    for (size_t index = 0; index < path_length; ++index) {
        if (index == 0 || path[index] != '/') {
            continue;
        }

        char saved = path[index];
        path[index] = '\0';

        if (mkdir(path, REF_DIRECTORY_MODE) < 0) {
            int mkdir_err = errno;
            gm_result_void_t mkdir_result =
                gm_errno_to_result("mkdir", path, mkdir_err);
            mkdir_result = ignore_error_code(mkdir_result, GM_ERR_ALREADY_EXISTS);
            path[index] = saved;
            if (!mkdir_result.ok) {
                return mkdir_result;
            }
        } else {
            path[index] = saved;
        }
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

static void directory_stack_cleanup(gm_directory_builder_frame_t *frames,
                                    size_t count) {
    if (frames == NULL) {
        return;
    }
    for (size_t index = 0; index < count; ++index) {
        if (frames[index].dir != NULL) {
            closedir(frames[index].dir);
            frames[index].dir = NULL;
        }
        if (frames[index].builder != NULL) {
            git_treebuilder_free(frames[index].builder);
            frames[index].builder = NULL;
        }
    }
}

static gm_result_void_t directory_stack_push(gm_directory_builder_frame_t **frames,
                                             size_t *count, size_t *capacity,
                                             const char *path,
                                             const char *entry_name,
                                             git_treebuilder *builder,
                                             DIR *dir,
                                             bool has_parent) {
    if (frames == NULL || count == NULL || capacity == NULL || path == NULL ||
        builder == NULL || dir == NULL) {
        closedir(dir);
        git_treebuilder_free(builder);
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, "invalid directory frame"));
    }

    if (*count == *capacity) {
        size_t new_capacity = (*capacity == 0U) ? DIR_STACK_INITIAL_CAPACITY
                                                : (*capacity * 2U);
        gm_directory_builder_frame_t *resized =
            (gm_directory_builder_frame_t *)realloc(
                *frames, new_capacity * sizeof(**frames));
        if (resized == NULL) {
            closedir(dir);
            git_treebuilder_free(builder);
            return gm_err_void(GM_ERROR(GM_ERR_OUT_OF_MEMORY,
                                        "directory stack allocation failed"));
        }
        *frames = resized;
        *capacity = new_capacity;
    }

    gm_directory_builder_frame_t *frame = &(*frames)[*count];
    frame->builder = NULL;
    frame->dir = NULL;
    frame->has_parent = has_parent;
    frame->entry_name[0] = '\0';
    frame->path[0] = '\0';

    if (gm_strcpy_safe(frame->path, sizeof(frame->path), path) != GM_OK) {
        closedir(dir);
        git_treebuilder_free(builder);
        return gm_err_void(
            GM_ERROR(GM_ERR_PATH_TOO_LONG, "directory path exceeds buffer"));
    }

    if (has_parent) {
        if (entry_name == NULL ||
            gm_strcpy_safe(frame->entry_name, sizeof(frame->entry_name),
                           entry_name) != GM_OK) {
            closedir(dir);
            git_treebuilder_free(builder);
            return gm_err_void(
                GM_ERROR(GM_ERR_PATH_TOO_LONG,
                         "directory entry name exceeds buffer"));
        }
    }

    frame->builder = builder;
    frame->dir = dir;
    *count += 1U;
    return gm_ok_void();
}

static gm_result_void_t push_child_directory(
    gm_libgit2_repository_port_state_t *state, const char *full_path,
    const char *entry_name, gm_directory_builder_frame_t **frames,
    size_t *count, size_t *capacity) {
    git_treebuilder *child_builder = NULL;
    if (git_treebuilder_new(&child_builder, state->repo, NULL) < 0) {
        return gm_err_void(
            GM_ERROR(GM_ERR_UNKNOWN,
                     "treebuilder allocation failed for %s", full_path));
    }

    DIR *child_dir = opendir(full_path);
    if (child_dir == NULL) {
        int opendir_err = errno;
        git_treebuilder_free(child_builder);
        return gm_errno_to_result("opendir", full_path, opendir_err);
    }

    return directory_stack_push(frames, count, capacity, full_path, entry_name,
                                child_builder, child_dir, true);
}

static gm_result_void_t finalize_directory_frame(
    gm_directory_builder_frame_t *frames, size_t *count, gm_oid_t *out_tree_oid) {
    if (count == NULL || *count == 0U) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "directory stack underflow"));
    }

    gm_directory_builder_frame_t *frame = &frames[*count - 1U];
    git_oid tree_oid;
    if (git_treebuilder_write(&tree_oid, frame->builder) < 0) {
        return gm_err_void(
            GM_ERROR(GM_ERR_UNKNOWN, "unable to write tree for %s", frame->path));
    }

    git_treebuilder_free(frame->builder);
    frame->builder = NULL;
    closedir(frame->dir);
    frame->dir = NULL;

    bool has_parent = frame->has_parent;
    char child_name[GIT_PATH_MAX] = {0};
    if (has_parent) {
        if (gm_strcpy_safe(child_name, sizeof(child_name), frame->entry_name) != GM_OK) {
            return gm_err_void(
                GM_ERROR(GM_ERR_PATH_TOO_LONG, "child directory name exceeds buffer"));
        }
    }

    *count -= 1U;

    if (!has_parent) {
        *out_tree_oid = tree_oid;
        return gm_ok_void();
    }

    if (*count == 0U) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, "missing parent directory frame"));
    }

    gm_directory_builder_frame_t *parent = &frames[*count - 1U];
    if (git_treebuilder_insert(NULL, parent->builder, child_name, &tree_oid,
                               GIT_FILEMODE_TREE) < 0) {
        return gm_err_void(
            GM_ERROR(GM_ERR_UNKNOWN, "unable to insert tree %s", child_name));
    }

    return gm_ok_void();
}

static gm_result_void_t process_directory_entry(
    gm_libgit2_repository_port_state_t *state,
    gm_directory_builder_frame_t **frames,
    size_t *count,
    size_t *capacity,
    gm_directory_builder_frame_t *frame,
    const char *entry_name) {
    char full_path[GIT_PATH_MAX];
    int composed =
        gm_snprintf(full_path, sizeof(full_path), "%s/%s", frame->path, entry_name);
    if (composed < 0 || (size_t)composed >= sizeof(full_path)) {
        return gm_err_void(
            GM_ERROR(GM_ERR_PATH_TOO_LONG, "entry path exceeds buffer"));
    }

    struct stat entry_stat = {0};
    if (stat(full_path, &entry_stat) != 0) {
        int stat_err = errno;
        gm_result_void_t stat_result =
            gm_errno_to_result("stat", full_path, stat_err);
        return ignore_error_code(stat_result, GM_ERR_NOT_FOUND);
    }

    if (S_ISDIR(entry_stat.st_mode)) {
        return push_child_directory(state, full_path, entry_name, frames, count,
                                    capacity);
    }

    if (!S_ISREG(entry_stat.st_mode)) {
        return gm_ok_void();
    }

    git_oid blob_oid;
    gm_result_void_t blob_result =
        write_blob_from_file(state->repo, full_path, &blob_oid);
    if (!blob_result.ok) {
        return blob_result;
    }

    if (git_treebuilder_insert(NULL, frame->builder, entry_name, &blob_oid,
                               GIT_FILEMODE_BLOB) < 0) {
        return gm_err_void(
            GM_ERROR(GM_ERR_UNKNOWN, "unable to insert blob for %s", entry_name));
    }

    return gm_ok_void();
}

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

static bool should_skip_entry(const char *name) {
    if (name == NULL) {
        return true;
    }
    if (strcmp(name, ".") == 0) {
        return true;
    }
    if (strcmp(name, "..") == 0) {
        return true;
    }
    return false;
}

static gm_result_void_t walk_directory_frames(
    gm_libgit2_repository_port_state_t *state,
    gm_directory_builder_frame_t **frames,
    size_t *frame_count,
    size_t *frame_capacity,
    gm_oid_t *out_tree_oid) {
    while (*frame_count > 0U) {
        gm_directory_builder_frame_t *current = &(*frames)[*frame_count - 1U];
        struct dirent *entry = readdir(current->dir);
        if (entry == NULL) {
            gm_result_void_t finalize_result =
                finalize_directory_frame(*frames, frame_count, out_tree_oid);
            if (!finalize_result.ok) {
                return finalize_result;
            }
            continue;
        }

        if (should_skip_entry(entry->d_name)) {
            continue;
        }

        gm_result_void_t entry_result =
            process_directory_entry(state, frames, frame_count, frame_capacity,
                                    current, entry->d_name);
        if (!entry_result.ok) {
            return entry_result;
        }
    }

    return gm_ok_void();
}

static gm_result_void_t ensure_commit_signature(git_repository *repo,
                                                git_signature **out_signature) {
    git_signature *signature = NULL;
    int status = git_signature_default(&signature, repo);
    if (status == 0) {
        *out_signature = signature;
        return gm_ok_void();
    }

    if (status == GIT_ENOTFOUND) {
        if (git_signature_now(&signature, "gitmind", "gitmind@example.invalid") <
            0) {
            return gm_err_void(GM_ERROR(GM_ERR_UNKNOWN,
                                        "unable to create fallback commit signature"));
        }
        *out_signature = signature;
        return gm_ok_void();
    }

    return gm_err_void(
        GM_ERROR(GM_ERR_UNKNOWN, "default commit signature lookup failed"));
}

static gm_result_void_t populate_commit_parents(
    gm_libgit2_repository_port_state_t *state,
    const gm_git_commit_spec_t *spec,
    git_commit ***out_parents,
    unsigned int *out_parent_count) {
    *out_parents = NULL;
    *out_parent_count = 0U;

    if (spec->parent_count == 0U) {
        return gm_ok_void();
    }

    if (spec->parents == NULL) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, "parent array missing"));
    }

    if (spec->parent_count > UINT_MAX) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, "too many parents"));
    }

    unsigned int parent_count = (unsigned int)spec->parent_count;
    git_commit **parents = (git_commit **)calloc(parent_count, sizeof(*parents));
    if (parents == NULL) {
        return gm_err_void(
            GM_ERROR(GM_ERR_OUT_OF_MEMORY, "allocating parent list failed"));
    }

    for (unsigned int index = 0; index < parent_count; ++index) {
        git_commit *parent_commit = NULL;
        if (git_commit_lookup(&parent_commit, state->repo, &spec->parents[index]) !=
            0) {
            for (unsigned int cleanup = 0; cleanup < index; ++cleanup) {
                git_commit_free(parents[cleanup]);
            }
            free((void *)parents);
            return gm_err_void(
                GM_ERROR(GM_ERR_NOT_FOUND, "parent commit missing"));
        }
        parents[index] = parent_commit;
    }

    *out_parents = parents;
    *out_parent_count = parent_count;
    return gm_ok_void();
}

static gm_result_void_t build_tree_from_directory_impl(
    gm_libgit2_repository_port_state_t *state, const char *dir_path,
    gm_oid_t *out_tree_oid) {
    if (dir_path == NULL || out_tree_oid == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "tree build requires directory and output"));
    }
    gm_memset_safe(out_tree_oid, sizeof(*out_tree_oid), 0, sizeof(*out_tree_oid));

    git_treebuilder *root_builder = NULL;
    if (git_treebuilder_new(&root_builder, state->repo, NULL) < 0) {
        return gm_err_void(
            GM_ERROR(GM_ERR_UNKNOWN, "unable to create root treebuilder"));
    }

    DIR *root_dir = opendir(dir_path);
    if (root_dir == NULL) {
        git_treebuilder_free(root_builder);
        int opendir_err = errno;
        return gm_errno_to_result("opendir", dir_path, opendir_err);
    }

    gm_directory_builder_frame_t *frames = NULL;
    size_t frame_count = 0U;
    size_t frame_capacity = 0U;

    gm_result_void_t push_result = directory_stack_push(&frames, &frame_count,
                                                        &frame_capacity, dir_path,
                                                        NULL, root_builder, root_dir,
                                                        false);
    if (!push_result.ok) {
        free(frames);
        return push_result;
    }

    gm_result_void_t walk_result = walk_directory_frames(state, &frames, &frame_count,
                                                        &frame_capacity, out_tree_oid);
    if (!walk_result.ok) {
        directory_stack_cleanup(frames, frame_count);
    }

    free(frames);
    return walk_result;
}

static void reset_reference_tip(gm_git_reference_tip_t *tip) {
    if (tip == NULL) {
        return;
    }
    tip->has_target = false;
    gm_memset_safe(&tip->oid, sizeof(tip->oid), 0, sizeof(tip->oid));
    tip->commit_time = 0;
    tip->oid_hex[0] = '\0';
}

static void store_reference_tip(gm_git_reference_tip_t *tip,
                                const git_oid *target,
                                uint64_t commit_time) {
    tip->has_target = true;
    tip->oid = *target;
    tip->commit_time = commit_time;
    git_oid_tostr(tip->oid_hex, sizeof(tip->oid_hex), target);
}

static gm_result_void_t consider_reference_candidate(
    gm_libgit2_repository_port_state_t *state,
    gm_git_reference_tip_t *tip,
    git_reference *reference,
    bool *found,
    uint64_t *best_time) {
    const git_oid *target = git_reference_target(reference);
    if (target == NULL) {
        return gm_ok_void();
    }

    git_commit *commit = NULL;
    if (git_commit_lookup(&commit, state->repo, target) != 0) {
        return gm_ok_void();
    }

    uint64_t commit_time = (uint64_t)git_commit_time(commit);
    git_commit_free(commit);

    if (!*found || commit_time > *best_time) {
        *found = true;
        *best_time = commit_time;
        store_reference_tip(tip, target, commit_time);
    }

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

    reset_reference_tip(out_tip);
    git_reference *current = NULL;
    bool found = false;
    uint64_t best_time = 0;

    while (git_reference_next(&current, iter) == 0) {
        gm_result_void_t consider_result =
            consider_reference_candidate(state, out_tip, current, &found, &best_time);
        git_reference_free(current);
        current = NULL;
        if (!consider_result.ok) {
            git_reference_iterator_free(iter);
            return consider_result;
        }
    }

    git_reference_iterator_free(iter);
    if (current != NULL) {
        git_reference_free(current);
    }

    if (!found) {
        reset_reference_tip(out_tip);
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
    *out_message = NULL;

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

    size_t message_len = strlen(message) + 1U;
    char *copy = (char *)malloc(message_len);
    if (copy == NULL) {
        git_commit_free(commit);
        return gm_err_void(
            GM_ERROR(GM_ERR_OUT_OF_MEMORY, "allocating commit message copy failed"));
    }

    gm_memcpy_safe(copy, message_len, message, message_len);
    git_commit_free(commit);

    *out_message = copy;
    return gm_ok_void();
}

static gm_result_void_t open_blob_entry(
    gm_libgit2_repository_port_state_t *state, const gm_oid_t *commit_oid,
    const char *path, git_commit **out_commit, git_tree **out_tree,
    git_tree_entry **out_entry) {
    if (commit_oid == NULL || path == NULL || out_commit == NULL ||
        out_tree == NULL || out_entry == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "blob lookup requires inputs"));
    }

    git_commit *commit = NULL;
    if (git_commit_lookup(&commit, state->repo, commit_oid) != 0) {
        return gm_err_void(
            GM_ERROR(GM_ERR_NOT_FOUND, "commit not found for blob lookup"));
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

    *out_commit = commit;
    *out_tree = tree;
    *out_entry = entry;
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
    git_tree *tree = NULL;
    git_tree_entry *entry = NULL;
    gm_result_void_t lookup_result =
        open_blob_entry(state, commit_oid, path, &commit, &tree, &entry);
    if (!lookup_result.ok) {
        return lookup_result;
    }

    git_blob *blob = NULL;
    const git_oid *blob_oid = git_tree_entry_id(entry);
    if (blob_oid == NULL ||
        git_blob_lookup(&blob, state->repo, blob_oid) != 0) {
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
        gm_memcpy_safe(buffer, size, raw, size);
    }

    git_blob_free(blob);
    git_tree_entry_free(entry);
    git_tree_free(tree);
    git_commit_free(commit);

    *out_data = buffer;
    *out_size = size;

    return gm_ok_void();
}

static void commit_blob_dispose_impl(void *self, uint8_t *data, size_t size) {
    (void)self;
    (void)size;
    free(data);
}

static gm_result_void_t resolve_blob_at_head_impl(
    gm_libgit2_repository_port_state_t *state, const char *path,
    gm_oid_t *out_blob_oid) {
    if (path == NULL || out_blob_oid == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "blob resolve requires path and output"));
    }

    git_reference *head = NULL;
    if (git_repository_head(&head, state->repo) != 0) {
        return gm_err_void(
            GM_ERROR(GM_ERR_NOT_FOUND, "unable to resolve repository HEAD"));
    }

    const git_oid *head_oid = git_reference_target(head);
    if (head_oid == NULL) {
        git_reference_free(head);
        return gm_err_void(
            GM_ERROR(GM_ERR_NOT_FOUND, "repository HEAD has no target"));
    }

    git_commit *commit = NULL;
    git_tree *tree = NULL;
    git_tree_entry *entry = NULL;
    gm_result_void_t lookup_result =
        open_blob_entry(state, head_oid, path, &commit, &tree, &entry);
    git_reference_free(head);
    if (!lookup_result.ok) {
        return lookup_result;
    }

    const git_oid *blob_oid = git_tree_entry_id(entry);
    if (blob_oid == NULL) {
        git_tree_entry_free(entry);
        git_tree_free(tree);
        git_commit_free(commit);
        return gm_err_void(
            GM_ERROR(GM_ERR_NOT_FOUND, "blob entry missing for %s", path));
    }

    *out_blob_oid = *blob_oid;

    git_tree_entry_free(entry);
    git_tree_free(tree);
    git_commit_free(commit);

    return gm_ok_void();
}

static gm_result_void_t resolve_blob_at_commit_impl(
    gm_libgit2_repository_port_state_t *state, const gm_oid_t *commit_oid,
    const char *path, gm_oid_t *out_blob_oid) {
    if (commit_oid == NULL || path == NULL || out_blob_oid == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "blob resolve requires commit, path, output"));
    }

    git_commit *commit = NULL;
    git_tree *tree = NULL;
    git_tree_entry *entry = NULL;
    gm_result_void_t lookup_result =
        open_blob_entry(state, commit_oid, path, &commit, &tree, &entry);
    if (!lookup_result.ok) {
        return lookup_result;
    }

    const git_oid *blob_oid = git_tree_entry_id(entry);
    if (blob_oid == NULL) {
        git_tree_entry_free(entry);
        git_tree_free(tree);
        git_commit_free(commit);
        return gm_err_void(
            GM_ERROR(GM_ERR_NOT_FOUND, "blob entry missing for %s", path));
    }

    *out_blob_oid = *blob_oid;

    git_tree_entry_free(entry);
    git_tree_free(tree);
    git_commit_free(commit);

    return gm_ok_void();
}

static gm_result_void_t ensure_reference_directories(git_repository *repo,
                                                     const char *ref_name) {
    const char *gitdir = git_repository_path(repo);
    if (gitdir == NULL) {
        return gm_ok_void();
    }

    char norm_ref[GIT_PATH_MAX];
    gm_result_void_t norm_result = normalize_reference_name(ref_name, norm_ref,
                                                            sizeof(norm_ref));
    if (!norm_result.ok) {
        return norm_result;
    }

    char fullpath[GIT_PATH_MAX];
    int written = gm_snprintf(fullpath, sizeof(fullpath), "%s%s", gitdir, norm_ref);
    if (written < 0 || (size_t)written >= sizeof(fullpath)) {
        return gm_err_void(GM_ERROR(GM_ERR_PATH_TOO_LONG, "ref path too long"));
    }

    return ensure_path_prefix_directories(fullpath, (size_t)written);
}

static gm_result_void_t create_reference_if_missing(
    gm_libgit2_repository_port_state_t *state,
    const gm_git_reference_update_spec_t *spec) {
    GM_TRY(ensure_reference_directories(state->repo, spec->ref_name));

    git_reference *created = NULL;
    int create_status = git_reference_create(&created, state->repo, spec->ref_name,
                                             spec->target_oid, 0,
                                             spec->log_message);
    if (created != NULL) {
        git_reference_free(created);
    }

    if (create_status < 0) {
        return gm_err_void(GM_ERROR(GM_ERR_UNKNOWN,
                                    "failed to create reference %s", spec->ref_name));
    }

    return gm_ok_void();
}

static gm_result_void_t update_existing_reference(
    gm_libgit2_repository_port_state_t *state,
    git_reference *existing,
    const gm_git_reference_update_spec_t *spec) {
    const git_oid *current_target = git_reference_target(existing);
    if (current_target == NULL) {
        return gm_err_void(
            GM_ERROR(GM_ERR_UNKNOWN, "reference %s missing target", spec->ref_name));
    }

    if (!gm_oid_equal(current_target, spec->target_oid)) {
        int descendant =
            git_graph_descendant_of(state->repo, spec->target_oid, current_target);
        if (descendant < 0) {
            return gm_err_void(GM_ERROR(GM_ERR_UNKNOWN,
                                        "failed to check ancestry for %s",
                                        spec->ref_name));
        }
        if (descendant == 0 && !spec->force) {
            return gm_err_void(GM_ERROR(GM_ERR_ALREADY_EXISTS,
                                        "non-fast-forward update rejected for %s",
                                        spec->ref_name));
        }
    }

    git_reference *updated = NULL;
    int set_status = git_reference_set_target(&updated, existing, spec->target_oid,
                                              spec->log_message);
    if (updated != NULL) {
        git_reference_free(updated);
    }

    if (set_status < 0) {
        return gm_err_void(
            GM_ERROR(GM_ERR_UNKNOWN, "failed to update reference %s", spec->ref_name));
    }

    return gm_ok_void();
}

static gm_result_void_t commit_parent_count_impl(
    gm_libgit2_repository_port_state_t *state, const gm_oid_t *commit_oid,
    size_t *out_parent_count) {
    if (commit_oid == NULL || out_parent_count == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "parent count requires commit and output"));
    }

    git_commit *commit = NULL;
    if (git_commit_lookup(&commit, state->repo, commit_oid) != 0) {
        return gm_err_void(
            GM_ERROR(GM_ERR_NOT_FOUND, "commit not found while counting parents"));
    }

    const unsigned int parent_total = git_commit_parentcount(commit);
    git_commit_free(commit);

    *out_parent_count = (size_t)parent_total;
    return gm_ok_void();
}

typedef struct gm_tree_stack_item {
    git_oid oid;
} gm_tree_stack_item_t;

static gm_result_void_t tree_stack_push(gm_tree_stack_item_t **items,
                                        size_t *count, size_t *capacity,
                                        const git_oid *oid) {
    if (*count == *capacity) {
        size_t new_capacity = (*capacity == 0U) ? TREE_STACK_INITIAL_CAPACITY
                                                : (*capacity * 2U);
        gm_tree_stack_item_t *resized =
            (gm_tree_stack_item_t *)realloc(*items,
                                            new_capacity * sizeof(**items));
        if (resized == NULL) {
            return gm_err_void(
                GM_ERROR(GM_ERR_OUT_OF_MEMORY, "tree stack allocation failed"));
        }
        *items = resized;
        *capacity = new_capacity;
    }

    (*items)[*count].oid = *oid;
    *count += 1U;
    return gm_ok_void();
}

static void accumulate_object_size(git_odb *odb,
                                   const git_oid *oid,
                                   uint64_t *total_size) {
    size_t object_size = 0;
    git_object_t object_type = GIT_OBJECT_INVALID;
    if (git_odb_read_header(&object_size, &object_type, odb, oid) == 0) {
        *total_size += object_size;
    }
}

static gm_result_void_t accumulate_blob_size(git_odb *odb,
                                             const git_oid *blob_oid,
                                             uint64_t *total_size) {
    accumulate_object_size(odb, blob_oid, total_size);
    return gm_ok_void();
}

static gm_result_void_t process_tree_entry_for_size(
    git_odb *odb,
    gm_tree_stack_item_t **stack,
    size_t *stack_count,
    size_t *stack_capacity,
    const git_tree_entry *entry,
    uint64_t *total_size) {
    if (entry == NULL) {
        return gm_ok_void();
    }

    const git_oid *entry_oid = git_tree_entry_id(entry);
    if (entry_oid == NULL) {
        return gm_ok_void();
    }

    git_filemode_t mode = git_tree_entry_filemode(entry);
    if (mode == GIT_FILEMODE_TREE) {
        return tree_stack_push(stack, stack_count, stack_capacity, entry_oid);
    }

    if (mode == GIT_FILEMODE_BLOB) {
        return accumulate_blob_size(odb, entry_oid, total_size);
    }

    return gm_ok_void();
}

static gm_result_void_t tree_size_iterative(git_repository *repo,
                                            const git_oid *root_oid,
                                            uint64_t *total_size) {
    gm_tree_stack_item_t *stack = NULL;
    size_t stack_count = 0U;
    size_t stack_capacity = 0U;

    gm_result_void_t push_result =
        tree_stack_push(&stack, &stack_count, &stack_capacity, root_oid);
    if (!push_result.ok) {
        return push_result;
    }

    git_odb *shared_odb = NULL;
    if (git_repository_odb(&shared_odb, repo) != 0) {
        free(stack);
        return gm_err_void(
            GM_ERROR(GM_ERR_UNKNOWN, "unable to open repository object database"));
    }

    gm_result_void_t result = gm_ok_void();

    while (stack_count > 0U) {
        if (!result.ok) {
            break;
        }
        git_oid current = stack[--stack_count].oid;

        git_tree *tree = NULL;
        if (git_tree_lookup(&tree, repo, &current) != 0) {
            result = gm_err_void(
                GM_ERROR(GM_ERR_UNKNOWN, "unable to lookup tree while sizing"));
            break;
        }

        accumulate_object_size(shared_odb, &current, total_size);

        size_t entry_count = git_tree_entrycount(tree);
        for (size_t idx = 0; idx < entry_count; ++idx) {
            const git_tree_entry *entry = git_tree_entry_byindex(tree, idx);
            gm_result_void_t entry_result = process_tree_entry_for_size(
                shared_odb, &stack, &stack_count, &stack_capacity, entry, total_size);
            if (!entry_result.ok) {
                result = entry_result;
                break;
            }
        }

        git_tree_free(tree);
    }

    git_odb_free(shared_odb);
    free(stack);
    return result;
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
    gm_result_void_t result = tree_size_iterative(state->repo, tree_oid, &total);
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

    git_revwalk_sorting(walk, GIT_SORT_NONE);
    git_revwalk_simplify_first_parent(walk);
    if (git_revwalk_push_ref(walk, ref_name) != 0) {
        git_revwalk_free(walk);
        return gm_err_void(
            GM_ERROR(GM_ERR_NOT_FOUND, "unable to push ref %s", ref_name));
    }

    git_oid oid;
    while (git_revwalk_next(&oid, walk) == 0) {
        int cb_result = visit_callback(&oid, userdata);
        commit_count++;
        if (cb_result == GM_CALLBACK_STOP) {
            break;
        }
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
    gm_memset_safe(out_commit_oid, sizeof(*out_commit_oid), 0,
                   sizeof(*out_commit_oid));

    git_signature *sig = NULL;
    GM_TRY(ensure_commit_signature(state->repo, &sig));

    git_tree *tree = NULL;
    if (git_tree_lookup(&tree, state->repo, spec->tree_oid) < 0) {
        git_signature_free(sig);
        return gm_err_void(
            GM_ERROR(GM_ERR_UNKNOWN, "unable to look up commit tree"));
    }

    git_commit **parents = NULL;
    unsigned int parent_count = 0U;
    gm_result_void_t parents_result =
        populate_commit_parents(state, spec, &parents, &parent_count);
    if (!parents_result.ok) {
        git_tree_free(tree);
        git_signature_free(sig);
        return parents_result;
    }

    git_buf buffer = {0};
    int git_status = git_commit_create_buffer(
        &buffer, state->repo, sig, sig, NULL, spec->message, tree, parent_count,
        (const git_commit **)parents);
    for (unsigned int i = 0; i < parent_count; ++i) {
        git_commit_free(parents[i]);
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

    git_reference *existing = NULL;
    int lookup_status =
        git_reference_lookup(&existing, state->repo, spec->ref_name);
    if (lookup_status == GIT_ENOTFOUND) {
        return create_reference_if_missing(state, spec);
    }

    if (lookup_status < 0) {
        return gm_err_void(
            GM_ERROR(GM_ERR_UNKNOWN, "unable to lookup reference %s", spec->ref_name));
    }

    gm_result_void_t update_result =
        update_existing_reference(state, existing, spec);
    git_reference_free(existing);
    return update_result;
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

static gm_result_void_t resolve_blob_at_head_bridge(void *self, const char *path,
                                                    gm_oid_t *out_blob_oid) {
    return resolve_blob_at_head_impl((gm_libgit2_repository_port_state_t *)self,
                                     path, out_blob_oid);
}

static gm_result_void_t resolve_blob_at_commit_bridge(
    void *self, const gm_oid_t *commit_oid, const char *path,
    gm_oid_t *out_blob_oid) {
    return resolve_blob_at_commit_impl((gm_libgit2_repository_port_state_t *)self,
                                       commit_oid, path, out_blob_oid);
}

static gm_result_void_t commit_parent_count_bridge(void *self,
                                                   const gm_oid_t *commit_oid,
                                                   size_t *out_parent_count) {
    return commit_parent_count_impl((gm_libgit2_repository_port_state_t *)self,
                                    commit_oid, out_parent_count);
}

static const gm_git_repository_port_vtbl_t GM_LIBGIT2_REPOSITORY_PORT_VTBL = { // NOLINT(readability-identifier-naming)
    .repository_path = repository_path_bridge,
    .head_branch = head_branch_bridge,
    .build_tree_from_directory = build_tree_from_directory_bridge,
    .reference_tip = reference_tip_bridge,
    .reference_glob_latest = reference_glob_latest_bridge,
    .commit_read_blob = commit_read_blob_bridge,
    .blob_dispose = commit_blob_dispose_impl,
    .commit_read_message = commit_read_message_bridge,
    .commit_message_dispose = commit_message_dispose_bridge,
    .walk_commits = walk_commits_bridge,
    .commit_tree_size = commit_tree_size_bridge,
    .commit_create = commit_create_bridge,
    .reference_update = reference_update_bridge,
    .resolve_blob_at_head = resolve_blob_at_head_bridge,
    .resolve_blob_at_commit = resolve_blob_at_commit_bridge,
    .commit_parent_count = commit_parent_count_bridge,
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
