/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_EDGE_H
#define GITMIND_EDGE_H

#include "gitmind/result.h"
#include "gitmind/types.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file edge.h
 * @brief Edge operations for the git-mind knowledge graph
 *
 * This module provides functionality for creating, manipulating, and
 * serializing edges in the knowledge graph. Edges represent relationships
 * between git objects (typically file blobs).
 */

/* Forward declarations */
typedef struct gm_context gm_context_t;

/**
 * Edge structure representing a directed relationship between two git objects
 */
typedef struct gm_edge {
    uint8_t src_sha[GM_SHA1_SIZE];      /**< Source object SHA-1 */
    uint8_t tgt_sha[GM_SHA1_SIZE];      /**< Target object SHA-1 */
    uint16_t rel_type;                  /**< Relationship type */
    uint16_t confidence;                /**< Confidence (IEEE-754 half float) */
    uint64_t timestamp;                 /**< Creation time (Unix millis) */
    char src_path[GM_PATH_MAX];         /**< Source file path */
    char tgt_path[GM_PATH_MAX];         /**< Target file path */
    char ulid[GM_ULID_SIZE + 1];        /**< Unique identifier */
} gm_edge_t;

/* Define result type for edge operations */
GM_RESULT_DEF(gm_result_edge, gm_edge_t);

/**
 * Create an edge between two files
 *
 * Resolves paths to Git blob SHAs and generates a ULID for the edge.
 *
 * @param ctx Context with Git repository
 * @param src_path Source file path
 * @param tgt_path Target file path
 * @param rel_type Relationship type
 * @return Result containing edge on success, error on failure
 */
gm_result_edge_t gm_edge_create(gm_context_t *ctx, const char *src_path,
                                const char *tgt_path, gm_rel_type_t rel_type);

/**
 * Compare two edges for equality
 *
 * Edges are considered equal if they have the same source SHA, target SHA,
 * and relationship type.
 *
 * @param edge_a First edge
 * @param edge_b Second edge
 * @return true if edges are equal, false otherwise
 */
bool gm_edge_equal(const gm_edge_t *edge_a, const gm_edge_t *edge_b);

/**
 * Format an edge as a human-readable string
 *
 * @param edge Edge to format
 * @param[out] buffer Output buffer
 * @param len Buffer length
 * @return Result containing success or error
 */
gm_result_void_t gm_edge_format(const gm_edge_t *edge, char *buffer, size_t len);

/**
 * Encode edge to CBOR format
 *
 * @param edge Edge to encode
 * @param[out] buffer Output buffer
 * @param[in,out] len In: buffer size, Out: bytes written
 * @return Result containing success or error
 */
gm_result_void_t gm_edge_encode_cbor(const gm_edge_t *edge, uint8_t *buffer,
                                     size_t *len);

/**
 * Decode edge from CBOR format
 *
 * @param buffer CBOR data
 * @param len Buffer length
 * @return Result containing edge on success, error on failure
 */
gm_result_edge_t gm_edge_decode_cbor(const uint8_t *buffer, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_EDGE_H */
