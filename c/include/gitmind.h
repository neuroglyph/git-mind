// SPDX-License-Identifier: Apache-2.0
// © 2025 J. Kirby Ross / Neuroglyph Collective

#ifndef GITMIND_H
#define GITMIND_H

#include <stddef.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

// Version
#define GITMIND_VERSION_MAJOR 0
#define GITMIND_VERSION_MINOR 1
#define GITMIND_VERSION_PATCH 0

// Error codes
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

// Error message constants
#define ERR_MSG_NOT_REPO "Not a git repository: %s"
#define ERR_MSG_NOT_FOUND "Not found: %s"
#define ERR_MSG_IO_ERROR "I/O error: %s"
#define ERR_MSG_GIT_FAILED "Git operation failed: %s"
#define ERR_MSG_MEMORY "Memory allocation failed"
#define ERR_MSG_INVALID_ARG "Invalid argument: %s"
#define ERR_MSG_PATH_TOO_LONG "Path too long: %s"
#define ERR_MSG_ALREADY_EXISTS "Already exists: %s"
#define ERR_MSG_EMPTY_PATH "Empty path"
#define ERR_MSG_ABSOLUTE_PATH "Absolute paths not allowed in links: %s"
#define ERR_MSG_PATH_TRAVERSAL "Path traversal not allowed: %s"
#define ERR_MSG_LINK_NOT_FOUND "Link not found"
#define ERR_MSG_DIR_CREATE_FAILED "Failed to create directory %s: %s"
#define ERR_MSG_FILE_CREATE_FAILED "Failed to create file: %s"
#define ERR_MSG_FILE_WRITE_FAILED "Failed to write file"
#define ERR_MSG_LINK_CONTENT_TOO_LONG "Link content too long"
#define ERR_MSG_CWD_FAILED "Failed to get current directory"
#define ERR_MSG_NULL_POINTER "NULL pointer provided for %s"

// CLI error messages
#define ERR_MSG_LINK_REQUIRES_ARGS "Error: link requires source and target arguments\n"
#define ERR_MSG_UNLINK_REQUIRES_ARGS "Error: unlink requires source and target arguments\n"
#define ERR_MSG_UNKNOWN_COMMAND "Error: Unknown command '%s'\n"
#define ERR_MSG_MISSING_FILE_ARG "Error: Missing file argument\n"
#define ERR_MSG_DEPTH_OUT_OF_RANGE "Error: Depth must be between 1 and %d\n"

// Status messages
#define MSG_INIT_SUCCESS "Initialized git-mind in current repository\n"
#define MSG_LINK_CREATED "Created link: %s -> %s (%s)\n"
#define MSG_LINK_REMOVED "Removed link: %s -> %s\n"
#define MSG_NO_LINKS "No links found\n"
#define MSG_ALL_LINKS_VALID "All links are valid\n"
#define MSG_BROKEN_LINKS_FOUND "Found %d broken link%s\n"
#define MSG_BROKEN_LINKS_REMOVED "Removed %d broken link%s\n"
#define MSG_RUN_CHECK_FIX "Run 'git mind check --fix' to remove them\n"
#define MSG_NOT_INITIALIZED "git-mind: Not initialized (run 'git mind init')\n"

// Version info
#define MSG_VERSION_FORMAT "git-mind version %s\n"

// Porcelain output formats
#define PORCELAIN_INIT_OK "init:ok\n"
#define PORCELAIN_LINK_CREATED "link:created:%s:%s:%s\n"
#define PORCELAIN_LINK_REMOVED "link:removed:%s:%s\n"
#define PORCELAIN_LINK_FORMAT "link:%s:%s:%s:%ld\n"

// Human-readable formats
#define MSG_LINK_FORMAT "%s: %s -> %s (ts:%ld)\n"

// Buffer sizes - centralized for maintainability
#define GM_MAX_PATH 4096
#define GM_MAX_TYPE 64
#define GM_MAX_LINK_CONTENT 8192
#define GM_MAX_COMMAND 8192
#define GM_ERROR_BUFFER_SIZE 256

// SHA constants
#define GM_SHA256_SIZE 32
#define GM_SHA256_STRING_SIZE 65
#define GM_SHA1_SIZE 20
#define GM_SHA1_STRING_SIZE 41

// Default values
#define GM_DEFAULT_LINK_TYPE "REFERENCES"
#define GM_LINKS_DIR ".gitmind/links"
#define GM_LINK_EXTENSION ".gml"
#define GM_LINK_FORMAT_VERSION "1.0"

// File format constants
#define GM_FILE_MAGIC "GMv1"  // 4-byte magic header
#define GM_FILE_SEPARATOR "|"
#define GM_EDGE_MARKER "+"
#define GM_TOMBSTONE_MARKER "-"

// Git commands
#define GM_GIT_HASH_OBJECT "git hash-object \"%s\" 2>/dev/null"
#define GM_GIT_CONFIG_USER_NAME "git config user.name"
#define GM_GIT_ADD_FILE "git add %s 2>/dev/null"

// Default values
#define GM_DEFAULT_AUTHOR "unknown"
#define GM_DEFAULT_CONFIDENCE 1.0

// Orphan ref paths
#define GM_GRAPH_REF "refs/gitmind/graph"
#define GM_INBOUND_NOTES_REF "refs/notes/gitmind/inbound"
#define GM_TRAVERSAL_NOTES_REF "refs/notes/gitmind/traversal"

// Multi-edge constants
#define GM_MAX_EDGES_PER_FILE 100
#define GM_MAX_AUTHOR 128
#define GM_MAX_REASON 512

