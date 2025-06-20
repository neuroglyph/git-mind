/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_H
#define GITMIND_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "gitmind/attribution.h"
#include "gitmind/constants.h"
#include "gitmind/output.h"
#include "gitmind/io_ops.h"
#include "gitmind/time_ops.h"
#include "gitmind/random_ops.h"

/* Forward declarations */
typedef struct gm_context gm_context_t;
typedef struct gm_edge gm_edge_t;
typedef struct gm_journal gm_journal_t;
typedef struct gm_cache gm_cache_t;

/* Error codes */
#define GM_OK           0
#define GM_ERROR       -1
#define GM_NOT_FOUND   -2
#define GM_INVALID_ARG -3
#define GM_NO_MEMORY   -4
#define GM_IO_ERROR    -5

/* Constants */
#define GM_SHA1_SIZE   20
#define GM_SHA256_SIZE 32
#define GM_PATH_MAX    256
#define GM_ULID_SIZE   26

/* Relationship types */
typedef enum {
    GM_REL_IMPLEMENTS = 1,
    GM_REL_REFERENCES = 2,
    GM_REL_DEPENDS_ON = 3,
    GM_REL_AUGMENTS   = 4,
    GM_REL_CUSTOM     = 1000
} gm_rel_type_t;

/* Edge structure */
struct gm_edge {
    uint8_t  src_sha[GM_SHA1_SIZE];
    uint8_t  tgt_sha[GM_SHA1_SIZE];
    uint16_t rel_type;
    uint16_t confidence;      /* IEEE-754 half float */
    uint64_t timestamp;       /* Unix millis */
    char     src_path[GM_PATH_MAX];
    char     tgt_path[GM_PATH_MAX];
    char     ulid[GM_ULID_SIZE + 1];
};

/* Context for dependency injection */
struct gm_context {
    /* Git operations */
    struct {
        int (*resolve_blob)(void *repo, const char *path, uint8_t *sha);
        int (*create_commit)(void *repo, const char *ref, const void *data, size_t len);
        int (*read_commits)(void *repo, const char *ref, void *callback, void *userdata);
    } git_ops;
    
    /* Storage operations */
    struct {
        int (*write_cache)(void *handle, const void *data, size_t len);
        int (*read_cache)(void *handle, void *data, size_t len);
    } storage_ops;
    
    /* Logging */
    void (*log_fn)(int level, const char *fmt, ...);
    
    /* Output control */
    gm_output_t *output;
    
    /* I/O operations for dependency injection */
    const gm_io_ops_t *io_ops;
    
    /* Time operations for dependency injection */
    const gm_time_ops_t *time_ops;
    
    /* Random operations for dependency injection */
    const gm_random_ops_t *random_ops;
    
    /* User data */
    void *git_repo;
    void *storage_handle;
    void *user_data;
};

/* Journal operations */

/**
 * Append edges to the journal for the current branch.
 * 
 * @param ctx Context with Git repository
 * @param edges Array of edges to append
 * @param n_edges Number of edges in array
 * @return GM_OK on success, error code on failure
 */
int gm_journal_append(gm_context_t *ctx, const gm_edge_t *edges, size_t n_edges);

/**
 * Read edges from journal for a branch.
 * 
 * @param ctx Context with Git repository
 * @param branch Branch name (NULL for current)
 * @param callback Function called for each edge
 * @param userdata Passed to callback
 * @return GM_OK on success, GM_NOT_FOUND if no journal
 */
int gm_journal_read(gm_context_t *ctx, const char *branch, 
                    int (*callback)(const gm_edge_t *edge, void *userdata),
                    void *userdata);

/* Edge operations */

/**
 * Encode edge to CBOR format.
 * 
 * @param edge Edge to encode
 * @param buffer Output buffer
 * @param len In: buffer size, Out: bytes written
 * @return GM_OK on success, GM_INVALID_ARG if buffer too small
 */
int gm_edge_encode_cbor(const gm_edge_t *edge, uint8_t *buffer, size_t *len);

/**
 * Decode edge from CBOR format.
 * 
 * @param buffer CBOR data
 * @param len Buffer length
 * @param edge Output edge structure
 * @return GM_OK on success, GM_INVALID_ARG on malformed CBOR
 */
int gm_edge_decode_cbor(const uint8_t *buffer, size_t len, gm_edge_t *edge);

/* Cache operations */

/**
 * Rebuild the cache index for a branch.
 * 
 * Scans journal and builds bitmap indices for fast queries.
 * 
 * @param ctx Context with Git repository
 * @param branch Branch to rebuild cache for
 * @return GM_OK on success, error code on failure
 */
int gm_cache_rebuild(gm_context_t *ctx, const char *branch);

/**
 * Query edges by SHA (fan-out query).
 * 
 * @param ctx Context with Git repository
 * @param sha Source SHA to query
 * @param edges Output array (caller must free)
 * @param n_edges Number of edges found
 * @return GM_OK on success, GM_NOT_FOUND if SHA unknown
 */
