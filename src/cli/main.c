/* SPDX-License-Identifier: Apache-2.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind.h"
#include <git2.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* External command functions */
int gm_cmd_link(gm_context_t *ctx, int argc, char **argv);
int gm_cmd_list(gm_context_t *ctx, int argc, char **argv);

/* External utility functions */
const char *gm_error_string(int error_code);
void gm_log_default(int level, const char *fmt, ...);

/* Print usage */
static void print_usage(const char *prog) {
    printf("Usage: %s <command> [args...]\n", prog);
    printf("\nCommands:\n");
    printf("  link <source> <target> [--type <type>]  Create a link between files\n");
    printf("  list [<path>] [--branch <branch>]       List links\n");
    printf("\nRelationship types:\n");
    printf("  implements    Source implements target\n");
    printf("  references    Source references target\n");
    printf("  depends_on    Source depends on target\n");
    printf("  augments      Source augments/updates target\n");
}

/* Initialize context with libgit2 */
static int init_context(gm_context_t *ctx) {
    git_repository *repo = NULL;
    int error;
    
    /* Initialize libgit2 */
    git_libgit2_init();
    
    /* Open repository */
    error = git_repository_open(&repo, ".");
    if (error < 0) {
        const git_error *e = git_error_last();
        fprintf(stderr, "Error: Not in a git repository\n");
        if (e) {
            fprintf(stderr, "Git error: %s\n", e->message);
        }
        return GM_ERROR;
    }
    
    /* Set up context */
    memset(ctx, 0, sizeof(gm_context_t));
    ctx->git_repo = repo;
    ctx->log_fn = gm_log_default;
    
    return GM_OK;
}

/* Cleanup context */
static void cleanup_context(gm_context_t *ctx) {
    if (ctx->git_repo) {
        git_repository_free((git_repository *)ctx->git_repo);
    }
    git_libgit2_shutdown();
}

/* Main entry point */
int main(int argc, char **argv) {
    gm_context_t ctx;
    int result;
    
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    /* Initialize context */
    result = init_context(&ctx);
    if (result != GM_OK) {
        return 1;
    }
    
    /* Dispatch command */
    const char *cmd = argv[1];
    if (strcmp(cmd, "link") == 0) {
        result = gm_cmd_link(&ctx, argc - 2, argv + 2);
    } else if (strcmp(cmd, "list") == 0) {
        result = gm_cmd_list(&ctx, argc - 2, argv + 2);
    } else if (strcmp(cmd, "--help") == 0 || strcmp(cmd, "-h") == 0) {
        print_usage(argv[0]);
        result = GM_OK;
    } else {
        fprintf(stderr, "Error: Unknown command '%s'\n", cmd);
        print_usage(argv[0]);
        result = GM_INVALID_ARG;
    }
    
    /* Cleanup */
    cleanup_context(&ctx);
    
    return (result == GM_OK) ? 0 : 1;
}