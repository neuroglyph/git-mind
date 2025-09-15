/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

/* Enable POSIX interfaces such as lstat() on all platforms */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#if !defined(__APPLE__) && !defined(_GNU_SOURCE)
#define _GNU_SOURCE
#endif

#include <dirent.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "gitmind/cache.h"
#include "gitmind/constants.h"
#include "gitmind/context.h"
#include "gitmind/error.h"
#include "gitmind/types.h"
#include "gitmind/security/string.h"

#include <git2/repository.h>
#include <git2/tree.h>
#include <git2/oid.h>
#include <git2/blob.h>

/* Forward declaration */
static int add_directory_to_tree(git_repository *repo,
                                 git_treebuilder *parent_builder,
                                 const char *dir_path, const char *rel_path);

/* Add a file to the tree builder */
static int add_file_to_tree(git_repository *repo, git_treebuilder *builder,
                            const char *file_path, const char *name) {
    git_oid blob_oid;
    int rc;

    /* Create blob from file */
    rc = git_blob_create_from_workdir(&blob_oid, repo, file_path);
    if (rc < 0) {
        return GM_ERR_UNKNOWN;
    }

    /* Add to tree */
    rc = git_treebuilder_insert(NULL, builder, name, &blob_oid,
                                GIT_FILEMODE_BLOB);
    return rc < 0 ? GM_ERR_UNKNOWN : GM_OK;
}

/* Process a single file system entry */
static int process_fs_entry(git_repository *repo, git_treebuilder *dir_builder,
                            const char *dir_path, const char *entry_name,
                            const char *rel_path) {
    char full_path[GM_PATH_MAX];
    char entry_rel_path[GM_PATH_MAX];
    struct stat st;
    int rc;

    /* Skip . and .. */
    if (strcmp(entry_name, ".") == 0 || strcmp(entry_name, "..") == 0) {
        return GM_OK;
    }

    /* Build full path */
    {
        int rn = gm_snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry_name);
        if (rn < 0 || (size_t)rn >= sizeof(full_path)) {
            return GM_ERR_BUFFER_TOO_SMALL;
        }
    }

    /* Get file info - use lstat to avoid following symlinks */
    if (lstat(full_path, &st) < 0) {
        return GM_OK; /* File disappeared or no access - skip silently */
    }

    if (S_ISDIR(st.st_mode)) {
        /* Build relative path for subdirectory */
        if (rel_path) {
            int rn = gm_snprintf(entry_rel_path, sizeof(entry_rel_path), "%s/%s",
                                 rel_path, entry_name);
            if (rn < 0 || (size_t)rn >= sizeof(entry_rel_path)) {
                return GM_ERR_BUFFER_TOO_SMALL;
            }
        } else {
            int rn2 = gm_snprintf(entry_rel_path, sizeof(entry_rel_path), "%s",
                                  entry_name);
            if (rn2 < 0 || (size_t)rn2 >= sizeof(entry_rel_path)) {
                return GM_ERR_BUFFER_TOO_SMALL;
            }
        }
        rc =
            add_directory_to_tree(repo, dir_builder, full_path, entry_rel_path);
        return (rc == GM_NOT_FOUND) ? GM_OK : rc;
    }

    if (S_ISREG(st.st_mode)) {
        rc = add_file_to_tree(repo, dir_builder, full_path, entry_name);
        return (rc == GM_NOT_FOUND) ? GM_OK : rc;
    }

    /* Skip other file types (symlinks, devices, etc.) */
    return GM_OK;
}

/* Add directory tree to parent */
static int add_tree_to_parent(git_treebuilder *dir_builder,
                              git_treebuilder *parent_builder,
                              const char *dir_path) {
    git_oid tree_oid;
    int rc;

    /* Write this directory's tree */
    rc = git_treebuilder_write(&tree_oid, dir_builder);
    if (rc < 0) {
        return GM_ERR_UNKNOWN;
    }

    if (parent_builder) {
        /* Add this tree to parent */
        const char *dirname = strrchr(dir_path, '/');
        dirname = dirname ? dirname + 1 : dir_path;
        rc = git_treebuilder_insert(NULL, parent_builder, dirname, &tree_oid,
                                    GIT_FILEMODE_TREE);
        if (rc < 0) {
            return GM_ERR_UNKNOWN;
        }
    }

    return GM_OK;
}