int gm_cache_query(gm_context_t *ctx, const uint8_t *sha, 
                   gm_edge_t **edges, size_t *n_edges);

/* Edge creation and utilities */

/**
 * Create edge between two files.
 * 
 * Resolves paths to Git blob SHAs and generates ULID.
 * 
 * @param ctx Context with Git repository
 * @param src_path Source file path
 * @param tgt_path Target file path
 * @param rel_type Relationship type
 * @param edge Output edge structure
 * @return GM_OK on success, GM_NOT_FOUND if files missing
 */
int gm_edge_create(gm_context_t *ctx, const char *src_path, const char *tgt_path,
                   gm_rel_type_t rel_type, gm_edge_t *edge);

/**
 * Compare two edges for equality.
 * 
 * @param a First edge
 * @param b Second edge
 * @return 1 if equal, 0 if different
 */
int gm_edge_equal(const gm_edge_t *a, const gm_edge_t *b);

/**
 * Format edge as human-readable string.
 * 
 * @param edge Edge to format
 * @param buffer Output buffer
 * @param len Buffer size
 * @return Number of bytes written (excluding NUL)
 */
int gm_edge_format(const gm_edge_t *edge, char *buffer, size_t len);

/* Attributed edge operations */

/**
 * Create attributed edge with AI/human attribution.
 * 
 * @param ctx Context with Git repository
 * @param src_path Source file path
 * @param tgt_path Target file path  
 * @param rel_type Relationship type
 * @param confidence Confidence score as half-float
 * @param attribution Attribution metadata (source, author, etc.)
 * @param lane Semantic lane (default, architecture, etc.)
 * @param edge Output attributed edge
 * @return GM_OK on success, GM_NOT_FOUND if files missing
 */
int gm_edge_attributed_create(gm_context_t *ctx, const char *src_path, const char *tgt_path,
                             gm_rel_type_t rel_type, uint16_t confidence,
                             const gm_attribution_t *attribution, gm_lane_type_t lane,
                             gm_edge_attributed_t *edge);

/**
 * Format attributed edge as string (without attribution).
 * 
 * @param edge Edge to format
 * @param buffer Output buffer
 * @param len Buffer size
 * @return Number of bytes written
 */
int gm_edge_attributed_format(const gm_edge_attributed_t *edge, char *buffer, size_t len);

/**
 * Format attributed edge with full attribution details.
 * 
 * @param edge Edge to format
 * @param buffer Output buffer
 * @param len Buffer size
 * @return Number of bytes written
 */
int gm_edge_attributed_format_with_attribution(const gm_edge_attributed_t *edge, 
                                              char *buffer, size_t len);

/* Journal operations for attributed edges */

/**
 * Append attributed edges to journal.
 * 
 * @param ctx Context with Git repository
 * @param edges Array of attributed edges
 * @param n_edges Number of edges
 * @return GM_OK on success
 */
int gm_journal_append_attributed(gm_context_t *ctx, const gm_edge_attributed_t *edges, size_t n_edges);

/**
 * Read attributed edges from journal.
 * 
 * Handles both attributed and legacy edges (auto-converts).
 * 
 * @param ctx Context with Git repository
 * @param branch Branch name (NULL for current)
 * @param callback Called for each edge
 * @param userdata Passed to callback
 * @return GM_OK on success, GM_NOT_FOUND if no journal
 */
int gm_journal_read_attributed(gm_context_t *ctx, const char *branch,
                              int (*callback)(const gm_edge_attributed_t *edge, void *userdata),
                              void *userdata);

/* Confidence conversion utilities */

/**
 * Convert confidence value to IEEE 754 half-float.
 * 
 * @param confidence Float value (0.0 to 1.0)
 * @return Half-float representation
 */
uint16_t gm_confidence_to_half_float(float confidence);

/**
 * Convert IEEE 754 half-float to confidence value.
 * 
 * @param half_float Half-float value
 * @return Float value (0.0 to 1.0)
 */
float gm_confidence_from_half_float(uint16_t half_float);

/**
 * Parse confidence string to half-float.
 * 
 * @param str String representation (e.g., "0.85")
 * @param confidence Output half-float value
 * @return GM_OK on success, GM_INVALID_ARG on parse error
 */
int gm_confidence_parse(const char *str, uint16_t *confidence);

/* Utility functions */

/**
 * Generate a new ULID (Universally Unique Lexicographically Sortable ID).
 * 
 * @param ulid Output buffer (must be >= 27 bytes)
 * @return GM_OK on success
 */
int gm_ulid_generate(char *ulid);

/**
 * Get Git blob SHA for a file path.
 * 
 * @param ctx Context with Git repository
 * @param path File path in repository
 * @param sha Output SHA1 (20 bytes)
 * @return GM_OK on success, GM_NOT_FOUND if file not in Git
 */
int gm_sha_from_path(gm_context_t *ctx, const char *path, uint8_t *sha);

/**
 * Get human-readable error message.
 * 
 * @param error_code Error code from git-mind function
 * @return Static string describing error
 */
const char *gm_error_string(int error_code);

#endif /* GITMIND_H */