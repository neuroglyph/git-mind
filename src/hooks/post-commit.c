/* SPDX-License-Identifier: Apache-2.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#define _GNU_SOURCE  /* For getline() and strdup() */
#include "augment.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* External functions we need */
int journal_create_commit(git_repository *repo, const char *ref, 
                         const void *data, size_t len);
int gm_journal_read(gm_context_t *ctx, const char *branch,
                   int (*callback)(const gm_edge_t *edge, void *userdata),
                   void *userdata);
int gm_ulid_generate(char *ulid);

/* Default git operations for context */
static int resolve_blob(void *repo, const char *path, uint8_t *sha) {
    return get_blob_sha((git_repository *)repo, "HEAD", path, sha);
}

static int create_commit(void *repo, const char *ref, const void *data, size_t len) {
    return journal_create_commit((git_repository *)repo, ref, data, len);
}

static int read_commits(void *repo, const char *ref, void *callback, void *userdata) {
    /* Not used in hook */
    (void)repo;
    (void)ref;
    (void)callback;
    (void)userdata;
    return GM_OK;
}

/* Get list of changed files from git */
static int get_changed_files(char ***files_out, size_t *count_out) {
    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    char **files = NULL;
    size_t capacity = 10;
    size_t count = 0;
    
    /* Initial allocation */
    files = malloc(capacity * sizeof(char *));
    if (!files) {
        return GM_NO_MEMORY;
    }
    
    /* Run git diff to get changed files */
    fp = popen("git diff HEAD~1 HEAD --name-only 2>/dev/null", "r");
    if (!fp) {
        free(files);
        return GM_ERROR;
    }
    
    /* Read each file */
    while ((read = getline(&line, &len, fp)) != -1) {
        /* Remove newline */
        if (line[read - 1] == '\n') {
            line[read - 1] = '\0';
        }
        
        /* Grow array if needed */
        if (count >= capacity) {
            capacity *= 2;
            char **new_files = realloc(files, capacity * sizeof(char *));
            if (!new_files) {
                /* Clean up */
                for (size_t i = 0; i < count; i++) {
                    free(files[i]);
                }
                free(files);
                free(line);
                pclose(fp);
                return GM_NO_MEMORY;
            }
            files = new_files;
        }
        
        /* Copy file path */
        files[count] = strdup(line);
        if (!files[count]) {
            /* Clean up */
            for (size_t i = 0; i < count; i++) {
                free(files[i]);
            }
            free(files);
            free(line);
            pclose(fp);
            return GM_NO_MEMORY;
        }
        count++;
    }
    
    free(line);
    pclose(fp);
    
    *files_out = files;
    *count_out = count;
    return GM_OK;
}

/* Main hook entry point */
int main(int argc, char **argv) {
    git_repository *repo = NULL;
    gm_context_t ctx;
    char **changed_files = NULL;
    size_t file_count = 0;
    bool is_merge = false;
    int error = 0;
    
    /* Silent operation by default */
    int verbose = 0;
    if (argc > 1 && strcmp(argv[1], "--verbose") == 0) {
        verbose = 1;
    }
    
    /* Initialize libgit2 */
    git_libgit2_init();
    
    /* Open repository */
    error = git_repository_open(&repo, ".");
    if (error < 0) {
        if (verbose) {
            fprintf(stderr, "Failed to open repository\n");
        }
        git_libgit2_shutdown();
        return 0; /* Don't fail the commit */
    }
    
    /* Check if merge commit */
    error = is_merge_commit(repo, &is_merge);
    if (error != GM_OK || is_merge) {
        if (verbose && is_merge) {
            printf("Skipping merge commit\n");
        }
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
    
    /* Skip if too many files */
    if (file_count > MAX_CHANGED_FILES) {
        if (verbose) {
            printf("Skipping: %zu files changed (max %d)\n", 
                   file_count, MAX_CHANGED_FILES);
        }
        goto cleanup;
    }
    
    /* Initialize context */
    memset(&ctx, 0, sizeof(ctx));
    ctx.git_ops.resolve_blob = resolve_blob;
    ctx.git_ops.create_commit = create_commit;
    ctx.git_ops.read_commits = read_commits;
    ctx.git_repo = repo;
    
    /* Process each changed file */
    for (size_t i = 0; i < file_count; i++) {
        if (verbose) {
            printf("Processing: %s\n", changed_files[i]);
        }
        
        error = process_changed_file(&ctx, repo, changed_files[i]);
        if (error != GM_OK && verbose) {
            fprintf(stderr, "Failed to process %s: %d\n", 
                    changed_files[i], error);
        }
    }
    
cleanup:
    /* Free changed files */
    for (size_t i = 0; i < file_count; i++) {
        free(changed_files[i]);
    }
    free(changed_files);
    
    /* Cleanup */
    git_repository_free(repo);
    git_libgit2_shutdown();
    
    return 0; /* Never fail the commit */
}