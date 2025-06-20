/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "../../include/gitmind.h"
#include "../cache/cache.h"
#include <git2.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/* Command: git-mind cache-rebuild [--branch <branch>] [--force] */
int gm_cmd_cache_rebuild(gm_context_t *ctx, int argc, char **argv) {
    const char *branch = NULL;
    bool force = false;
    int rc;
    
    /* Parse arguments */
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], GM_FLAG_BRANCH) == 0 && i + 1 < argc) {
            branch = argv[++i];
        } else if (strcmp(argv[i], GM_FLAG_FORCE) == 0) {
            force = true;
        } else if (argv[i][0] == GM_OPTION_PREFIX) {
            gm_output_error(ctx->output, GM_ERROR_UNKNOWN_OPT "\n", argv[i]);
            return GM_INVALID_ARG;
        }
    }
    
    /* Get current branch if not specified */
    if (!branch) {
        git_reference *head = NULL;
        rc = git_repository_head(&head, ctx->git_repo);
        if (rc < 0) {
            gm_output_error(ctx->output, GM_ERROR_GET_BRANCH "\n");
            return GM_ERROR;
        }
        
        branch = git_reference_shorthand(head);
        git_reference_free(head);
    }
    
    /* Check if cache needs rebuild */
    if (!force && !gm_cache_is_stale(ctx->git_repo, branch)) {
        if (gm_output_is_porcelain(ctx->output)) {
            gm_output_porcelain(ctx->output, "status", "up-to-date");
            gm_output_porcelain(ctx->output, "branch", "%s", branch);
        } else {
            gm_output_print(ctx->output, GM_MSG_CACHE_CURRENT "\n", branch);
        }
        return GM_OK;
    }
    
    /* Start rebuild */
    gm_output_verbose(ctx->output, GM_MSG_CACHE_REBUILD "\n", branch);
    clock_t start = clock();
    
    /* Pass force flag through context */
    ctx->user_data = &force;
    rc = gm_cache_rebuild(ctx, branch);
    ctx->user_data = NULL;
    if (rc != GM_OK) {
        gm_output_error(ctx->output, GM_ERROR_CACHE_FAILED "\n", gm_error_string(rc));
        return rc;
    }
    
    /* Report statistics */
    clock_t end = clock();
    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    uint64_t edge_count = 0;
    uint64_t cache_size = 0;
    gm_cache_stats(ctx->git_repo, branch, &edge_count, &cache_size);
    
    if (gm_output_is_porcelain(ctx->output)) {
        gm_output_porcelain(ctx->output, "status", "success");
        gm_output_porcelain(ctx->output, "branch", "%s", branch);
        gm_output_porcelain(ctx->output, "edges", "%llu", (unsigned long long)edge_count);
        gm_output_porcelain(ctx->output, "cache_size_kb", "%llu", (unsigned long long)(cache_size / GM_BYTES_PER_KB));
        gm_output_porcelain(ctx->output, "build_time_seconds", GM_FMT_TIME_SECONDS, elapsed);
    } else {
        gm_output_print(ctx->output, GM_MSG_CACHE_SUCCESS "\n");
        gm_output_print(ctx->output, GM_MSG_CACHE_STATS "\n", (unsigned long long)edge_count, (unsigned long long)(cache_size / GM_BYTES_PER_KB), elapsed);
        gm_output_print(ctx->output, GM_MSG_CACHE_PERF "\n");
    }
    
    return GM_OK;
}