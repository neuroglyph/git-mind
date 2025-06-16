/* SPDX-License-Identifier: Apache-2.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#define _POSIX_C_SOURCE 200112L
#include "gitmind.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Merge a new entry into an existing tree at a specific path
int gm_merge_tree_path(const char* base_tree, const char* path, 
                      const char* entry_mode, const char* entry_sha,
                      char* out_tree) {
    if (!path || !entry_mode || !entry_sha || !out_tree) {
        gm_set_error("Invalid arguments");
        return GM_ERR_INVALID_ARG;
    }
    
    // Split path into first component and rest
    char path_copy[GM_MAX_PATH];
    strncpy(path_copy, path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';
    
    char* slash = strchr(path_copy, '/');
    char* rest_path = NULL;
    if (slash) {
        *slash = '\0';
        rest_path = slash + 1;
    }
    
    char cmd[GM_MAX_COMMAND];
    
    if (rest_path) {
        // Not at leaf - need to recurse
        char subtree_sha[GM_SHA1_STRING_SIZE] = {0};
        
        // Get existing subtree if it exists
        if (base_tree && strlen(base_tree) > 0) {
            snprintf(cmd, sizeof(cmd),
                "git ls-tree %s %s 2>/dev/null | awk '{print $3}'",
                base_tree, path_copy);
            
            FILE* fp = popen(cmd, "r");
            if (fp) {
                if (fgets(subtree_sha, sizeof(subtree_sha), fp) != NULL) {
                    char* nl = strchr(subtree_sha, '\n');
                    if (nl) *nl = '\0';
                }
                pclose(fp);
            }
        }
        
        // Recursively merge into subtree
        char new_subtree[GM_SHA1_STRING_SIZE];
        int ret = gm_merge_tree_path(subtree_sha, rest_path, 
                                    entry_mode, entry_sha, new_subtree);
        if (ret != GM_OK) return ret;
        
        // Now merge this subtree into current level (tree mode)
        return gm_merge_tree_path(base_tree, path_copy, GM_GIT_MODE_TREE, new_subtree, out_tree);
    } else {
        // At leaf level - merge entry
        // Determine object type from mode
        const char* obj_type = (strcmp(entry_mode, GM_GIT_MODE_TREE) == 0) ? 
                              GM_GIT_TYPE_TREE : GM_GIT_TYPE_BLOB;
        
        if (base_tree && strlen(base_tree) > 0) {
            // Merge with existing tree
            snprintf(cmd, sizeof(cmd),
                "(git ls-tree %s 2>/dev/null | grep -v '\t%s$'; "
                "echo '%s %s %s\t%s') | sort -k4 | git mktree",
                base_tree, path_copy, entry_mode, obj_type, entry_sha, path_copy);
        } else {
            // Create new tree with single entry
            snprintf(cmd, sizeof(cmd),
                "echo '%s %s %s\t%s' | git mktree",
                entry_mode, obj_type, entry_sha, path_copy);
        }
        
        FILE* fp = popen(cmd, "r");
        if (!fp) {
            gm_set_error("Failed to create tree");
            return GM_ERR_GIT;
        }
        
        if (!fgets(out_tree, GM_SHA1_STRING_SIZE, fp)) {
            pclose(fp);
            gm_set_error("Failed to read tree SHA");
            return GM_ERR_GIT;
        }
        pclose(fp);
        
        char* nl = strchr(out_tree, '\n');
        if (nl) *nl = '\0';
    }
    
    return GM_OK;
}
