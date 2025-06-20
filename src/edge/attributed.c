/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind.h"
#include "gitmind/constants_internal.h"
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
    
    /* Get timestamp using context if available */
    if (ctx && ctx->time_ops && ctx->time_ops->time) {
        edge->timestamp = (uint64_t)ctx->time_ops->time(NULL) * MILLIS_PER_SECOND;
    } else {
        /* Fallback to direct call */
        edge->timestamp = (uint64_t)time(NULL) * MILLIS_PER_SECOND;
    }
    
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

/* Get relationship type arrow string */
static const char* get_rel_arrow_string(gm_rel_type_t rel_type) {
    switch (rel_type) {
        case GM_REL_IMPLEMENTS:
            return GM_STR_IMPLEMENTS;
        case GM_REL_REFERENCES:
            return GM_STR_REFERENCES;
        case GM_REL_DEPENDS_ON:
            return GM_STR_DEPENDS_ON;
        case GM_REL_AUGMENTS:
            return GM_STR_AUGMENTS;
        default:
            return GM_STR_CUSTOM;
    }
}

/**
 * Format attributed edge without attribution info (legacy format)
 */
int gm_edge_attributed_format(const gm_edge_attributed_t *edge, char *buffer, size_t len) {
    if (!edge || !buffer || len < GM_FORMAT_BUFFER_SIZE) {
        return GM_INVALID_ARG;
    }
    
    const char *rel_str = get_rel_arrow_string(edge->rel_type);
    
    snprintf(buffer, len, "%s ──%s──> %s", 
             edge->src_path, rel_str, edge->tgt_path);
    
    return GM_OK;
}

/* Get source type string */
static const char* get_source_type_string(gm_source_type_t source_type) {
    switch (source_type) {
        case GM_SOURCE_HUMAN:
            return GM_ENV_VAL_HUMAN;
        case GM_SOURCE_AI_CLAUDE:
            return GM_ENV_VAL_CLAUDE;
        case GM_SOURCE_AI_GPT:
            return GM_ENV_VAL_GPT;
        case GM_SOURCE_SYSTEM:
            return GM_ENV_VAL_SYSTEM;
        default:
            return "unknown";
    }
}

/* Format attribution suffix */
static void format_attribution_suffix(const gm_edge_attributed_t *edge,
                                     const char *source_str,
                                     char *buffer, size_t len) {
    if (edge->attribution.source_type == GM_SOURCE_HUMAN) {
        /* For humans, don't show confidence (always 1.0) */
        snprintf(buffer, len, " [%s: %s]", 
                 source_str, edge->attribution.author);
    } else {
        /* For AI, show confidence */
        float confidence = gm_confidence_from_half_float(edge->confidence);
        snprintf(buffer, len, " [%s: %s, conf: %.2f]", 
                 source_str, edge->attribution.author, confidence);
    }
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
    
    /* Get source type string */
    const char *source_str = get_source_type_string(edge->attribution.source_type);
    
    /* Format with attribution */
    char suffix[GM_FORMAT_BUFFER_SIZE];
    format_attribution_suffix(edge, source_str, suffix, sizeof(suffix));
    
    snprintf(buffer, len, "%s%s", basic_format, suffix);
    
    return GM_OK;
}