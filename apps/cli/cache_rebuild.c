/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <git2.h>

#include <stdio.h>
#include <string.h>
#include <time.h>

#include <stdbool.h>
#include "gitmind/output.h"
#include "gitmind/context.h"
#include "gitmind/cache.h"
#include "gitmind/error.h"
#include "gitmind/constants.h"
#include "gitmind/constants_internal.h"
#include "cli_runtime.h"
/* Parse command line arguments */
static int parse_cache_rebuild_args(gm_cli_ctx_t *cli, int argc, char **argv, const char **branch,
                                    bool *force) {
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], GM_FLAG_BRANCH) == 0 && i + 1 < argc) {
            *branch = argv[++i];
        } else if (strcmp(argv[i], GM_FLAG_FORCE) == 0) {
            *force = true;
        } else if (argv[i][0] == GM_OPTION_PREFIX) {
            gm_output_error(cli->out, GM_ERROR_UNKNOWN_OPT "\n", argv[i]);
            return GM_ERR_INVALID_ARGUMENT;
        }
    }
    return GM_OK;
}

/* Get current branch name */
static int get_current_branch(git_repository *repo, const char **branch,
                              gm_output_t *output) {
    git_reference *head = NULL;
    int rc = git_repository_head(&head, repo);
    if (rc < 0) {
        gm_output_error(output, GM_ERROR_GET_BRANCH "\n");
        return GM_ERR_INVALID_ARGUMENT;
    }

    *branch = git_reference_shorthand(head);
    git_reference_free(head);
    return GM_OK;
}

/* Report cache is up to date */
static void report_cache_current(gm_output_t *output, const char *branch) {
    if (gm_output_is_porcelain(output)) {
        gm_output_porcelain(output, PORCELAIN_KEY_STATUS,
                            PORCELAIN_STATUS_UP_TO_DATE);
        gm_output_porcelain(output, PORCELAIN_KEY_BRANCH, "%s", branch);
    } else {
        gm_output_print(output, GM_MSG_CACHE_CURRENT "\n", branch);
    }
}

/* Report rebuild success */
static void report_rebuild_success(gm_output_t *output, const char *branch,
                                   uint64_t edge_count, uint64_t cache_size,
                                   double elapsed) {
    if (gm_output_is_porcelain(output)) {
        gm_output_porcelain(output, PORCELAIN_KEY_STATUS,
                            PORCELAIN_STATUS_SUCCESS);
        gm_output_porcelain(output, PORCELAIN_KEY_BRANCH, "%s", branch);
        gm_output_porcelain(output, PORCELAIN_KEY_EDGES, "%llu",
                            (unsigned long long)edge_count);
        gm_output_porcelain(output, PORCELAIN_KEY_CACHE_SIZE_KB, "%llu",
                            (unsigned long long)(cache_size / GM_BYTES_PER_KB));
        gm_output_porcelain(output, PORCELAIN_KEY_BUILD_TIME,
                            GM_FMT_TIME_SECONDS, elapsed);
    } else {
        gm_output_print(output, GM_MSG_CACHE_SUCCESS "\n");
        gm_output_print(
            output, GM_MSG_CACHE_STATS "\n", (unsigned long long)edge_count,
            (unsigned long long)(cache_size / GM_BYTES_PER_KB), elapsed);
        gm_output_print(output, GM_MSG_CACHE_PERF "\n");
    }
}

/* Execute cache rebuild */
static int execute_cache_rebuild(gm_context_t *ctx, gm_cli_ctx_t *cli, const char *branch,
                                 bool force) {
    gm_output_verbose(cli->out, GM_MSG_CACHE_REBUILD "\n", branch);
    clock_t start = clock();

    int rc = gm_cache_rebuild(ctx, branch, force);

    if (rc != GM_OK) {
        gm_output_error(cli->out, GM_ERROR_CACHE_FAILED "\n", "failed");
        return rc;
    }

    /* Get statistics */
    clock_t end = clock();
    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC;

    uint64_t edge_count = 0;
    uint64_t cache_size = 0;
    gm_cache_stats(ctx, branch, &edge_count, &cache_size);

    report_rebuild_success(cli->out, branch, edge_count, cache_size, elapsed);
    return GM_OK;
}

/* Command: git-mind cache-rebuild [--branch <branch>] [--force] */
/* Forward declaration to satisfy -Wmissing-prototypes */
int gm_cmd_cache_rebuild(gm_context_t *ctx, gm_cli_ctx_t *cli, int argc, char **argv);

int gm_cmd_cache_rebuild(gm_context_t *ctx, gm_cli_ctx_t *cli, int argc, char **argv) {
    const char *branch = NULL;
    bool force = false;
    int rc;

    /* Parse arguments */
    rc = parse_cache_rebuild_args(cli, argc, argv, &branch, &force);
    if (rc != GM_OK) {
        return rc;
    }

    /* Get current branch if not specified */
    if (!branch) {
        rc = get_current_branch(ctx->git_repo, &branch, cli->out);
        if (rc != GM_OK) {
            return rc;
        }
    }

    /* Check if cache needs rebuild */
    if (!force && !gm_cache_is_stale(ctx->git_repo, branch)) {
        report_cache_current(cli->out, branch);
        return GM_OK;
    }

    /* Execute rebuild */
    return execute_cache_rebuild(ctx, cli, branch, force);
}
