/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "../../include/gitmind.h"
#include "../../include/gitmind/constants_internal.h"
#include "../cache/cache.h"
#include <git2.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/* Parse command line arguments */
static int parse_cache_rebuild_args(int argc, char **argv, const char **branch, 
                                   bool *force, gm_output_t *output) {
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], GM_FLAG_BRANCH) == 0 && i + 1 < argc) {
            *branch = argv[++i];
        } else if (strcmp(argv[i], GM_FLAG_FORCE) == 0) {
            *force = true;
        } else if (argv[i][0] == GM_OPTION_PREFIX) {
            gm_output_error(output, GM_ERROR_UNKNOWN_OPT "\n", argv[i]);
            return GM_INVALID_ARG;
        }
    }
    return GM_OK;
}

/* Get current branch name */
static int get_current_branch(git_repository *repo, const char **branch, gm_output_t *output) {
    git_reference *head = NULL;
    int rc = git_repository_head(&head, repo);
    if (rc < 0) {
        gm_output_error(output, GM_ERROR_GET_BRANCH "\n");
        return GM_ERROR;
    }
    
    *branch = git_reference_shorthand(head);
    git_reference_free(head);
    return GM_OK;
}

/* Report cache is up to date */
static void report_cache_current(gm_output_t *output, const char *branch) {
    if (gm_output_is_porcelain(output)) {
        gm_output_porcelain(output, PORCELAIN_KEY_STATUS, PORCELAIN_STATUS_UP_TO_DATE);
        gm_output_porcelain(output, PORCELAIN_KEY_BRANCH, "%s", branch);
    } else {
        gm_output_print(output, GM_MSG_CACHE_CURRENT "\n", branch);
    }
}

/* Report rebuild success */
static void report_rebuild_success(gm_output_t *output, const char *branch,
                                  uint64_t edge_count, uint64_t cache_size, double elapsed) {
    if (gm_output_is_porcelain(output)) {
        gm_output_porcelain(output, PORCELAIN_KEY_STATUS, PORCELAIN_STATUS_SUCCESS);
        gm_output_porcelain(output, PORCELAIN_KEY_BRANCH, "%s", branch);
        gm_output_porcelain(output, PORCELAIN_KEY_EDGES, "%llu", (unsigned long long)edge_count);
        gm_output_porcelain(output, PORCELAIN_KEY_CACHE_SIZE_KB, "%llu", (unsigned long long)(cache_size / GM_BYTES_PER_KB));
        gm_output_porcelain(output, PORCELAIN_KEY_BUILD_TIME, GM_FMT_TIME_SECONDS, elapsed);
    } else {
        gm_output_print(output, GM_MSG_CACHE_SUCCESS "\n");
        gm_output_print(output, GM_MSG_CACHE_STATS "\n", 
                       (unsigned long long)edge_count, 
                       (unsigned long long)(cache_size / GM_BYTES_PER_KB), 
                       elapsed);
        gm_output_print(output, GM_MSG_CACHE_PERF "\n");
    }
}

/* Execute cache rebuild */
static int execute_cache_rebuild(gm_context_t *ctx, const char *branch, bool force) {
    gm_output_verbose(ctx->output, GM_MSG_CACHE_REBUILD "\n", branch);
    clock_t start = clock();
    
    /* Pass force flag through context */
    ctx->user_data = &force;
    int rc = gm_cache_rebuild(ctx, branch);
    ctx->user_data = NULL;
    
    if (rc != GM_OK) {
        gm_output_error(ctx->output, GM_ERROR_CACHE_FAILED "\n", gm_error_string(rc));
        return rc;
    }
    
    /* Get statistics */
    clock_t end = clock();
    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    uint64_t edge_count = 0;
    uint64_t cache_size = 0;
    gm_cache_stats(ctx->git_repo, branch, &edge_count, &cache_size);
    
    report_rebuild_success(ctx->output, branch, edge_count, cache_size, elapsed);
    return GM_OK;
}

/* Command: git-mind cache-rebuild [--branch <branch>] [--force] */
int gm_cmd_cache_rebuild(gm_context_t *ctx, int argc, char **argv) {
    const char *branch = NULL;
    bool force = false;
    int rc;
    
    /* Parse arguments */
    rc = parse_cache_rebuild_args(argc, argv, &branch, &force, ctx->output);
    if (rc != GM_OK) {
        return rc;
    }
    
    /* Get current branch if not specified */
    if (!branch) {
        rc = get_current_branch(ctx->git_repo, &branch, ctx->output);
        if (rc != GM_OK) {
            return rc;
        }
    }
    
    /* Check if cache needs rebuild */
    if (!force && !gm_cache_is_stale(ctx->git_repo, branch)) {
        report_cache_current(ctx->output, branch);
        return GM_OK;
    }
    
    /* Execute rebuild */
    return execute_cache_rebuild(ctx, branch, force);
}