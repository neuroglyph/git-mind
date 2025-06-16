/* SPDX-License-Identifier: Apache-2.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

/**
 * @file gitmind_lib.h
 * @brief GitMind Library - Public API
 * 
 * This is the ONLY header you need to include to use GitMind as a library.
 * 
 * Example usage:
 * @code
 * // Create context with default Git backend
 * gm_context_t* ctx = gm_create_context(NULL);
 * 
 * // Create a link
 * int ret = gm_link_create(ctx, "README.md", "docs/api.md", "documents");
 * if (ret != GM_OK) {
 *     fprintf(stderr, "Error: %s\n", gm_last_error(ctx));
 * }
 * 
 * // List links
 * gm_link_set_t* links = NULL;
 * gm_link_list(ctx, &links, NULL, NULL);
 * for (size_t i = 0; i < links->count; i++) {
 *     printf("%s -> %s (%s)\n", 
 *            links->links[i].source,
 *            links->links[i].target,
 *            links->links[i].type);
 * }
 * gm_link_set_free(links);
 * 
 * // Cleanup
 * gm_destroy_context(ctx);
 * @endcode
 */

#ifndef GITMIND_LIB_H
#define GITMIND_LIB_H

#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Version information */
#define GITMIND_VERSION_MAJOR 0
#define GITMIND_VERSION_MINOR 1
#define GITMIND_VERSION_PATCH 0
#define GITMIND_VERSION_STRING "0.1.0"

/* Error codes */
typedef enum {
    GM_OK = 0,
    GM_ERR_NOT_REPO = -1,
    GM_ERR_NOT_FOUND = -2,
    GM_ERR_IO = -3,
    GM_ERR_GIT = -4,
    GM_ERR_MEMORY = -5,
    GM_ERR_INVALID_ARG = -6,
    GM_ERR_PATH_TOO_LONG = -7,
    GM_ERR_ALREADY_EXISTS = -8
} gm_error_t;

/* Opaque types */
typedef struct gm_context gm_context_t;

/* Link structure */
typedef struct gm_link {
    char source[4096];
    char target[4096];
    char type[64];
    time_t timestamp;
} gm_link_t;

/* Link set structure */
typedef struct gm_link_set {
    gm_link_t* links;
    size_t count;
    size_t capacity;
} gm_link_set_t;

/* Backend operations - for extensibility */
typedef struct gm_backend_ops {
    /* Repository operations */
    int (*open_repo)(void* backend_data, const char* path, void** out_handle);
    void (*close_repo)(void* backend_data, void* handle);
    
    /* Object operations */
    int (*hash_object)(void* backend_data, const void* data, size_t len, 
                       const char* type, char* out_sha);
    int (*read_object)(void* backend_data, const char* sha, void* out_data, 
                       size_t max_size, size_t* actual_size);
    
    /* Tree operations */
    int (*read_tree)(void* backend_data, const char* tree_sha, char* out_entries);
    int (*write_tree)(void* backend_data, const char* entries, char* out_sha);
    
    /* Reference operations */
    int (*read_ref)(void* backend_data, const char* ref_name, char* out_sha);
    int (*update_ref)(void* backend_data, const char* ref_name, 
                      const char* new_sha, const char* message);
    
    /* Commit operations */
    int (*create_commit)(void* backend_data, const char* tree_sha,
                        const char* parent_sha, const char* message, char* out_sha);
    int (*read_commit_tree)(void* backend_data, const char* commit_sha, char* out_tree_sha);
    
    /* Note operations */
    int (*write_note)(void* backend_data, const char* notes_ref, 
                      const char* object_sha, const char* note_content);
    int (*read_note)(void* backend_data, const char* notes_ref, 
                     const char* object_sha, char* out_content, size_t max_size);
    
    /* Backend-specific data */
    void* data;
} gm_backend_ops_t;

/* Output modes */
typedef enum {
    GM_OUTPUT_SILENT = 0,    /* Default: no output unless error */
    GM_OUTPUT_VERBOSE = 1,   /* Human-readable progress messages */
    GM_OUTPUT_PORCELAIN = 2  /* Machine-readable structured output */
} gm_output_mode_t;

/* Context management */
gm_context_t* gm_create_context(const gm_backend_ops_t* backend);
void gm_destroy_context(gm_context_t* ctx);
void gm_set_output_mode(gm_context_t* ctx, gm_output_mode_t mode);

/* Error handling */
const char* gm_last_error(const gm_context_t* ctx);
void gm_clear_error(gm_context_t* ctx);

/* Core operations */
int gm_init(gm_context_t* ctx, const char* repo_path);
int gm_link_create(gm_context_t* ctx, const char* source, 
                   const char* target, const char* type);
int gm_link_list(gm_context_t* ctx, gm_link_set_t** out_set,
                 const char* filter_source, const char* filter_target);
int gm_link_unlink(gm_context_t* ctx, const char* source, const char* target);

/* Link set operations */
gm_link_set_t* gm_link_set_new(void);
void gm_link_set_free(gm_link_set_t* set);
int gm_link_set_add(gm_link_set_t* set, const gm_link_t* link);

/* Graph operations */
int gm_traverse(gm_context_t* ctx, const char* start_file, int depth,
                void (*callback)(const gm_link_t* link, int level, void* userdata),
                void* userdata);

/* Status and maintenance */
int gm_status(gm_context_t* ctx, int* out_link_count, int* out_unique_files);
int gm_check(gm_context_t* ctx, int fix, int* out_broken_count);

/* Utility functions */
int gm_validate_link_path(const char* path);
const char* gm_version_string(void);

/* Default backend (libgit2) */
const gm_backend_ops_t* gm_backend_libgit2(void);

/* Test backend (for unit testing) */
const gm_backend_ops_t* gm_backend_test(void);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_LIB_H */