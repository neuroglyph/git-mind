/* SPDX-License-Identifier: Apache-2.0 */
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
        if (strcmp(argv[i], "--branch") == 0 && i + 1 < argc) {
            branch = argv[++i];
        } else if (strcmp(argv[i], "--force") == 0) {
            force = true;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Error: Unknown option '%s'\n", argv[i]);
            return GM_INVALID_ARG;
        }
    }
    
    /* Get current branch if not specified */
    if (!branch) {
        git_reference *head = NULL;
        rc = git_repository_head(&head, ctx->git_repo);
        if (rc < 0) {
            fprintf(stderr, "Error: Failed to get current branch\n");
            return GM_ERROR;
        }
        
        branch = git_reference_shorthand(head);
        git_reference_free(head);
    }
    
    /* Check if cache needs rebuild */
    if (!force && !gm_cache_is_stale(ctx->git_repo, branch)) {
        printf("Cache is up to date for branch '%s'\n", branch);
        return GM_OK;
    }
    
    /* Start rebuild */
    printf("Rebuilding cache for branch '%s'...\n", branch);
    clock_t start = clock();
    
    /* Pass force flag through context */
    ctx->user_data = &force;
    rc = gm_cache_rebuild(ctx, branch);
    ctx->user_data = NULL;
    if (rc != GM_OK) {
        fprintf(stderr, "Error: Cache rebuild failed: %s\n", gm_error_string(rc));
        return rc;
    }
    
    /* Report statistics */
    clock_t end = clock();
    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC;
    
    uint64_t edge_count = 0;
    uint64_t cache_size = 0;
    gm_cache_stats(ctx->git_repo, branch, &edge_count, &cache_size);
    
    printf("Cache rebuilt successfully!\n");
    printf("  Edges indexed: %llu\n", (unsigned long long)edge_count);
    printf("  Cache size: ~%llu KB\n", (unsigned long long)(cache_size / 1024));
    printf("  Build time: %.2f seconds\n", elapsed);
    printf("\nQueries will now use the bitmap cache for O(log N) performance.\n");
    
    return GM_OK;
}