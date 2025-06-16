/* SPDX-License-Identifier: Apache-2.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

/**
 * @file gitmind_internal.h
 * @brief Internal implementation details - NOT part of public API
 */

#ifndef GITMIND_INTERNAL_H
#define GITMIND_INTERNAL_H

#include "gitmind_lib.h"
#include <stdio.h>

/* Internal constants */
#define GM_SHA1_STRING_SIZE 41
#define GM_SHA256_STRING_SIZE 65
#define GM_ULID_SIZE 27
#define GM_CBOR_BUFFER_SIZE 256
#define GM_LINE_BUFFER_SIZE 1024
#define GM_MAX_COMMAND 8192
#define GM_ERROR_BUFFER_SIZE 256
#define GM_FANOUT_SIZE 6
#define GM_FANOUT_PREFIX_SIZE 2
#define GM_REL_HASH_SIZE 8
#define GM_REL_HASH_BUFFER_SIZE 9
#define GM_SHA_HEX_SIZE 40
#define GM_MAX_TYPE 64

/* Git object modes */
#define GM_GIT_MODE_BLOB "100644"
#define GM_GIT_MODE_TREE "040000"

/* Git refs */
#define GM_GRAPH_REF "refs/gitmind/graph"
#define GM_NOTES_PATH_REF "refs/notes/gitmind/paths"
#define GM_NOTES_TYPES_REF "refs/notes/gitmind/types"

/* Message formats */
#define GM_EDGE_MESSAGE_FORMAT "Add edge: %s -[%s]-> %s"

/* Context implementation */
struct gm_context {
    /* Backend operations */
    const gm_backend_ops_t* backend;
    
    /* Repository handle (backend-specific) */
    void* repo_handle;
    
    /* Error state */
    char error_buffer[GM_ERROR_BUFFER_SIZE];
    gm_error_t last_error;
    
    /* Configuration */
    struct {
        int initialized;
        char repo_path[4096];
    } config;
    
    /* Output mode */
    gm_output_mode_t output_mode;
};

/* Internal functions */

/* Error handling */
void gm_set_error_ctx(gm_context_t* ctx, gm_error_t code, const char* fmt, ...);

/* Output handling - respects context output mode */
void gm_output_verbose(gm_context_t* ctx, const char* fmt, ...);
void gm_output_porcelain(gm_context_t* ctx, const char* fmt, ...);

/* SHA operations */
int gm_sha1_string(const char* input, char* out_sha);
int gm_sha256_string(const char* input, char* out_sha);

/* ULID operations */
int gm_ulid_generate(char* out_ulid);
int gm_ulid_timestamp(const char* ulid, time_t* out_timestamp);

/* CBOR operations */
int gm_cbor_encode_edge(const char* target_sha, float confidence, 
                       time_t timestamp, unsigned char* out_buf, 
                       size_t* out_len, size_t max_len);
int gm_cbor_decode_edge(const unsigned char* cbor_data, size_t data_len,
                       char* out_target_sha, float* out_confidence,
                       time_t* out_timestamp);

/* Fan-out operations */
void gm_compute_fanout(const char* sha, char* out_fanout, size_t fanout_size);
int gm_compute_rel_hash(const char* rel_type, char* out_hash);
int gm_build_edge_path(const char* src_sha, const char* rel_type, 
                      const char* edge_id, char* out_path, size_t path_size);

/* Tree operations */
int gm_merge_tree_path(const char* base_tree, const char* path, 
                      const char* entry_mode, const char* entry_sha,
                      char* out_tree);

/* Orphan ref operations */
int gm_orphan_ref_exists_ctx(gm_context_t* ctx);
int gm_orphan_ref_create_ctx(gm_context_t* ctx);
int gm_get_graph_tree_ctx(gm_context_t* ctx, char* out_tree_sha);
int gm_update_graph_ref_ctx(gm_context_t* ctx, const char* new_tree_sha, 
                           const char* message);

/* Path resolution */
int gm_find_path_by_sha(gm_context_t* ctx, const char* blob_sha, 
                       char* out_path, size_t path_size);

#endif /* GITMIND_INTERNAL_H */