// Edge structure for multi-edge model
typedef struct {
    char type[GM_MAX_TYPE];
    char author[GM_MAX_AUTHOR];
    time_t timestamp;
    double confidence;
    int is_tombstone;  // Using int instead of bool for C99 compatibility
    char tombstone_reason[GM_MAX_REASON];
} gm_edge_t;

// Multi-edge link file structure
typedef struct {
    char source_sha[GM_SHA256_STRING_SIZE];  // Git blob SHA
    char target_sha[GM_SHA256_STRING_SIZE];  // Git blob SHA
    char source_path[GM_MAX_PATH];           // Path metadata
    char target_path[GM_MAX_PATH];           // Path metadata
    gm_edge_t edges[GM_MAX_EDGES_PER_FILE];
    size_t edge_count;
} gm_link_file_t;

// Legacy single-edge structure (for compatibility during migration)
typedef struct {
    char type[GM_MAX_TYPE];
    char source[GM_MAX_PATH];
    char target[GM_MAX_PATH];
    time_t timestamp;
} gm_link_t;

// Link collection (now represents edges across multiple files)
typedef struct {
    gm_link_t* links;
    size_t count;
    size_t capacity;
} gm_link_set_t;

// Repository handle
typedef struct gm_repo gm_repo;

// Traversal constants
#define GM_MAX_DEPTH 10
#define GM_DEFAULT_DEPTH 1

// Traversal formats
typedef enum {
    GM_FORMAT_TREE = 0,
    GM_FORMAT_LIST = 1
} gm_format_t;

// Traversal node
typedef struct {
    char path[GM_MAX_PATH];
    int depth;
    char parent[GM_MAX_PATH];
} gm_traverse_node_t;

// Traversal result
typedef struct {
    gm_traverse_node_t* nodes;
    size_t count;
    size_t capacity;
    int direct_count;
    int total_count;
} gm_traverse_result_t;

// Core operations
int gm_init(const char* repo_path);
int gm_link_create(const char* source, const char* target, const char* type);
int gm_link_list(gm_link_set_t** set, const char* filter_source, const char* filter_target);
int gm_link_unlink(const char* source, const char* target);
int gm_link_unlink_all(const char* source);
int gm_link_check(int fix, int* broken_count);
int gm_status(void);
int gm_traverse(const char* start_node, int depth, gm_format_t format, gm_traverse_result_t** result);

// Link set operations
gm_link_set_t* gm_link_set_new(void);
void gm_link_set_free(gm_link_set_t* set);
int gm_link_set_add(gm_link_set_t* set, const gm_link_t* link);

// Traverse result operations
gm_traverse_result_t* gm_traverse_result_new(void);
void gm_traverse_result_free(gm_traverse_result_t* result);
int gm_traverse_result_add(gm_traverse_result_t* result, const gm_traverse_node_t* node);
void gm_traverse_print_tree(const gm_traverse_result_t* result, const char* start_node);
void gm_traverse_print_list(const gm_traverse_result_t* result, const char* start_node);

// Error handling
const char* gm_last_error(void);
const char* gm_error_string(int error_code);

// Utilities
int gm_sha256_string(const char* content, char* out_sha);
int gm_sha1_string(const char* content, char* out_sha);
int gm_normalize_path(const char* path, char* out_normalized);
int gm_path_join(char* dest, size_t dest_size, const char* dir, const char* file);
int gm_validate_link_path(const char* path);

// Multi-edge operations
int gm_path_to_sha(const char* path, char* out_sha);
int gm_compute_link_filename(const char* source_sha, const char* target_sha, 
                           char* out_filename, size_t filename_size);
int gm_link_file_load(const char* filename, gm_link_file_t* link_file);
int gm_link_file_save(const char* filename, const gm_link_file_t* link_file);
int gm_link_file_merge(gm_link_file_t* dest, 
                      const gm_link_file_t* src1,
                      const gm_link_file_t* src2);

// Internal - not part of public API
void gm_set_error(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

// Version info
const char* gm_version_string(void);

// Orphan ref operations
int gm_orphan_ref_exists(void);
int gm_orphan_ref_create(void);
int gm_get_graph_tree(char* out_tree_sha);
int gm_update_graph_ref(const char* new_tree_sha, const char* message);

// ULID operations
#define GM_ULID_SIZE 27  // 26 chars + null terminator
int gm_ulid_generate(char* out_ulid);
int gm_ulid_timestamp(const char* ulid, time_t* out_timestamp);

// CBOR operations
int gm_cbor_encode_edge(const char* target_sha, float confidence, 
                       time_t timestamp, unsigned char* out_buf, 
                       size_t* out_len, size_t max_len);
int gm_cbor_decode_edge(const unsigned char* cbor_data, size_t data_len,
                       char* out_target_sha, float* out_confidence,
                       time_t* out_timestamp);

// Fan-out operations
void gm_compute_fanout(const char* sha, char* out_fanout, size_t fanout_size);
int gm_compute_rel_hash(const char* rel_type, char* out_hash);
int gm_build_edge_path(const char* src_sha, const char* rel_type, 
                      const char* edge_id, char* out_path, size_t path_size);
int gm_update_tree_with_blob(const char* tree_sha, const char* path,
                           const char* blob_sha, char* out_new_tree);
int gm_build_edge_tree(const char* edge_path, const char* edge_blob_sha,
                      const char* current_tree_sha, char* out_tree_sha);

// Shell-safe command execution
int gm_exec_git_command(const char* git_args[], char* output, size_t output_size);
int gm_git_hash_object(const void* data, size_t size, const char* type, char* out_sha);
int gm_git_cat_file_blob(const char* sha, void* output, size_t max_size, size_t* actual_size);

#ifdef __cplusplus
}
#endif

#endif // GITMIND_H
