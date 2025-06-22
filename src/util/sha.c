/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind.h"
#include "gitmind/constants_internal.h"
#include "gm_mem.h"

#include <git2.h>
#include <string.h>

/* Git index constants */
#define GIT_INDEX_STAGE_DEFAULT 0

/* Get git index (DI-friendly through context) */
static int get_git_index(gm_context_t *ctx, git_index **index) {
    git_repository *repo = (git_repository *)ctx->git_repo;
    return git_repository_index(index, repo);
}

/* Look up path in index (Single Responsibility) */
static const git_index_entry *find_path_entry(git_index *index,
                                               const char *path) {
    return git_index_get_bypath(index, path, GIT_INDEX_STAGE_DEFAULT);
}

/* Extract SHA from entry (Single Responsibility) */
static void extract_sha(const git_index_entry *entry, uint8_t *sha) {
    gm_memcpy(sha, entry->id.id, GM_SHA1_SIZE);
}

/* Get blob SHA from file path */
int gm_sha_from_path(gm_context_t *ctx, const char *path, uint8_t *sha) {
    if (!ctx || !path || !sha) {
        return GM_INVALID_ARG;
    }

    git_index *index = NULL;
    int error = get_git_index(ctx, &index);
    if (error < 0) {
        return GM_ERROR;
    }

    const git_index_entry *entry = find_path_entry(index, path);
    if (!entry) {
        git_index_free(index);
        return GM_NOT_FOUND;
    }

    extract_sha(entry, sha);
    git_index_free(index);
    return GM_OK;
}