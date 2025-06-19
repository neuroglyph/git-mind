/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

/**
 * Convert float confidence to IEEE 754 half-float representation
 */
uint16_t gm_confidence_to_half_float(float confidence) {
    /* Clamp to valid range */
    if (confidence < GM_CONFIDENCE_MIN) confidence = GM_CONFIDENCE_MIN;
    if (confidence > GM_CONFIDENCE_MAX) confidence = GM_CONFIDENCE_MAX;
    
    /* Simple conversion: scale to 0x3C00 (1.0 in half-float) */
    return (uint16_t)(confidence * GM_CONFIDENCE_SCALE);
}

/**
 * Convert IEEE 754 half-float to regular float
 */
float gm_confidence_from_half_float(uint16_t half_float) {
    return (float)half_float / GM_CONFIDENCE_SCALE;
}

/**
 * Parse confidence string to half-float
 */
int gm_confidence_parse(const char *str, uint16_t *confidence) {
    if (!str || !confidence) {
        return GM_INVALID_ARG;
    }
    
    char *endptr;
    float val = strtof(str, &endptr);
    
    /* Check for parsing errors */
    if (endptr == str || *endptr != '\0') {
        return GM_INVALID_ARG;
    }
    
    /* Check range */
    if (val < GM_CONFIDENCE_MIN || val > GM_CONFIDENCE_MAX) {
        return GM_INVALID_ARG;
    }
    
    *confidence = gm_confidence_to_half_float(val);
    return GM_OK;
}

/**
 * Create an attributed edge from paths and metadata
 */
int gm_edge_attributed_create(gm_context_t *ctx, const char *src_path, const char *tgt_path,
                             gm_rel_type_t rel_type, uint16_t confidence,
                             const gm_attribution_t *attribution, gm_lane_type_t lane,
                             gm_edge_attributed_t *edge) {
    if (!ctx || !src_path || !tgt_path || !attribution || !edge) {
        return GM_INVALID_ARG;
    }
    
    /* Clear the edge structure */
    memset(edge, 0, sizeof(gm_edge_attributed_t));
    
    /* Resolve blob SHAs */
    int result = gm_sha_from_path(ctx, src_path, edge->src_sha);
    if (result != GM_OK) {
        return result;
    }
    
    result = gm_sha_from_path(ctx, tgt_path, edge->tgt_sha);
    if (result != GM_OK) {
        return result;
    }
    
    /* Set basic edge properties */
    edge->rel_type = rel_type;
    edge->confidence = confidence;
    edge->timestamp = (uint64_t)time(NULL) * 1000; /* Unix millis */
    
    /* Copy paths */
    strncpy(edge->src_path, src_path, sizeof(edge->src_path) - 1);
    strncpy(edge->tgt_path, tgt_path, sizeof(edge->tgt_path) - 1);
    
    /* Generate ULID */
    result = gm_ulid_generate(edge->ulid);
    if (result != GM_OK) {
        return result;
    }
    
    /* Copy attribution */
    memcpy(&edge->attribution, attribution, sizeof(gm_attribution_t));
    edge->lane = lane;
    
    return GM_OK;
}

/**
 * Format attributed edge without attribution info (legacy format)
 */
int gm_edge_attributed_format(const gm_edge_attributed_t *edge, char *buffer, size_t len) {
    if (!edge || !buffer || len < GM_FORMAT_BUFFER_SIZE) {
        return GM_INVALID_ARG;
    }
    
    const char *rel_str;
    switch (edge->rel_type) {
        case GM_REL_IMPLEMENTS:
            rel_str = GM_STR_IMPLEMENTS;
            break;
        case GM_REL_REFERENCES:
            rel_str = GM_STR_REFERENCES;
            break;
        case GM_REL_DEPENDS_ON:
            rel_str = GM_STR_DEPENDS_ON;
            break;
        case GM_REL_AUGMENTS:
            rel_str = GM_STR_AUGMENTS;
            break;
        default:
            rel_str = GM_STR_CUSTOM;
            break;
    }
    
    snprintf(buffer, len, "%s ──%s──> %s", 
             edge->src_path, rel_str, edge->tgt_path);
    
    return GM_OK;
}

/**
 * Format attributed edge with attribution information
 */
int gm_edge_attributed_format_with_attribution(const gm_edge_attributed_t *edge, 
                                              char *buffer, size_t len) {
    if (!edge || !buffer || len < GM_FORMAT_BUFFER_SIZE) {
        return GM_INVALID_ARG;
    }
    
    /* Format basic edge first */
    char basic_format[GM_FORMAT_BUFFER_SIZE];
    int result = gm_edge_attributed_format(edge, basic_format, sizeof(basic_format));
    if (result != GM_OK) {
        return result;
    }
    
    /* Add attribution info */
    const char *source_str;
    switch (edge->attribution.source_type) {
        case GM_SOURCE_HUMAN:
            source_str = GM_ENV_VAL_HUMAN;
            break;
        case GM_SOURCE_AI_CLAUDE:
            source_str = GM_ENV_VAL_CLAUDE;
            break;
        case GM_SOURCE_AI_GPT:
            source_str = GM_ENV_VAL_GPT;
            break;
        case GM_SOURCE_SYSTEM:
            source_str = GM_ENV_VAL_SYSTEM;
            break;
        default:
            source_str = "unknown";
            break;
    }
    
    float confidence = gm_confidence_from_half_float(edge->confidence);
    
    if (edge->attribution.source_type == GM_SOURCE_HUMAN) {
        /* For humans, don't show confidence (always 1.0) */
        snprintf(buffer, len, "%s [%s: %s]", 
                 basic_format, source_str, edge->attribution.author);
    } else {
        /* For AI, show confidence */
        snprintf(buffer, len, "%s [%s: %s, conf: %.2f]", 
                 basic_format, source_str, edge->attribution.author, confidence);
    }
    
    return GM_OK;
}