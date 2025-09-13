/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/output.h"
#include "gitmind/context.h"
#include "gitmind/error.h"
#include "cli_runtime.h"

#include "gitmind/constants_internal.h"
#include "../../include/gitmind/constants.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define HOOK_SCRIPT                                                            \
    "#!/bin/sh\n"                                                              \
    "# SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0\n"                    \
    "# git-mind post-commit hook\n"                                            \
    "\n"                                                                       \
    "# Find git-mind-hook binary\n"                                            \
    "HOOK_BIN=\"$(dirname \"$0\")/../../build/bin/git-mind-hook\"\n"           \
    "if [ ! -x \"$HOOK_BIN\" ]; then\n"                                        \
    "    # Try global installation\n"                                          \
    "    HOOK_BIN=\"$(which git-mind-hook 2>/dev/null)\"\n"                    \
    "fi\n"                                                                     \
    "\n"                                                                       \
    "# Run hook if found\n"                                                    \
    "if [ -x \"$HOOK_BIN\" ]; then\n"                                          \
    "    \"$HOOK_BIN\" \"$@\"\n"                                               \
    "fi\n"                                                                     \
    "\n"                                                                       \
    "# Always exit 0 to not block commits\n"                                   \
    "exit 0\n"

/* Hook constants are now defined in gitmind/constants.h */

/* Forward declaration to satisfy -Wmissing-prototypes */
int gm_cmd_install_hooks(gm_context_t *ctx, gm_cli_ctx_t *cli, int argc, char **argv);

/* Check if git hooks directory exists */
static int check_git_hooks_directory(gm_output_t *output) {
    /* Try to create directory with mkdir - it will fail safely if it exists */
    if (mkdir(GM_HOOKS_DIR, 0755) != 0) {
        if (errno != EEXIST) {
            gm_output_error(output, GM_ERR_HOOK_NO_DIR "\n");
            return GM_ERR_IO_FAILED;
        }
        /* Directory already exists - that's fine */
    }

    return GM_OK;
}

/* Check if hook is already ours */
static int check_existing_hook(const char *hook_path, int *is_ours) {
    FILE *fp;
    char line[GM_HOOK_BUFFER_SIZE];

    *is_ours = 0;

    /* Directly try to open - avoids TOCTOU */
    fp = fopen(hook_path, "r");
    if (!fp) {
        if (errno == ENOENT) {
            /* Hook doesn't exist */
            return GM_ERR_NOT_FOUND;
        }
        return GM_ERR_IO_FAILED;
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
    snprintf(backup_path, sizeof(backup_path), "%s" GM_HOOK_BACKUP_SUFFIX,
             hook_path);

    gm_output_print(output, GM_MSG_HOOK_EXISTS "\n");
    gm_output_print(output, GM_MSG_HOOK_BACKUP "\n", backup_path);

    if (rename(hook_path, backup_path) != 0) {
        gm_output_error(output, GM_ERR_HOOK_BACKUP "\n", strerror(errno));
        return GM_ERR_IO_FAILED;
    }

    return GM_OK;
}

/* Write hook script to file */
static int write_hook_script(const char *hook_path, gm_output_t *output) {
    FILE *fp = fopen(hook_path, "w");
    if (!fp) {
        gm_output_error(output, GM_ERR_HOOK_CREATE "\n", strerror(errno));
        return GM_ERR_IO_FAILED;
    }

    size_t script_len = strlen(HOOK_SCRIPT);
    if (fwrite(HOOK_SCRIPT, 1, script_len, fp) != script_len) {
        fclose(fp);
        unlink(hook_path);
        gm_output_error(output, GM_ERR_HOOK_WRITE "\n");
        return GM_ERR_IO_FAILED;
    }

    fclose(fp);
    return GM_OK;
}

/* Make hook executable */
static int make_hook_executable(const char *hook_path, gm_output_t *output) {
    if (chmod(hook_path, GM_HOOK_PERMS) != 0) {
        gm_output_error(output, GM_ERR_HOOK_CHMOD "\n", strerror(errno));
        unlink(hook_path);
        return GM_ERR_IO_FAILED;
    }
    return GM_OK;
}

/* Print installation success message */
static void print_success_message(gm_output_t *output) {
    if (gm_output_is_porcelain(output)) {
        gm_output_porcelain(output, PORCELAIN_KEY_STATUS,
                            PORCELAIN_STATUS_INSTALLED);
        gm_output_porcelain(output, PORCELAIN_KEY_HOOK, POST_COMMIT_HOOK_NAME);
    } else {
        gm_output_print(output, GM_MSG_HOOK_INSTALLED "\n");
        gm_output_print(output, GM_MSG_HOOK_DETAILS "\n");
    }
}

/* Install git hooks */
int gm_cmd_install_hooks(gm_context_t *ctx, gm_cli_ctx_t *cli, int argc, char **argv) {
    (void)ctx;
    (void)argc;
    (void)argv;

    int is_ours = 0;
    int rc;

    /* Check if .git/hooks exists */
    rc = check_git_hooks_directory(cli->out);
    if (rc != GM_OK) {
        return rc;
    }

    /* Check if hook already exists */
    rc = check_existing_hook(GM_HOOK_PATH, &is_ours);
    if (rc == GM_OK) {
        if (is_ours) {
if (gm_output_is_porcelain(cli->out)) {
                gm_output_porcelain(cli->out, PORCELAIN_KEY_STATUS,
                                    PORCELAIN_STATUS_ALREADY_INSTALLED);
            } else {
                gm_output_print(cli->out, GM_MSG_HOOK_ALREADY "\n");
            }
            return GM_OK;
        }

        /* Not our hook, back it up */
        rc = backup_existing_hook(GM_HOOK_PATH, cli->out);
        if (rc != GM_OK) {
            return rc;
        }
    }

    /* Write our hook */
    rc = write_hook_script(GM_HOOK_PATH, cli->out);
    if (rc != GM_OK) {
        return rc;
    }

    /* Make executable */
    rc = make_hook_executable(GM_HOOK_PATH, cli->out);
    if (rc != GM_OK) {
        return rc;
    }

    /* Print success message */
    print_success_message(cli->out);

    return GM_OK;
}
