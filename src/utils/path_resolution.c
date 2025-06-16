/* SPDX-License-Identifier: Apache-2.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#define _POSIX_C_SOURCE 200112L
#include "gitmind_lib.h"
#include "gitmind_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward declaration for recursive tree search */
static int search_tree_for_blob(gm_context_t* ctx, const char* tree_sha,
                                const char* target_blob_sha, 
                                const char* path_prefix,
                                char* out_path, size_t path_size);

/* Find file path by blob SHA by searching HEAD tree */
int gm_find_path_by_sha(gm_context_t* ctx, const char* blob_sha, 
                       char* out_path, size_t path_size) {
    if (!ctx || !blob_sha || !out_path || path_size == 0) {
        if (ctx) gm_set_error_ctx(ctx, GM_ERR_INVALID_ARG, "Invalid arguments");
        return GM_ERR_INVALID_ARG;
    }
    
    /* Get HEAD tree SHA */
    char head_tree[GM_SHA1_STRING_SIZE];
    int ret = ctx->backend->read_ref(ctx, "HEAD", head_tree);
    if (ret != GM_OK) {
        gm_set_error_ctx(ctx, ret, "Failed to read HEAD");
        return ret;
    }
    
    /* Get commit tree */
    char commit_data[8192];
    size_t commit_size = 0;
    ret = ctx->backend->read_object(ctx, head_tree, 
                                   (unsigned char*)commit_data, 
                                   sizeof(commit_data), &commit_size);
    if (ret != GM_OK) {
        gm_set_error_ctx(ctx, ret, "Failed to read HEAD commit");
        return ret;
    }
    
    /* Parse commit to find tree line */
    char* tree_line = strstr(commit_data, "tree ");
    if (!tree_line) {
        gm_set_error_ctx(ctx, GM_ERR_GIT, "Invalid commit format");
        return GM_ERR_GIT;
    }
    
    char tree_sha[GM_SHA1_STRING_SIZE];
    if (sscanf(tree_line, "tree %40s", tree_sha) != 1) {
        gm_set_error_ctx(ctx, GM_ERR_GIT, "Failed to parse tree SHA");
        return GM_ERR_GIT;
    }
    
    /* Search the tree recursively */
    ret = search_tree_for_blob(ctx, tree_sha, blob_sha, "", out_path, path_size);
    if (ret != GM_OK) {
        gm_set_error_ctx(ctx, GM_ERR_NOT_FOUND, 
                        "Blob %s not found in HEAD tree", blob_sha);
        return GM_ERR_NOT_FOUND;
    }
    
    return GM_OK;
}

/* Recursively search a tree for a blob with given SHA */
static int search_tree_for_blob(gm_context_t* ctx, const char* tree_sha,
                                const char* target_blob_sha, 
                                const char* path_prefix,
                                char* out_path, size_t path_size) {
    char tree_entries[32768];
    int ret = ctx->backend->read_tree(ctx, tree_sha, tree_entries);
    if (ret != GM_OK) {
        return ret;
    }
    
    /* Parse tree entries */
    char* line = tree_entries;
    while (*line) {
        /* Parse entry format: "mode type sha\tname\n" */
        char mode[7], type[10], sha[41], name[256];
        char* tab = strchr(line, '\t');
        if (!tab) break;
        
        char* newline = strchr(tab, '\n');
        if (!newline) break;
        
        *tab = '\0';
        *newline = '\0';
        
        if (sscanf(line, "%6s %9s %40s", mode, type, sha) == 3) {
            strcpy(name, tab + 1);
            
            /* Build full path */
            char full_path[4096];
            if (strlen(path_prefix) > 0) {
                snprintf(full_path, sizeof(full_path), "%s/%s", path_prefix, name);
            } else {
                strcpy(full_path, name);
            }
            
            if (strcmp(type, "blob") == 0 && strcmp(sha, target_blob_sha) == 0) {
                /* Found it! */
                strncpy(out_path, full_path, path_size - 1);
                out_path[path_size - 1] = '\0';
                return GM_OK;
            } else if (strcmp(type, "tree") == 0) {
                /* Recurse into subtree */
                ret = search_tree_for_blob(ctx, sha, target_blob_sha, 
                                         full_path, out_path, path_size);
                if (ret == GM_OK) {
                    return GM_OK;  /* Found in subtree */
                }
            }
        }
        
        /* Move to next line */
        line = newline + 1;
    }
    
    return GM_ERR_NOT_FOUND;
}