/* SPDX-License-Identifier: Apache-2.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#define _POSIX_C_SOURCE 200112L
#include "gitmind.h"
#include <git2.h>
#include <stdio.h>
#include <string.h>

// Path mapping using Git notes
// Maps SHA -> path for display purposes

// Store a SHA->path mapping
int gm_store_path_mapping(const char* sha, const char* path) {
    if (!sha || !path) {
        gm_set_error("Invalid arguments");
        return GM_ERR_INVALID_ARG;
    }
    
    // Use git notes to store the mapping
    char cmd[GM_MAX_COMMAND];
    snprintf(cmd, sizeof(cmd), GM_GIT_NOTES_ADD_CMD, path, sha);
    
    FILE* fp = popen(cmd, "r");
    if (!fp) {
        gm_set_error("Failed to store path mapping");
        return GM_ERR_GIT;
    }
    
    int status = pclose(fp);
    if (status != 0) {
        // Non-zero status is ok - might mean the note already exists
        return GM_OK;
    }
    
    return GM_OK;
}

// Store a path mapping using libgit2
int gm_path_mapping_store_git2(git_repository *repo, const git_oid *oid, const char *path) {
    git_signature *sig = NULL;
    int ret = GM_OK;
    
    if (!repo || !oid || !path) {
        gm_set_error("Invalid arguments");
        return GM_ERR_INVALID_ARG;
    }
    
    // Get signature
    ret = gm_git_backend_signature_default(repo, &sig);
    if (ret != GM_OK) {
        return ret;
    }
    
    // Create or update note
    git_oid note_oid;
    if (git_note_create(&note_oid, repo, GM_NOTES_PATH_REF, sig, sig, oid, path, 1) < 0) {
        const git_error *e = git_error_last();
        gm_set_error("Failed to create note: %s", e ? e->message : "unknown error");
        ret = GM_ERR_GIT;
    }
    
    git_signature_free(sig);
    return ret;
}

// Retrieve path for a SHA
int gm_get_path_for_sha(const char* sha, char* out_path, size_t path_size) {
    if (!sha || !out_path) {
        gm_set_error("Invalid arguments");
        return GM_ERR_INVALID_ARG;
    }
    
    // Try to get path from git notes
    char cmd[GM_MAX_COMMAND];
    snprintf(cmd, sizeof(cmd), GM_GIT_NOTES_SHOW_CMD, sha);
    
    FILE* fp = popen(cmd, "r");
    if (!fp) {
        // No mapping found - return SHA as fallback
        strncpy(out_path, sha, path_size - 1);
        out_path[path_size - 1] = '\0';
        return GM_OK;
    }
    
    if (!fgets(out_path, path_size, fp)) {
        pclose(fp);
        // No mapping found - return SHA as fallback
        strncpy(out_path, sha, path_size - 1);
        out_path[path_size - 1] = '\0';
        return GM_OK;
    }
    
    pclose(fp);
    
    // Remove newline
    char* nl = strchr(out_path, '\n');
    if (nl) *nl = '\0';
    
    return GM_OK;
}

// Get path for SHA from notes using libgit2
int gm_path_mapping_get_git2(git_repository *repo, const git_oid *oid, char *out_path, size_t path_size) {
    git_note *note = NULL;
    char sha_str[GIT_OID_HEXSZ + 1];
    
    if (!repo || !oid || !out_path || path_size == 0) {
        gm_set_error("Invalid arguments");
        return GM_ERR_INVALID_ARG;
    }
    
    // Try to read note
    if (git_note_read(&note, repo, GM_NOTES_PATH_REF, oid) < 0) {
        // No note found, return SHA as fallback
        git_oid_tostr(sha_str, sizeof(sha_str), oid);
        strncpy(out_path, sha_str, path_size - 1);
        out_path[path_size - 1] = '\0';
        return GM_OK;
    }
    
    const char *msg = git_note_message(note);
    if (msg) {
        strncpy(out_path, msg, path_size - 1);
        out_path[path_size - 1] = '\0';
        // Remove newline if present
        char *nl = strchr(out_path, '\n');
        if (nl) *nl = '\0';
    } else {
        git_oid_tostr(sha_str, sizeof(sha_str), oid);
        strncpy(out_path, sha_str, path_size - 1);
        out_path[path_size - 1] = '\0';
    }
    
    git_note_free(note);
    return GM_OK;
}
