// SPDX-License-Identifier: Apache-2.0
// © 2025 J. Kirby Ross / Neuroglyph Collective

#include "gitmind.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>

// Safe path joining with bounds checking
int gm_path_join(char* dest, size_t dest_size, const char* dir, const char* file) {
    int ret = snprintf(dest, dest_size, "%s/%s", dir, file);
    if (ret < 0 || (size_t)ret >= dest_size) {
        gm_set_error(ERR_MSG_PATH_TOO_LONG, dir);
        return GM_ERR_PATH_TOO_LONG;
    }
    return GM_OK;
}

// Manual path normalization when realpath fails
static int gm_normalize_path_manual(const char* path, char* out_normalized) {
    if (!path || !out_normalized) {
        return GM_ERR_INVALID_ARG;
    }
    
    // Working buffer for building normalized path
    char buffer[GM_MAX_PATH];
    char* components[GM_MAX_PATH/2];  // Max possible components
    int comp_count = 0;
    
    // Copy path for tokenization
    char path_copy[GM_MAX_PATH];
    snprintf(path_copy, sizeof(path_copy), "%s", path);
    
    // Track if path is absolute
    int is_absolute = (path[0] == '/');
    
    // Tokenize path by '/'
    char* token = strtok(path_copy, "/");
    while (token != NULL) {
        if (strcmp(token, ".") == 0) {
            // Skip current directory references
        } else if (strcmp(token, "..") == 0) {
            // Go up one level if possible
            if (comp_count > 0) {
                comp_count--;
            } else if (!is_absolute) {
                // For relative paths, we can't go above current directory
                // This is a security risk - return error
                gm_set_error("Path traversal not allowed: %s", path);
                return GM_ERR_INVALID_ARG;
            }
        } else if (strlen(token) > 0) {
            // Normal component
            if (comp_count < (int)(sizeof(components)/sizeof(components[0]))) {
                components[comp_count++] = token;
            }
        }
        token = strtok(NULL, "/");
    }
    
    // Rebuild path
    buffer[0] = '\0';
    if (is_absolute) {
        strcpy(buffer, "/");
    }
    
    for (int i = 0; i < comp_count; i++) {
        if (i > 0 || is_absolute) {
            strcat(buffer, "/");
        }
        strcat(buffer, components[i]);
    }
    
    // Handle empty result
    if (buffer[0] == '\0') {
        strcpy(buffer, ".");
    }
    
    // Copy result
    snprintf(out_normalized, GM_MAX_PATH, "%s", buffer);
    
    return GM_OK;
}

// Normalize path (remove ./, ../, trailing slashes)
int gm_normalize_path(const char* path, char* out_normalized) {
    if (!path || !out_normalized) {
        return GM_ERR_INVALID_ARG;
    }
    
    // Always use manual normalization for portability
    return gm_normalize_path_manual(path, out_normalized);
}

// Check if a path component is exactly ".."
static int is_parent_ref(const char* start, const char* end) {
    size_t len = (size_t)(end - start);
    return len == 2 && start[0] == '.' && start[1] == '.';
}

// Validate path is safe (no .., no absolute paths for links)
int gm_validate_link_path(const char* path) {
    if (!path || *path == '\0') {
        gm_set_error("Empty path");
        return GM_ERR_INVALID_ARG;
    }
    
    // No absolute paths in links
    if (path[0] == '/') {
        gm_set_error("Absolute paths not allowed in links: %s", path);
        return GM_ERR_INVALID_ARG;
    }
    
    // Windows-style absolute paths (C:\, D:\, etc.)
    if (strlen(path) >= 3 && path[1] == ':' && 
        (path[2] == '\\' || path[2] == '/')) {
        gm_set_error("Absolute paths not allowed in links: %s", path);
        return GM_ERR_INVALID_ARG;
    }
    
    // Length check
    if (strlen(path) >= GM_MAX_PATH) {
        gm_set_error("Path too long: %s", path);
        return GM_ERR_PATH_TOO_LONG;
    }
    
    // Note: We don't decode URL encoding - %2F is treated as literal "%2F"
    // This is consistent with filesystem behavior
    
    // Check for ".." anywhere in the path BEFORE normalization
    // This is the safest approach - just reject any path with ".."
    const char* p = path;
    const char* component_start = p;
    
    while (*p) {
        if (*p == '/' || *p == '\\') {
            // Check if this component is ".."
            if (is_parent_ref(component_start, p)) {
                gm_set_error("Path traversal not allowed: %s", path);
                return GM_ERR_INVALID_ARG;
            }
            // Skip the separator and start next component
            p++;
            component_start = p;
        } else {
            p++;
        }
    }
    
    // Check the last component
    if (is_parent_ref(component_start, p)) {
        gm_set_error("Path traversal not allowed: %s", path);
        return GM_ERR_INVALID_ARG;
    }
    
    // Now normalize the path for consistency
    char normalized[GM_MAX_PATH];
    if (gm_normalize_path(path, normalized) != GM_OK) {
        return GM_ERR_INVALID_ARG;
    }
    
    return GM_OK;
}
