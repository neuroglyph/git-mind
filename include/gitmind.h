/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_H
#define GITMIND_H

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include "gitmind/attribution.h"
#include "gitmind/constants.h"

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
    
    /* User data */
    void *git_repo;
    void *storage_handle;
    void *user_data;
};

/* Journal operations */
int gm_journal_append(gm_context_t *ctx, const gm_edge_t *edges, size_t n_edges);
int gm_journal_read(gm_context_t *ctx, const char *branch, 
                    int (*callback)(const gm_edge_t *edge, void *userdata),
                    void *userdata);

/* Edge operations */
int gm_edge_encode_cbor(const gm_edge_t *edge, uint8_t *buffer, size_t *len);
int gm_edge_decode_cbor(const uint8_t *buffer, size_t len, gm_edge_t *edge);

/* Cache operations */
int gm_cache_rebuild(gm_context_t *ctx, const char *branch);
int gm_cache_query(gm_context_t *ctx, const uint8_t *sha, 
                   gm_edge_t **edges, size_t *n_edges);

/* Edge operations */
int gm_edge_create(gm_context_t *ctx, const char *src_path, const char *tgt_path,
                   gm_rel_type_t rel_type, gm_edge_t *edge);
int gm_edge_equal(const gm_edge_t *a, const gm_edge_t *b);
int gm_edge_format(const gm_edge_t *edge, char *buffer, size_t len);

/* Attributed edge operations */
int gm_edge_attributed_create(gm_context_t *ctx, const char *src_path, const char *tgt_path,
                             gm_rel_type_t rel_type, uint16_t confidence,
                             const gm_attribution_t *attribution, gm_lane_type_t lane,
                             gm_edge_attributed_t *edge);
int gm_edge_attributed_format(const gm_edge_attributed_t *edge, char *buffer, size_t len);
int gm_edge_attributed_format_with_attribution(const gm_edge_attributed_t *edge, 
                                              char *buffer, size_t len);

/* Journal operations for attributed edges */
int gm_journal_append_attributed(gm_context_t *ctx, const gm_edge_attributed_t *edges, size_t n_edges);
int gm_journal_read_attributed(gm_context_t *ctx, const char *branch,
                              int (*callback)(const gm_edge_attributed_t *edge, void *userdata),
                              void *userdata);

/* Confidence conversion utilities */
uint16_t gm_confidence_to_half_float(float confidence);
float gm_confidence_from_half_float(uint16_t half_float);
int gm_confidence_parse(const char *str, uint16_t *confidence);

/* Utility functions */
int gm_ulid_generate(char *ulid);
int gm_sha_from_path(gm_context_t *ctx, const char *path, uint8_t *sha);
const char *gm_error_string(int error_code);

#endif /* GITMIND_H */