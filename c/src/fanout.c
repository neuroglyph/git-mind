// SPDX-License-Identifier: Apache-2.0
// © 2025 J. Kirby Ross / Neuroglyph Collective

#define _POSIX_C_SOURCE 200112L
#include "gitmind.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Compute fan-out path for a SHA
void gm_compute_fanout(const char* sha, char* out_fanout, size_t fanout_size) {
    // Take first 2 bytes (4 hex chars) for fan-out
    snprintf(out_fanout, fanout_size, "%.2s/%.2s", sha, sha + 2);
}

// Compute relationship type hash (8 hex chars of SHA1)
int gm_compute_rel_hash(const char* rel_type, char* out_hash) {
    if (!rel_type || !out_hash) {
        gm_set_error("Invalid arguments");
        return GM_ERR_INVALID_ARG;
    }
    
    // Normalize to lowercase
    char normalized[GM_MAX_TYPE];
    size_t i;
    for (i = 0; rel_type[i] && i < sizeof(normalized) - 1; i++) {
        normalized[i] = tolower(rel_type[i]);
    }
    normalized[i] = '\0';
    
    // Compute SHA1 using internal implementation
    char full_hash[GM_SHA1_STRING_SIZE];
    int ret = gm_sha1_string(normalized, full_hash);
    if (ret != GM_OK) {
        gm_set_error("Failed to compute relationship hash");
        return ret;
    }
    
    // Take first 8 chars
    strncpy(out_hash, full_hash, 8);
    out_hash[8] = '\0';
    
    return GM_OK;
}

// Build full edge path
int gm_build_edge_path(const char* src_sha, const char* rel_type, 
                      const char* edge_id, char* out_path, size_t path_size) {
    if (!src_sha || !rel_type || !edge_id || !out_path) {
        gm_set_error("Invalid arguments");
        return GM_ERR_INVALID_ARG;
    }
    
    // Compute components
    char src_fanout[6];  // "ab/cd\0"
    gm_compute_fanout(src_sha, src_fanout, sizeof(src_fanout));
    
    char rel_hash[9];
    int ret = gm_compute_rel_hash(rel_type, rel_hash);
    if (ret != GM_OK) return ret;
    
    char edge_fanout[6];
    gm_compute_fanout(edge_id, edge_fanout, sizeof(edge_fanout));
    
    // Build path: ab/cd/<src-sha>/<rel-hash>/ef/gh/<edge-id>
    snprintf(out_path, path_size, "%s/%s/%s/%s/%s",
             src_fanout, src_sha, rel_hash, edge_fanout, edge_id);
    
    return GM_OK;
}

// Update tree with new blob
int gm_update_tree_with_blob(const char* tree_sha, const char* path,
                           const char* blob_sha, char* out_new_tree) {
    if (!path || !blob_sha || !out_new_tree) {
        gm_set_error("Invalid arguments");
        return GM_ERR_INVALID_ARG;
    }
    
    // Use mktree to update
    char cmd[GM_MAX_COMMAND];
    
    if (tree_sha && strlen(tree_sha) > 0) {
        // Update existing tree
        snprintf(cmd, sizeof(cmd),
            "(git ls-tree %s 2>/dev/null; echo '100644 blob %s\t%s') | "
            "sort -k4 | uniq -f3 | git mktree",
            tree_sha, blob_sha, path);
    } else {
        // Create new tree with single entry
        snprintf(cmd, sizeof(cmd),
            "echo '100644 blob %s\t%s' | git mktree",
            blob_sha, path);
    }
    
    FILE* fp = popen(cmd, "r");
    if (!fp) {
        gm_set_error("Failed to update tree");
        return GM_ERR_GIT;
    }
    
    if (!fgets(out_new_tree, 41, fp)) {
        pclose(fp);
        gm_set_error("Failed to create tree");
        return GM_ERR_GIT;
    }
    pclose(fp);
    
    // Remove newline
    char* nl = strchr(out_new_tree, '\n');
    if (nl) *nl = '\0';
    
    return GM_OK;
}

// Recursively build tree structure for edge
int gm_build_edge_tree(const char* edge_path, const char* edge_blob_sha,
                      const char* current_tree_sha, char* out_tree_sha) {
    if (!edge_path || !edge_blob_sha || !out_tree_sha) {
        gm_set_error("Invalid arguments");
        return GM_ERR_INVALID_ARG;
    }
    
    // TODO: Merge with current_tree_sha when implementing updates
    (void)current_tree_sha;  // Suppress unused warning
    
    // Split path into components
    char path_copy[GM_MAX_PATH];
    strncpy(path_copy, edge_path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';
    
    // Count path components
    int depth = 0;
    for (const char* p = path_copy; *p; p++) {
        if (*p == '/') depth++;
    }
    
    // Build from bottom up
    char* components[16];  // Max depth
    int comp_count = 0;
    
    char* token = strtok(path_copy, "/");
    while (token && comp_count < 16) {
        components[comp_count++] = token;
        token = strtok(NULL, "/");
    }
    
    // Start with the blob at the deepest level
    char current_sha[41];
    strncpy(current_sha, edge_blob_sha, sizeof(current_sha));
    
    // Build trees from deepest to root
    for (int i = comp_count - 1; i >= 0; i--) {
        char new_tree[41];
        
        // Create tree with current item
        if (i == comp_count - 1) {
            // Leaf level - add blob
            int ret = gm_update_tree_with_blob("", components[i], current_sha, new_tree);
            if (ret != GM_OK) return ret;
        } else {
            // Directory level - add tree
            char cmd[GM_MAX_COMMAND];
            snprintf(cmd, sizeof(cmd),
                "echo '040000 tree %s\t%s' | git mktree",
                current_sha, components[i]);
            
            FILE* fp = popen(cmd, "r");
            if (!fp) {
                gm_set_error("Failed to create tree");
                return GM_ERR_GIT;
            }
            
            if (!fgets(new_tree, sizeof(new_tree), fp)) {
                pclose(fp);
                gm_set_error("Failed to read tree SHA");
                return GM_ERR_GIT;
            }
            pclose(fp);
            
            char* nl = strchr(new_tree, '\n');
            if (nl) *nl = '\0';
        }
        
        strncpy(current_sha, new_tree, sizeof(current_sha));
    }
    
    strncpy(out_tree_sha, current_sha, 41);
    return GM_OK;
}