/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#define _GNU_SOURCE /* For getline() and strdup() */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../../include/gitmind/constants_internal.h"
#include "augment.h"
#include "gitmind/adapters/git/libgit2_repository_port.h"
#include "gitmind/error.h"
#include <gitmind/security/memory.h>

/* Array management constants */
#define INITIAL_FILE_ARRAY_SIZE 10
#define ARRAY_GROWTH_FACTOR 2

/* External functions we need */
int gm_journal_read(gm_context_t *ctx, const char *branch,
                    int (*callback)(const gm_edge_t *edge, void *userdata),
                    void *userdata);
int gm_ulid_generate(char *ulid);

/* Get list of changed files from git */
/* Free files array on error */
static void free_files_array(char **files, size_t count) {
    if (!files)
        return;
    for (size_t i = 0; i < count; i++) {
        free(files[i]);
    }
    free(files);
}

/* Grow files array if needed */
static int grow_files_array(char ***files, size_t *capacity) {
    *capacity *= ARRAY_GROWTH_FACTOR;
    char **new_files = realloc(*files, *capacity * sizeof(char *));
    if (!new_files) {
        return GM_NO_MEMORY;
    }
    *files = new_files;
    return GM_OK;
}

/* Read single file from git diff output */
static int read_file_entry(char **files, size_t *count, size_t *capacity,
                           char *line, ssize_t len) {
    /* Remove newline */
    if (line[len - 1] == '\n') {
        line[len - 1] = '\0';
    }

    /* Grow array if needed */
    if (*count >= *capacity) {
        int error = grow_files_array(&files, capacity);
        if (error != GM_OK) {
            return error;
        }
    }

    /* Copy file path */
    files[*count] = strdup(line);
    if (!files[*count]) {
        return GM_NO_MEMORY;
    }

    (*count)++;
    return GM_OK;
}

static int get_changed_files(char ***files_out, size_t *count_out) {
    FILE *fp = NULL;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    char **files = NULL;
    size_t capacity = INITIAL_FILE_ARRAY_SIZE;
    size_t count = 0;
    int error = GM_OK;

    /* Initial allocation */
    files = malloc(capacity * sizeof(char *));
    if (!files) {
        return GM_NO_MEMORY;
    }

    /* Run git diff to get changed files */
    fp = popen("git diff HEAD~1 HEAD --name-only 2>/dev/null", "r");
    if (!fp) {
        free(files);
        return GM_ERR_IO_FAILED;
    }

    /* Read each file */
    while ((read = getline(&line, &len, fp)) != -1) {
        error = read_file_entry(files, &count, &capacity, line, read);
        if (error != GM_OK) {
            free_files_array(files, count);
            free(line);
            pclose(fp);
            return error;
        }
    }

    free(line);
    pclose(fp);

    *files_out = files;
    *count_out = count;
    return GM_OK;
}

/* Initialize and open repository */
static int initialize_repository(git_repository **repo, int verbose) {
    int error = git_repository_open(repo, ".");
    if (error < 0) {
        if (verbose) {
            fprintf(stderr, "Failed to open repository\n");
        }
        return GM_ERR_UNKNOWN;
    }
    return GM_OK;
}

/* Check if we should process this commit */
static int should_process_commit(git_repository *repo, int verbose) {
    bool is_merge = false;
    int error = is_merge_commit(repo, &is_merge);

    if (error != GM_OK || is_merge) {
        if (verbose && is_merge) {
            printf("Skipping merge commit\n");
        }
        return 0;
    }
    return 1;
}

/* Process all changed files */
static void process_all_files(gm_context_t *ctx, git_repository *repo,
                              char **changed_files, size_t file_count,
                              int verbose) {
    for (size_t i = 0; i < file_count; i++) {
        if (verbose) {
            printf("Processing: %s\n", changed_files[i]);
        }

        int error = process_changed_file(ctx, repo, changed_files[i]);
        if (error != GM_OK && verbose) {
            fprintf(stderr, "Failed to process %s: %d\n", changed_files[i],
                    error);
        }
    }
}

/* Free changed files array */
static void free_changed_files(char **changed_files, size_t file_count) {
    if (!changed_files)
        return;

    for (size_t i = 0; i < file_count; i++) {
        free(changed_files[i]);
    }
    free(changed_files);
}

/* Main hook entry point */
int main(int argc, char **argv) {
    git_repository *repo = NULL;
    gm_context_t ctx;
    char **changed_files = NULL;
    size_t file_count = 0;
    int error = 0;

    /* Silent operation by default */
    int verbose = (argc > 1 && strcmp(argv[1], "--verbose") == 0);

    /* Initialize libgit2 */
    git_libgit2_init();

    /* Open repository */
    error = initialize_repository(&repo, verbose);
    if (error != GM_OK) {
        git_libgit2_shutdown();
        return 0; /* Don't fail the commit */
    }

    /* Check if we should process this commit */
    if (!should_process_commit(repo, verbose)) {
        git_repository_free(repo);
        git_libgit2_shutdown();
        return 0;
    }

    /* Get changed files */
    error = get_changed_files(&changed_files, &file_count);
    if (error != GM_OK) {
        if (verbose) {
            fprintf(stderr, "Failed to get changed files\n");
        }
        git_repository_free(repo);
        git_libgit2_shutdown();
        return 0;
    }

    /* Process files if not too many */
    if (file_count <= MAX_CHANGED_FILES) {
        /* Initialize context */
        gm_memset_safe(&ctx, sizeof(ctx), 0, sizeof(ctx));
        void (*repo_port_dispose)(gm_git_repository_port_t *) = NULL;
        gm_result_void_t port_result =
            gm_libgit2_repository_port_create(&ctx.git_repo_port,
                                              NULL,
                                              &repo_port_dispose, repo);
        if (!port_result.ok) {
            if (port_result.u.err != NULL) {
                gm_error_free(port_result.u.err);
            }
            free_changed_files(changed_files, file_count);
            git_repository_free(repo);
            git_libgit2_shutdown();
            return 0;
        }
        ctx.git_repo_port_dispose = repo_port_dispose;

        process_all_files(&ctx, repo, changed_files, file_count, verbose);

        if (ctx.git_repo_port_dispose != NULL) {
            ctx.git_repo_port_dispose(&ctx.git_repo_port);
            ctx.git_repo_port_dispose = NULL;
        }
    } else if (verbose) {
        printf("Skipping: %zu files changed (max %d)\n", file_count,
               MAX_CHANGED_FILES);
    }

    /* Cleanup */
    free_changed_files(changed_files, file_count);
    git_repository_free(repo);
    git_libgit2_shutdown();

    return 0; /* Never fail the commit */
}
