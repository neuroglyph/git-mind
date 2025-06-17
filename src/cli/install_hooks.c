/* SPDX-License-Identifier: Apache-2.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define HOOK_SCRIPT "#!/bin/sh\n" \
    "# SPDX-License-Identifier: Apache-2.0\n" \
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

/* Install git hooks */
int gm_cmd_install_hooks(gm_context_t *ctx, int argc, char **argv) {
    (void)ctx; /* Not used */
    (void)argc;
    (void)argv;
    
    const char *hook_path = ".git/hooks/post-commit";
    FILE *fp = NULL;
    struct stat st;
    int error;
    
    /* Check if .git/hooks exists */
    error = stat(".git/hooks", &st);
    if (error != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Error: .git/hooks directory not found\n");
        fprintf(stderr, "Are you in a git repository?\n");
        return GM_ERROR;
    }
    
    /* Check if hook already exists */
    error = stat(hook_path, &st);
    if (error == 0) {
        /* Hook exists, check if it's ours */
        fp = fopen(hook_path, "r");
        if (fp) {
            char line[256];
            int is_ours = 0;
            while (fgets(line, sizeof(line), fp)) {
                if (strstr(line, "git-mind post-commit hook")) {
                    is_ours = 1;
                    break;
                }
            }
            fclose(fp);
            
            if (is_ours) {
                printf("git-mind hooks already installed\n");
                return GM_OK;
            }
        }
        
        /* Not our hook, back it up */
        char backup_path[256];
        snprintf(backup_path, sizeof(backup_path), "%s.backup", hook_path);
        
        printf("Existing post-commit hook found\n");
        printf("Backing up to: %s\n", backup_path);
        
        error = rename(hook_path, backup_path);
        if (error != 0) {
            perror("Failed to backup existing hook");
            return GM_ERROR;
        }
    }
    
    /* Write our hook */
    fp = fopen(hook_path, "w");
    if (!fp) {
        perror("Failed to create post-commit hook");
        return GM_ERROR;
    }
    
    if (fwrite(HOOK_SCRIPT, 1, strlen(HOOK_SCRIPT), fp) != strlen(HOOK_SCRIPT)) {
        fclose(fp);
        unlink(hook_path);
        fprintf(stderr, "Failed to write hook script\n");
        return GM_ERROR;
    }
    
    fclose(fp);
    
    /* Make executable */
    error = chmod(hook_path, 0755);
    if (error != 0) {
        perror("Failed to make hook executable");
        unlink(hook_path);
        return GM_ERROR;
    }
    
    printf("✅ git-mind hooks installed successfully\n");
    printf("\n");
    printf("The post-commit hook will automatically create AUGMENTS edges\n");
    printf("when you modify files that have existing semantic links.\n");
    printf("\n");
    printf("To test: modify a linked file and commit the change.\n");
    
    return GM_OK;
}