// SPDX-License-Identifier: Apache-2.0
// © 2025 J. Kirby Ross / Neuroglyph Collective

#define _POSIX_C_SOURCE 200112L
#include "gitmind.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GM_GRAPH_REF "refs/gitmind/graph"
#define GM_INBOUND_NOTES_REF "refs/notes/gitmind/inbound"

// Check if orphan ref exists
int gm_orphan_ref_exists(void) {
    char cmd[GM_MAX_COMMAND];
    snprintf(cmd, sizeof(cmd), "git rev-parse --verify %s >/dev/null 2>&1", GM_GRAPH_REF);
    return system(cmd) == 0 ? GM_OK : GM_ERR_NOT_FOUND;
}

// Create empty orphan ref
int gm_orphan_ref_create(void) {
    // Check if already exists
    if (gm_orphan_ref_exists() == GM_OK) {
        return GM_OK;  // Already exists, idempotent
    }
    
    // Create empty tree
    FILE* fp = popen("git hash-object -t tree -w /dev/null", "r");
    if (!fp) {
        gm_set_error("Failed to create empty tree");
        return GM_ERR_GIT;
    }
    
    char empty_tree[41];
    if (!fgets(empty_tree, sizeof(empty_tree), fp)) {
        pclose(fp);
        gm_set_error("Failed to get empty tree SHA");
        return GM_ERR_GIT;
    }
    pclose(fp);
    
    // Remove newline
    char* nl = strchr(empty_tree, '\n');
    if (nl) *nl = '\0';
    
    // Create orphan commit with proper author
    char cmd[GM_MAX_COMMAND];
    snprintf(cmd, sizeof(cmd), 
        "GIT_AUTHOR_NAME='git-mind' GIT_AUTHOR_EMAIL='git-mind@localhost' "
        "GIT_COMMITTER_NAME='git-mind' GIT_COMMITTER_EMAIL='git-mind@localhost' "
        "git commit-tree %s -m 'Initialize git-mind graph' | xargs git update-ref %s",
        empty_tree, GM_GRAPH_REF);
    
    if (system(cmd) != 0) {
        gm_set_error("Failed to create orphan ref");
        return GM_ERR_GIT;
    }
    
    return GM_OK;
}

// Get current graph tree SHA
int gm_get_graph_tree(char* out_tree_sha) {
    if (!out_tree_sha) {
        gm_set_error("Invalid argument");
        return GM_ERR_INVALID_ARG;
    }
    
    char cmd[GM_MAX_COMMAND];
    snprintf(cmd, sizeof(cmd), "git rev-parse %s^{tree} 2>/dev/null", GM_GRAPH_REF);
    
    FILE* fp = popen(cmd, "r");
    if (!fp) {
        gm_set_error("Failed to get graph tree");
        return GM_ERR_GIT;
    }
    
    if (!fgets(out_tree_sha, 41, fp)) {
        pclose(fp);
        gm_set_error("Graph ref not found");
        return GM_ERR_NOT_FOUND;
    }
    pclose(fp);
    
    // Remove newline
    char* nl = strchr(out_tree_sha, '\n');
    if (nl) *nl = '\0';
    
    return GM_OK;
}

// Update graph ref with new tree
int gm_update_graph_ref(const char* new_tree_sha, const char* message) {
    if (!new_tree_sha || !message) {
        gm_set_error("Invalid arguments");
        return GM_ERR_INVALID_ARG;
    }
    
    // Get current commit (parent)
    char parent_commit[41] = "";
    char parent_cmd[GM_MAX_COMMAND];
    snprintf(parent_cmd, sizeof(parent_cmd), "git rev-parse %s 2>/dev/null", GM_GRAPH_REF);
    
    FILE* fp = popen(parent_cmd, "r");
    if (fp) {
        if (fgets(parent_commit, sizeof(parent_commit), fp)) {
            char* nl = strchr(parent_commit, '\n');
            if (nl) *nl = '\0';
        }
        pclose(fp);
    }
    
    // Create new commit with proper author
    char cmd[GM_MAX_COMMAND];
    const char* git_env = "GIT_AUTHOR_NAME='git-mind' GIT_AUTHOR_EMAIL='git-mind@localhost' "
                         "GIT_COMMITTER_NAME='git-mind' GIT_COMMITTER_EMAIL='git-mind@localhost' ";
    
    if (parent_commit[0]) {
        // With parent
        snprintf(cmd, sizeof(cmd), 
            "%secho '%s' | git commit-tree %s -p %s | xargs git update-ref %s",
            git_env, message, new_tree_sha, parent_commit, GM_GRAPH_REF);
    } else {
        // No parent (shouldn't happen after init, but handle it)
        snprintf(cmd, sizeof(cmd), 
            "%secho '%s' | git commit-tree %s | xargs git update-ref %s",
            git_env, message, new_tree_sha, GM_GRAPH_REF);
    }
    
    if (system(cmd) != 0) {
        gm_set_error("Failed to update graph ref");
        return GM_ERR_GIT;
    }
    
    return GM_OK;
}
