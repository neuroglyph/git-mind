/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_EDGE_ATTRIBUTED_H
#define GITMIND_EDGE_ATTRIBUTED_H

#include "gitmind/attribution.h"
#include "gitmind/edge.h"
#include "gitmind/result.h"
#include "gitmind/types.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file edge_attributed.h
 * @brief Attributed edge operations for the git-mind knowledge graph
 *
 * This module extends basic edges with attribution metadata, including
 * author information, source type (human/AI), and confidence scores.
 */

/**
 * Lane types for edge organization
 */
typedef enum {
    GM_LANE_PRIMARY = 0,    /**< Primary knowledge lane */
    GM_LANE_EXPLORATION = 1,/**< Exploratory connections */
    GM_LANE_REVIEW = 2,     /**< Under review */
    GM_LANE_ARCHIVED = 3    /**< Archived/deprecated */
} gm_lane_type_t;

/**
 * Attributed edge structure with full metadata
 */
typedef struct gm_edge_attributed {
    /* Base edge fields */
    uint8_t src_sha[GM_SHA1_SIZE];      /**< Source object SHA-1 (legacy) */
    uint8_t tgt_sha[GM_SHA1_SIZE];      /**< Target object SHA-1 (legacy) */
    gm_oid_t src_oid;                   /**< Source object OID (preferred) */
    gm_oid_t tgt_oid;                   /**< Target object OID (preferred) */
    uint16_t rel_type;                  /**< Relationship type */
    uint16_t confidence;                /**< Confidence (IEEE-754 half float) */
    uint64_t timestamp;                 /**< Creation time (Unix millis) */
    char src_path[GM_PATH_MAX];         /**< Source file path */
    char tgt_path[GM_PATH_MAX];         /**< Target file path */
    char ulid[GM_ULID_SIZE + 1];        /**< Unique identifier */
    
    /* Attribution fields */
    gm_attribution_t attribution;       /**< Author and source metadata */
    gm_lane_type_t lane;               /**< Knowledge lane */
} gm_edge_attributed_t;

/* Define result types for attributed edge operations */
GM_RESULT_DEF(gm_result_edge_attributed, gm_edge_attributed_t);
GM_RESULT_DEF(gm_result_uint16, uint16_t);

/**
 * Convert float confidence to IEEE 754 half-float representation
 *
 * @param confidence Confidence value (0.0 to 1.0)
 * @return Half-float representation
 */
uint16_t gm_confidence_to_half_float(float confidence);

/**
 * Convert IEEE 754 half-float to regular float
 *
 * @param half_float Half-float value
 * @return Confidence value (0.0 to 1.0)
 */
float gm_confidence_from_half_float(uint16_t half_float);

/**
 * Parse confidence string to half-float
 *
 * @param str String representation of confidence (e.g., "0.95")
 * @return Result containing half-float value on success, error on failure
 */
gm_result_uint16_t gm_confidence_parse(const char *str);

/**
 * Create an attributed edge with full metadata
 *
 * @param ctx Context with Git repository
 * @param src_path Source file path
 * @param tgt_path Target file path
 * @param rel_type Relationship type
 * @param confidence Confidence score (half-float)
 * @param attribution Attribution metadata
 * @param lane Knowledge lane
 * @return Result containing edge on success, error on failure
 */
gm_result_edge_attributed_t gm_edge_attributed_create(
    gm_context_t *ctx, const char *src_path, const char *tgt_path,
    gm_rel_type_t relationship_type, uint16_t confidence_value,
    const gm_attribution_t *attribution, gm_lane_type_t lane);

/**
 * Format attributed edge without attribution info (legacy format)
 *
 * @param edge Edge to format
 * @param[out] buffer Output buffer
 * @param len Buffer length
 * @return Result containing success or error
 */
gm_result_void_t gm_edge_attributed_format(const gm_edge_attributed_t *edge,
                                          char *buffer, size_t len);

/**
 * Format attributed edge with full attribution information
 *
 * @param edge Edge to format
 * @param[out] buffer Output buffer
 * @param len Buffer length
 * @return Result containing success or error
 */
gm_result_void_t gm_edge_attributed_format_with_attribution(
    const gm_edge_attributed_t *edge, char *buffer, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_EDGE_ATTRIBUTED_H */