/* Recursively add directory contents to tree */
static int add_directory_to_tree(git_repository *repo,
                                 git_treebuilder *parent_builder,
                                 const char *dir_path, const char *rel_path) {
    DIR *dir;
    struct dirent *entry;
    git_treebuilder *dir_builder = NULL;
    int rc = GM_OK;

    dir = opendir(dir_path);
    if (!dir) {
        return GM_IO_ERROR;
    }

    /* Create tree builder for this directory */
    rc = git_treebuilder_new(&dir_builder, repo, NULL);
    if (rc < 0) {
        closedir(dir);
        return GM_ERR_UNKNOWN;
    }

    /* Process directory entries */
    while ((entry = readdir(dir)) != NULL && rc == GM_OK) {
        rc = process_fs_entry(repo, dir_builder, dir_path, entry->d_name,
                              rel_path);
    }

    closedir(dir);

    /* Add tree to parent if successful */
    if (rc == GM_OK) {
        rc = add_tree_to_parent(dir_builder, parent_builder, dir_path);
    }

    git_treebuilder_free(dir_builder);
    return rc;
}

/* Process a single directory entry */
static int process_directory_entry(git_repository *repo,
                                   git_treebuilder *parent_builder,
                                   const char *dir_path,
                                   const char *entry_name) {
    char full_path[GM_PATH_MAX];
    struct stat st;
    int rc;

    /* Skip special entries */
    if (strcmp(entry_name, ".") == 0 || strcmp(entry_name, "..") == 0) {
        return GM_OK;
    }

    /* Build full path */
    {
        int rn = gm_snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry_name);
        if (rn < 0 || (size_t)rn >= sizeof(full_path)) {
            return GM_ERR_BUFFER_TOO_SMALL;
        }
    }

    /* Check if path exists and is directory */
    if (lstat(full_path, &st) < 0) {
        return GM_OK; /* File disappeared - skip silently */
    }

    if (!S_ISDIR(st.st_mode)) {
        return GM_OK; /* Skip non-directories for cache tree */
    }

    /* Build subtree */
    git_oid subtree_oid;
    git_treebuilder *sub_builder = NULL;

    rc = git_treebuilder_new(&sub_builder, repo, NULL);
    if (rc < 0) {
        return GM_ERR_UNKNOWN;
    }

    rc = add_directory_to_tree(repo, sub_builder, full_path, NULL);
    if (rc == GM_OK) {
        rc = git_treebuilder_write(&subtree_oid, sub_builder);
        if (rc == 0) {
            rc = git_treebuilder_insert(NULL, parent_builder, entry_name,
                                        &subtree_oid, GIT_FILEMODE_TREE);
        }
    } else if (rc == GM_NOT_FOUND) {
        rc = GM_OK; /* Directory disappeared - not fatal */
    }

    git_treebuilder_free(sub_builder);
    return rc < 0 ? GM_ERR_UNKNOWN : GM_OK;
}

/* Build directory tree */
static int build_directory_tree(git_repository *repo, git_treebuilder *builder,
                                const char *dir_path) {
    DIR *dir = opendir(dir_path);
    if (!dir) {
        return GM_IO_ERROR;
    }

    struct dirent *entry;
    int rc = GM_OK;

    while ((entry = readdir(dir)) != NULL && rc == GM_OK) {
        rc = process_directory_entry(repo, builder, dir_path, entry->d_name);
    }

    closedir(dir);
    return rc;
}

/* Build Git tree from temp directory */
int gm_build_tree_from_directory(git_repository *repo, const char *dir_path,
                                 git_oid *tree_oid) {
    git_treebuilder *root_builder = NULL;
    int rc;

    /* Create root tree builder */
    rc = git_treebuilder_new(&root_builder, repo, NULL);
    if (rc < 0) {
        return GM_ERR_UNKNOWN;
    }

    /* Build directory tree */
    rc = build_directory_tree(repo, root_builder, dir_path);

    /* Write root tree if successful */
    if (rc == GM_OK) {
        rc = git_treebuilder_write(tree_oid, root_builder);
        if (rc < 0) {
            rc = GM_ERR_UNKNOWN;
        }
    }

    git_treebuilder_free(root_builder);
    return rc;
}
