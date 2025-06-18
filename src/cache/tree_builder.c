/* SPDX-License-Identifier: Apache-2.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#define _POSIX_C_SOURCE 200809L

#include <git2.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include "../../include/gitmind.h"

/* Forward declaration */
static int add_directory_to_tree(git_repository* repo, git_treebuilder* parent_builder,
                                const char* dir_path, const char* rel_path);

/* Add a file to the tree builder */
static int add_file_to_tree(git_repository* repo, git_treebuilder* builder,
                           const char* file_path, const char* name) {
    git_oid blob_oid;
    int rc;
    
    /* Create blob from file */
    rc = git_blob_create_from_workdir(&blob_oid, repo, file_path);
    if (rc < 0) {
        return GM_ERROR;
    }
    
    /* Add to tree */
    rc = git_treebuilder_insert(NULL, builder, name, &blob_oid, GIT_FILEMODE_BLOB);
    return rc < 0 ? GM_ERROR : GM_OK;
}

/* Recursively add directory contents to tree */
static int add_directory_to_tree(git_repository* repo, git_treebuilder* parent_builder,
                                const char* dir_path, const char* rel_path) {
    DIR* dir;
    struct dirent* entry;
    char full_path[GM_PATH_MAX * 2];  /* Extra space for concatenation */
    char entry_name[GM_PATH_MAX * 2];  /* Extra space for path building */
    struct stat st;
    int rc = GM_OK;
    
    dir = opendir(dir_path);
    if (!dir) {
        return GM_IO_ERROR;
    }
    
    /* Create tree builder for this directory */
    git_treebuilder* dir_builder = NULL;
    rc = git_treebuilder_new(&dir_builder, repo, NULL);
    if (rc < 0) {
        closedir(dir);
        return GM_ERROR;
    }
    
    /* Process directory entries */
    while ((entry = readdir(dir)) != NULL) {
        /* Skip . and .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        /* Build full path */
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        
        /* Get file info */
        if (stat(full_path, &st) < 0) {
            continue;
        }
        
        if (S_ISDIR(st.st_mode)) {
            /* Subdirectory - recurse */
            if (rel_path) {
                snprintf(entry_name, sizeof(entry_name), "%s/%s", rel_path, entry->d_name);
            } else {
                strncpy(entry_name, entry->d_name, sizeof(entry_name) - 1);
                entry_name[sizeof(entry_name) - 1] = '\0';
            }
            rc = add_directory_to_tree(repo, dir_builder, full_path, entry_name);
            if (rc != GM_OK) {
                break;
            }
        } else if (S_ISREG(st.st_mode)) {
            /* Regular file - add as blob */
            rc = add_file_to_tree(repo, dir_builder, full_path, entry->d_name);
            if (rc != GM_OK) {
                break;
            }
        }
    }
    
    closedir(dir);
    
    /* Write this directory's tree */
    if (rc == GM_OK) {
        git_oid tree_oid;
        rc = git_treebuilder_write(&tree_oid, dir_builder);
        if (rc == 0 && parent_builder) {
            /* Add this tree to parent */
            const char* dirname = strrchr(dir_path, '/');
            dirname = dirname ? dirname + 1 : dir_path;
            rc = git_treebuilder_insert(NULL, parent_builder, dirname,
                                       &tree_oid, GIT_FILEMODE_TREE);
        }
    }
    
    git_treebuilder_free(dir_builder);
    return rc < 0 ? GM_ERROR : rc;
}

/* Build Git tree from temp directory */
int gm_build_tree_from_directory(git_repository* repo, const char* dir_path,
                                git_oid* tree_oid) {
    git_treebuilder* root_builder = NULL;
    int rc;
    
    /* Create root tree builder */
    rc = git_treebuilder_new(&root_builder, repo, NULL);
    if (rc < 0) {
        return GM_ERROR;
    }
    
    /* Add directory contents */
    rc = add_directory_to_tree(repo, NULL, dir_path, NULL);
    if (rc != GM_OK) {
        git_treebuilder_free(root_builder);
        return rc;
    }
    
    /* Get the final tree OID */
    /* Note: Since we built the tree in the recursive call, we need to
       rebuild from scratch to get the proper tree */
    git_treebuilder_free(root_builder);
    
    /* Simpler approach: create tree directly from directory */
    rc = git_treebuilder_new(&root_builder, repo, NULL);
    if (rc < 0) return GM_ERROR;
    
    /* Process top-level entries */
    DIR* dir = opendir(dir_path);
    if (!dir) {
        git_treebuilder_free(root_builder);
        return GM_IO_ERROR;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        char full_path[GM_PATH_MAX * 2];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        
        struct stat st;
        if (stat(full_path, &st) < 0) continue;
        
        if (S_ISDIR(st.st_mode)) {
            /* Build subtree */
            git_oid subtree_oid;
            git_treebuilder* sub_builder = NULL;
            
            rc = git_treebuilder_new(&sub_builder, repo, NULL);
            if (rc < 0) break;
            
            rc = add_directory_to_tree(repo, sub_builder, full_path, NULL);
            if (rc == GM_OK) {
                rc = git_treebuilder_write(&subtree_oid, sub_builder);
                if (rc == 0) {
                    rc = git_treebuilder_insert(NULL, root_builder, entry->d_name,
                                               &subtree_oid, GIT_FILEMODE_TREE);
                }
            }
            git_treebuilder_free(sub_builder);
            if (rc < 0) break;
        }
    }
    
    closedir(dir);
    
    /* Write root tree */
    if (rc >= 0) {
        rc = git_treebuilder_write(tree_oid, root_builder);
    }
    
    git_treebuilder_free(root_builder);
    return rc < 0 ? GM_ERROR : GM_OK;
}