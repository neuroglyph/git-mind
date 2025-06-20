/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

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

/* Hook constants are now defined in gitmind/constants.h */

/* Check if git hooks directory exists */
static int check_git_hooks_directory(gm_output_t *output) {
    struct stat st;
    int error = stat(GM_HOOKS_DIR, &st);
    
    if (error != 0 || !S_ISDIR(st.st_mode)) {
        gm_output_error(output, GM_ERR_HOOK_NO_DIR "\n");
        return GM_ERROR;
    }
    
    return GM_OK;
}

/* Check if hook is already ours */
static int check_existing_hook(const char *hook_path, int *is_ours) {
    struct stat st;
    FILE *fp;
    char line[GM_HOOK_BUFFER_SIZE];
    
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
        if (strstr(line, GM_HOOK_IDENTIFIER)) {
            *is_ours = 1;
            break;
        }
    }
    fclose(fp);
    
    return GM_OK;
}

/* Backup existing hook */
static int backup_existing_hook(const char *hook_path, gm_output_t *output) {
    char backup_path[GM_HOOK_BUFFER_SIZE];
    snprintf(backup_path, sizeof(backup_path), "%s" GM_HOOK_BACKUP_SUFFIX, hook_path);
    
    gm_output_print(output, GM_MSG_HOOK_EXISTS "\n");
    gm_output_print(output, GM_MSG_HOOK_BACKUP "\n", backup_path);
    
    if (rename(hook_path, backup_path) != 0) {
        gm_output_error(output, GM_ERR_HOOK_BACKUP "\n", strerror(errno));
        return GM_ERROR;
    }
    
    return GM_OK;
}

/* Write hook script to file */
static int write_hook_script(const char *hook_path, gm_output_t *output) {
    FILE *fp = fopen(hook_path, "w");
    if (!fp) {
        gm_output_error(output, GM_ERR_HOOK_CREATE "\n", strerror(errno));
        return GM_ERROR;
    }
    
    size_t script_len = strlen(HOOK_SCRIPT);
    if (fwrite(HOOK_SCRIPT, 1, script_len, fp) != script_len) {
        fclose(fp);
        unlink(hook_path);
        gm_output_error(output, GM_ERR_HOOK_WRITE "\n");
        return GM_ERROR;
    }
    
    fclose(fp);
    return GM_OK;
}

/* Make hook executable */
static int make_hook_executable(const char *hook_path, gm_output_t *output) {
    if (chmod(hook_path, GM_HOOK_PERMS) != 0) {
        gm_output_error(output, GM_ERR_HOOK_CHMOD "\n", strerror(errno));
        unlink(hook_path);
        return GM_ERROR;
    }
    return GM_OK;
}

/* Print installation success message */
static void print_success_message(gm_output_t *output) {
    if (gm_output_is_porcelain(output)) {
        gm_output_porcelain(output, "status", "installed");
        gm_output_porcelain(output, "hook", "post-commit");
    } else {
        gm_output_print(output, GM_MSG_HOOK_INSTALLED "\n");
        gm_output_print(output, GM_MSG_HOOK_DETAILS "\n");
    }
}

/* Install git hooks */
int gm_cmd_install_hooks(gm_context_t *ctx, int argc, char **argv) {
    (void)ctx; /* Not used */
    (void)argc;
    (void)argv;
    
    int is_ours = 0;
    int rc;
    
    /* Check if .git/hooks exists */
    rc = check_git_hooks_directory(ctx->output);
    if (rc != GM_OK) {
        return rc;
    }
    
    /* Check if hook already exists */
    rc = check_existing_hook(GM_HOOK_PATH, &is_ours);
    if (rc == GM_OK) {
        if (is_ours) {
            if (gm_output_is_porcelain(ctx->output)) {
                gm_output_porcelain(ctx->output, "status", "already-installed");
            } else {
                gm_output_print(ctx->output, GM_MSG_HOOK_ALREADY "\n");
            }
            return GM_OK;
        }
        
        /* Not our hook, back it up */
        rc = backup_existing_hook(GM_HOOK_PATH, ctx->output);
        if (rc != GM_OK) {
            return rc;
        }
    }
    
    /* Write our hook */
    rc = write_hook_script(GM_HOOK_PATH, ctx->output);
    if (rc != GM_OK) {
        return rc;
    }
    
    /* Make executable */
    rc = make_hook_executable(GM_HOOK_PATH, ctx->output);
    if (rc != GM_OK) {
        return rc;
    }
    
    /* Print success message */
    print_success_message(ctx->output);
    
    return GM_OK;
}