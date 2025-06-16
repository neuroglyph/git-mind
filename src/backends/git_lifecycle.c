/* SPDX-License-Identifier: Apache-2.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#define _POSIX_C_SOURCE 200112L
#include "gitmind.h"
#include <git2.h>
#include <stdio.h>
#include <stdlib.h>

// Track libgit2 initialization state
static int git2_init_count = 0;

// Initialize libgit2 on first use
int gm_git_backend_init(void) {
    if (git2_init_count == 0) {
        int ret = git_libgit2_init();
        if (ret < 0) {
            const git_error *e = git_error_last();
            gm_set_error("Failed to initialize libgit2: %s", e ? e->message : "unknown error");
            return GM_ERR_GIT;
        }
        git2_init_count = ret;
    }
    return GM_OK;
}

// Cleanup libgit2
void gm_git_backend_cleanup(void) {
    if (git2_init_count > 0) {
        git_libgit2_shutdown();
        git2_init_count = 0;
    }
}

// Open repository
int gm_git_backend_open_repo(git_repository **repo) {
    if (!repo) {
        gm_set_error("Invalid argument");
        return GM_ERR_INVALID_ARG;
    }
    
    if (git_repository_open(repo, ".") < 0) {
        const git_error *e = git_error_last();
        gm_set_error("Failed to open repository: %s", e ? e->message : "unknown error");
        return GM_ERR_NOT_REPO;
    }
    
    return GM_OK;
}

// Create default signature
int gm_git_backend_signature_default(git_repository *repo, git_signature **sig) {
    if (!repo || !sig) {
        gm_set_error("Invalid arguments");
        return GM_ERR_INVALID_ARG;
    }
    
    if (git_signature_default(sig, repo) < 0) {
        // If no default signature, create a generic one
        if (git_signature_now(sig, "git-mind", "git-mind@localhost") < 0) {
            const git_error *e = git_error_last();
            gm_set_error("Failed to create signature: %s", e ? e->message : "unknown error");
            return GM_ERR_GIT;
        }
    }
    
    return GM_OK;
}
