/* SPDX-License-Identifier: Apache-2.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind.h"
#include <git2.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/* External command functions */
int gm_cmd_link(gm_context_t *ctx, int argc, char **argv);
int gm_cmd_list(gm_context_t *ctx, int argc, char **argv);
int gm_cmd_install_hooks(gm_context_t *ctx, int argc, char **argv);
int gm_cmd_cache_rebuild(gm_context_t *ctx, int argc, char **argv);

/* External utility functions */
const char *gm_error_string(int error_code);
void gm_log_default(int level, const char *fmt, ...);

/* SAFETY: Prevent running in development repo */
static void safety_check(void) {
    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        return;
    }
    
    /* Check for known dangerous paths */
    const char *dangerous[] = {
        "/git-mind",
        "/git-mind/",
        "neuroglyph/git-mind",
        NULL
    };
    
    for (const char **p = dangerous; *p; p++) {
        if (strstr(cwd, *p) != NULL) {
            fprintf(stderr, "\n");
            fprintf(stderr, "🚨🚨🚨 SAFETY VIOLATION DETECTED! 🚨🚨🚨\n");
            fprintf(stderr, "\n");
            fprintf(stderr, "git-mind MUST NOT be run in its own development repository!\n");
            fprintf(stderr, "Current directory: %s\n", cwd);
            fprintf(stderr, "\n");
            fprintf(stderr, "This is a safety feature to prevent:\n");
            fprintf(stderr, "  - Creating journal commits in the development repo\n");
            fprintf(stderr, "  - Accidentally corrupting the git-mind source\n");
            fprintf(stderr, "  - Breaking the First Commandment of CLAUDE.md\n");
            fprintf(stderr, "\n");
            fprintf(stderr, "To test git-mind:\n");
            fprintf(stderr, "  1. Use 'make test' (runs in Docker)\n");
            fprintf(stderr, "  2. Copy binary to a different repo\n");
            fprintf(stderr, "  3. Run tests in /tmp or other safe location\n");
            fprintf(stderr, "\n");
            fprintf(stderr, "Remember: NEVER run git operations in the working repository!\n");
            fprintf(stderr, "\n");
            exit(42);  /* Special exit code for safety violation */
        }
    }
    
    /* Also check for .git/config containing git-mind */
    FILE *f = fopen(".git/config", "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            if (strstr(line, "neuroglyph/git-mind") || 
                strstr(line, "url = git@github.com:.*git-mind") ||
                strstr(line, "url = https://github.com/.*git-mind")) {
                fclose(f);
                fprintf(stderr, "\n");
                fprintf(stderr, "🚨 SAFETY: Detected git-mind development repo! 🚨\n");
                fprintf(stderr, "Use 'make test' instead.\n");
                fprintf(stderr, "\n");
                exit(42);
            }
        }
        fclose(f);
    }
}

/* Print usage */
static void print_usage(const char *prog) {
    printf("Usage: %s <command> [args...]\n", prog);
    printf("\nCommands:\n");
    printf("  link <source> <target> [--type <type>]  Create a link between files\n");
    printf("  list [<path>] [--branch <branch>]       List links\n");
    printf("  install-hooks                            Install git hooks for AUGMENTS\n");
    printf("  cache-rebuild [--branch <branch>]        Rebuild bitmap cache for fast queries\n");
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
    
    /* SAFETY FIRST! */
    safety_check();
    
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
    } else if (strcmp(cmd, "install-hooks") == 0) {
        result = gm_cmd_install_hooks(&ctx, argc - 2, argv + 2);
    } else if (strcmp(cmd, "cache-rebuild") == 0) {
        result = gm_cmd_cache_rebuild(&ctx, argc - 2, argv + 2);
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