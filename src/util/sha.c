/* SPDX-License-Identifier: Apache-2.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind.h"
#include <git2.h>
#include <string.h>

/* Get blob SHA from file path */
int gm_sha_from_path(gm_context_t *ctx, const char *path, uint8_t *sha) {
    git_repository *repo;
    git_index *index = NULL;
    const git_index_entry *entry;
    int error;
    
    if (!ctx || !path || !sha) {
        return GM_INVALID_ARG;
    }
    
    repo = (git_repository *)ctx->git_repo;
    
    /* Get repository index */
    error = git_repository_index(&index, repo);
    if (error < 0) {
        return GM_ERROR;
    }
    
    /* Look up path in index */
    entry = git_index_get_bypath(index, path, 0);
    if (!entry) {
        git_index_free(index);
        return GM_NOT_FOUND;
    }
    
    /* Copy SHA */
    memcpy(sha, entry->id.id, GM_SHA1_SIZE);
    
    git_index_free(index);
    return GM_OK;
}