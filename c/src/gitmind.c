// SPDX-License-Identifier: Apache-2.0
// © 2025 J. Kirby Ross / Neuroglyph Collective

#include "gitmind.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>

// Thread-local error storage with portability
// Note: We compile with -std=c99, so __STDC_VERSION__ == 199901L
// _Thread_local is C11, so we rely on compiler extensions
#if defined(__GNUC__) || defined(__clang__)
    #define THREAD_LOCAL __thread
#elif defined(_MSC_VER)
    #define THREAD_LOCAL __declspec(thread)
#elif __STDC_VERSION__ >= 201112L
    #define THREAD_LOCAL _Thread_local
#else
    #define THREAD_LOCAL  // fallback: no TLS
    #warning "Thread-local storage not available, using global error buffer"
#endif

static THREAD_LOCAL char gm_err_buf[GM_ERROR_BUFFER_SIZE];

// Set error message
void gm_set_error(const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(gm_err_buf, sizeof(gm_err_buf), fmt, args);
    va_end(args);
}

// Get last error
const char* gm_last_error(void) {
    return gm_err_buf;
}

// Get error string for code
const char* gm_error_string(int error_code) {
    switch (error_code) {
        case GM_OK: return "Success";
        case GM_ERR_NOT_REPO: return "Not a git repository";
        case GM_ERR_NOT_FOUND: return "Not found";
        case GM_ERR_IO: return "I/O error";
        case GM_ERR_GIT: return "Git operation failed";
        case GM_ERR_MEMORY: return "Memory allocation failed";
        case GM_ERR_INVALID_ARG: return "Invalid argument";
        case GM_ERR_PATH_TOO_LONG: return "Path too long";
        case GM_ERR_ALREADY_EXISTS: return "Already exists";
        default: return "Unknown error";
    }
}

// Version string
const char* gm_version_string(void) {
    static char version[32];
    snprintf(version, sizeof(version), "%d.%d.%d",
        GITMIND_VERSION_MAJOR,
        GITMIND_VERSION_MINOR,
        GITMIND_VERSION_PATCH);
    return version;
}

// Check if directory exists
static int dir_exists(const char* path) {
    struct stat st;
    return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

// Create directory if it doesn't exist
static int ensure_dir(const char* path) {
    if (dir_exists(path)) {
        return GM_OK;
    }
    
    if (mkdir(path, 0755) != 0) {
        gm_set_error(ERR_MSG_DIR_CREATE_FAILED, path, strerror(errno));
        return GM_ERR_IO;
    }
    
    return GM_OK;
}

// Initialize gitmind in repository
int gm_init(const char* repo_path) {
    if (!repo_path) {
        gm_set_error(ERR_MSG_NULL_POINTER, "repo_path");
        return GM_ERR_INVALID_ARG;
    }
    
    // Check if .git exists
    char git_path[GM_MAX_PATH];
    int ret = snprintf(git_path, sizeof(git_path), "%s/.git", repo_path);
    if (ret < 0 || ret >= (int)sizeof(git_path)) {
        gm_set_error(ERR_MSG_PATH_TOO_LONG, repo_path);
        return GM_ERR_PATH_TOO_LONG;
    }
    
    if (!dir_exists(git_path)) {
        gm_set_error(ERR_MSG_NOT_REPO, repo_path);
        return GM_ERR_NOT_REPO;
    }
    
    // Create .gitmind directory
    char gitmind_path[GM_MAX_PATH];
    snprintf(gitmind_path, sizeof(gitmind_path), "%s/.gitmind", repo_path);
    
    ret = ensure_dir(gitmind_path);
    if (ret != 0) return ret;
    
    // Create .gitmind/links directory (for backward compatibility)
    char links_path[GM_MAX_PATH];
    snprintf(links_path, sizeof(links_path), "%s/.gitmind/links", repo_path);
    
    ret = ensure_dir(links_path);
    if (ret != 0) return ret;
    
    // Create orphan ref for holy architecture
    ret = gm_orphan_ref_create();
    if (ret != 0 && ret != GM_ERR_ALREADY_EXISTS) return ret;
    
    return GM_OK;
}

// Link set operations
gm_link_set_t* gm_link_set_new(void) {
    gm_link_set_t* set = calloc(1, sizeof(gm_link_set_t));
    if (!set) {
        gm_set_error("Memory allocation failed");
        return NULL;
    }
    
    set->capacity = 16;
    set->links = calloc(set->capacity, sizeof(gm_link_t));
    if (!set->links) {
        free(set);
        gm_set_error("Memory allocation failed");
        return NULL;
    }
    
    return set;
}

void gm_link_set_free(gm_link_set_t* set) {
    if (set) {
        free(set->links);
        free(set);
    }
}

int gm_link_set_add(gm_link_set_t* set, const gm_link_t* link) {
    if (!set || !link) {
        gm_set_error("Invalid argument");
        return GM_ERR_INVALID_ARG;
    }
    
    // Grow if needed
    if (set->count >= set->capacity) {
        size_t new_capacity = set->capacity * 2;
        gm_link_t* new_links = realloc(set->links, new_capacity * sizeof(gm_link_t));
        if (!new_links) {
            gm_set_error("Memory allocation failed");
            return GM_ERR_MEMORY;
        }
        set->links = new_links;
        set->capacity = new_capacity;
    }
    
    // Copy link
    memcpy(&set->links[set->count], link, sizeof(gm_link_t));
    set->count++;
    
    return GM_OK;
}
