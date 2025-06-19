/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define HOOK_SCRIPT "#!/bin/sh\n" \
    "# SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0\n" \
    "# git-mind post-commit hook\n" \
    "\n" \
    "# Find git-mind-hook binary\n" \
    "HOOK_BIN=\"$(dirname \"$0\")/../../build/bin/git-mind-hook\"\n" \
    "if [ ! -x \"$HOOK_BIN\" ]; then\n" \
    "    # Try global installation\n" \
    "    HOOK_BIN=\"$(which git-mind-hook 2>/dev/null)\"\n" \
    "fi\n" \
    "\n" \
    "# Run hook if found\n" \
    "if [ -x \"$HOOK_BIN\" ]; then\n" \
    "    \"$HOOK_BIN\" \"$@\"\n" \
    "fi\n" \
    "\n" \
    "# Always exit 0 to not block commits\n" \
    "exit 0\n"

#define HOOK_PATH ".git/hooks/post-commit"
#define HOOKS_DIR ".git/hooks"
#define HOOK_IDENTIFIER "git-mind post-commit hook"
#define HOOK_PERMS 0755
#define BUFFER_SIZE 256

/* Check if git hooks directory exists */
static int check_git_hooks_directory(void) {
    struct stat st;
    int error = stat(HOOKS_DIR, &st);
    
    if (error != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Error: .git/hooks directory not found\n");
        fprintf(stderr, "Are you in a git repository?\n");
        return GM_ERROR;
    }
    
    return GM_OK;
}

/* Check if hook is already ours */
static int check_existing_hook(const char *hook_path, int *is_ours) {
    struct stat st;
    FILE *fp;
    char line[BUFFER_SIZE];
    
    *is_ours = 0;
    
    if (stat(hook_path, &st) != 0) {
        /* Hook doesn't exist */
        return GM_NOT_FOUND;
    }
    
    /* Hook exists, check if it's ours */
    fp = fopen(hook_path, "r");
    if (!fp) {
        return GM_IO_ERROR;
    }
    
    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, HOOK_IDENTIFIER)) {
            *is_ours = 1;
            break;
        }
    }
    fclose(fp);
    
    return GM_OK;
}

/* Backup existing hook */
static int backup_existing_hook(const char *hook_path) {
    char backup_path[BUFFER_SIZE];
    snprintf(backup_path, sizeof(backup_path), "%s.backup", hook_path);
    
    printf("Existing post-commit hook found\n");
    printf("Backing up to: %s\n", backup_path);
    
    if (rename(hook_path, backup_path) != 0) {
        perror("Failed to backup existing hook");
        return GM_ERROR;
    }
    
    return GM_OK;
}

/* Write hook script to file */
static int write_hook_script(const char *hook_path) {
    FILE *fp = fopen(hook_path, "w");
    if (!fp) {
        perror("Failed to create post-commit hook");
        return GM_ERROR;
    }
    
    size_t script_len = strlen(HOOK_SCRIPT);
    if (fwrite(HOOK_SCRIPT, 1, script_len, fp) != script_len) {
        fclose(fp);
        unlink(hook_path);
        fprintf(stderr, "Failed to write hook script\n");
        return GM_ERROR;
    }
    
    fclose(fp);
    return GM_OK;
}

/* Make hook executable */
static int make_hook_executable(const char *hook_path) {
    if (chmod(hook_path, HOOK_PERMS) != 0) {
        perror("Failed to make hook executable");
        unlink(hook_path);
        return GM_ERROR;
    }
    return GM_OK;
}

/* Print installation success message */
static void print_success_message(void) {
    printf("✅ git-mind hooks installed successfully\n");
    printf("\n");
    printf("The post-commit hook will automatically create AUGMENTS edges\n");
    printf("when you modify files that have existing semantic links.\n");
    printf("\n");
    printf("To test: modify a linked file and commit the change.\n");
}

/* Install git hooks */
int gm_cmd_install_hooks(gm_context_t *ctx, int argc, char **argv) {
    (void)ctx; /* Not used */
    (void)argc;
    (void)argv;
    
    int is_ours = 0;
    int rc;
    
    /* Check if .git/hooks exists */
    rc = check_git_hooks_directory();
    if (rc != GM_OK) {
        return rc;
    }
    
    /* Check if hook already exists */
    rc = check_existing_hook(HOOK_PATH, &is_ours);
    if (rc == GM_OK) {
        if (is_ours) {
            printf("git-mind hooks already installed\n");
            return GM_OK;
        }
        
        /* Not our hook, back it up */
        rc = backup_existing_hook(HOOK_PATH);
        if (rc != GM_OK) {
            return rc;
        }
    }
    
    /* Write our hook */
    rc = write_hook_script(HOOK_PATH);
    if (rc != GM_OK) {
        return rc;
    }
    
    /* Make executable */
    rc = make_hook_executable(HOOK_PATH);
    if (rc != GM_OK) {
        return rc;
    }
    
    /* Print success message */
    print_success_message();
    
    return GM_OK;
}