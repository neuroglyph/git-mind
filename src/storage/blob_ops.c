/* SPDX-License-Identifier: Apache-2.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#define _POSIX_C_SOURCE 200112L
#include "gitmind.h"
#include <git2.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// Get or create a blob from file or path content
int gm_blob_get_or_create(git_repository *repo, const char *path, git_oid *out_oid) {
    if (!repo || !path || !out_oid) {
        gm_set_error("Invalid arguments");
        return GM_ERR_INVALID_ARG;
    }
    
    // Check if file exists
    if (access(path, F_OK) == 0) {
        // File exists, create blob from file
        if (git_blob_create_from_workdir(out_oid, repo, path) < 0) {
            const git_error *e = git_error_last();
            gm_set_error("Failed to create blob from file: %s", e ? e->message : "unknown error");
            return GM_ERR_GIT;
        }
    } else {
        // File doesn't exist, create blob from path string
        if (git_blob_create_from_buffer(out_oid, repo, path, strlen(path)) < 0) {
            const git_error *e = git_error_last();
            gm_set_error("Failed to create blob from path: %s", e ? e->message : "unknown error");
            return GM_ERR_GIT;
        }
    }
    return GM_OK;
}

// Create blob from buffer
int gm_blob_create_from_buffer(git_repository *repo, const void *buffer, size_t len, git_oid *out_oid) {
    if (!repo || !buffer || !out_oid) {
        gm_set_error("Invalid arguments");
        return GM_ERR_INVALID_ARG;
    }
    
    if (git_blob_create_from_buffer(out_oid, repo, buffer, len) < 0) {
        const git_error *e = git_error_last();
        gm_set_error("Failed to create blob: %s", e ? e->message : "unknown error");
        return GM_ERR_GIT;
    }
    
    return GM_OK;
}
