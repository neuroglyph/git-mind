/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/edge_attributed.h"
#include "gitmind/constants.h"
#include "gitmind/context.h"
#include "gitmind/error.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Result types are defined in edge_attributed.h */

/* Constants for confidence conversion */
#define CONFIDENCE_SCALE 0x3C00 /* 1.0 in IEEE-754 half-float */

/**
 * Get current timestamp in milliseconds
 */
static uint64_t get_timestamp_millis(gm_context_t *ctx) {
    if (ctx && ctx->time_ops && ctx->time_ops->time) {
        return (uint64_t)ctx->time_ops->time(NULL) * MILLIS_PER_SECOND;
    }
    return (uint64_t)time(NULL) * MILLIS_PER_SECOND;
}

/**
 * Resolve SHA for a path using context operations
 */
static gm_result_void_t resolve_sha(gm_context_t *ctx, const char *path,
                                    uint8_t *sha) {
    if (!ctx || !ctx->git_ops.resolve_blob) {
        return gm_err_void(GM_ERROR(GM_ERR_NOT_IMPLEMENTED,
                                    "Git operations not available"));
    }
    
    int result = ctx->git_ops.resolve_blob(ctx->git_repo, path, sha);
    if (result != 0) {
        return gm_err_void(GM_ERROR(GM_ERR_NOT_FOUND,
                                    "Failed to resolve blob SHA"));
    }
    
    return gm_ok_void();
}

/**
 * Convert float confidence to IEEE 754 half-float representation
 */
uint16_t gm_confidence_to_half_float(float confidence) {
    /* Clamp to valid range */
    if (confidence < GM_CONFIDENCE_MIN) {
        confidence = GM_CONFIDENCE_MIN;
    }
    if (confidence > GM_CONFIDENCE_MAX) {
        confidence = GM_CONFIDENCE_MAX;
    }
    
    /* Simple conversion: scale to half-float representation */
    return (uint16_t)(confidence * (float)CONFIDENCE_SCALE);
}

/**
 * Convert IEEE 754 half-float to regular float
 */
float gm_confidence_from_half_float(uint16_t half_float) {
    return (float)half_float / (float)CONFIDENCE_SCALE;
}

/**
 * Parse confidence string to half-float
 */
gm_result_uint16_t gm_confidence_parse(const char *str) {
    if (!str) {
        return (gm_result_uint16_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERR_INVALID_ARGUMENT, "Null confidence string")
        };
    }
    
    char *endptr;
    float val = strtof(str, &endptr);
    
    /* Check for parsing errors */
    if (endptr == str || *endptr != '\0') {
        return (gm_result_uint16_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERR_INVALID_ARGUMENT, "Invalid confidence format")
        };
    }
    
    /* Check range */
    if (val < GM_CONFIDENCE_MIN || val > GM_CONFIDENCE_MAX) {
        return (gm_result_uint16_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERR_INVALID_ARGUMENT, "Confidence out of range")
        };
    }
    
    uint16_t confidence = gm_confidence_to_half_float(val);
    return (gm_result_uint16_t){.ok = true, .u.val = confidence};
}

/**
 * Create an attributed edge with full metadata
 */
gm_result_edge_attributed_t gm_edge_attributed_create(
    gm_context_t *ctx, const char *src_path, const char *tgt_path,
    gm_rel_type_t rel_type, uint16_t confidence,
    const gm_attribution_t *attribution, gm_lane_type_t lane) {
    
    /* Validate arguments */
    if (!ctx || !src_path || !tgt_path || !attribution) {
        return (gm_result_edge_attributed_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERR_INVALID_ARGUMENT, "Invalid arguments")
        };
    }
    
    /* Check path lengths */
    if (strlen(src_path) >= GM_PATH_MAX || strlen(tgt_path) >= GM_PATH_MAX) {
        return (gm_result_edge_attributed_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERR_INVALID_ARGUMENT, "Path too long")
        };
    }
    
    /* Initialize edge */
    gm_edge_attributed_t edge;
    memset(&edge, 0, sizeof(edge));
    
    /* Resolve source SHA */
    gm_result_void_t src_result = resolve_sha(ctx, src_path, edge.src_sha);
    if (!src_result.ok) {
        return (gm_result_edge_attributed_t){.ok = false, .u.err = src_result.u.err};
    }
    
    /* Resolve target SHA */
    gm_result_void_t tgt_result = resolve_sha(ctx, tgt_path, edge.tgt_sha);
    if (!tgt_result.ok) {
        return (gm_result_edge_attributed_t){.ok = false, .u.err = tgt_result.u.err};
    }
    
    /* Set basic fields */
    edge.rel_type = (uint16_t)rel_type;
    edge.confidence = confidence;
    edge.timestamp = get_timestamp_millis(ctx);
    
    /* Copy paths */
    strncpy(edge.src_path, src_path, GM_PATH_MAX - 1);
    edge.src_path[GM_PATH_MAX - 1] = '\0';
    
    strncpy(edge.tgt_path, tgt_path, GM_PATH_MAX - 1);
    edge.tgt_path[GM_PATH_MAX - 1] = '\0';
    
    /* Generate ULID */
    gm_result_ulid_t ulid_result = gm_ulid_generate(edge.ulid);
    if (!ulid_result.ok) {
        return (gm_result_edge_attributed_t){.ok = false, .u.err = ulid_result.u.err};
    }
    
    /* Copy attribution and lane */
    memcpy(&edge.attribution, attribution, sizeof(gm_attribution_t));
    edge.lane = lane;
    
    return (gm_result_edge_attributed_t){.ok = true, .u.val = edge};
}

/**
 * Get relationship type arrow string
 */
static const char *get_rel_arrow_string(uint16_t rel_type) {
    switch ((gm_rel_type_t)rel_type) {
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
gm_result_void_t gm_edge_attributed_format(const gm_edge_attributed_t *edge,
                                          char *buffer, size_t len) {
    if (!edge || !buffer || len < GM_FORMAT_BUFFER_SIZE) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "Invalid arguments"));
    }
    
    const char *rel_str = get_rel_arrow_string(edge->rel_type);
    
    int written = snprintf(buffer, len, "%s ──%s──> %s",
                          edge->src_path, rel_str, edge->tgt_path);
    
    if (written < 0 || (size_t)written >= len) {
        return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL,
                                    "Buffer too small for formatted edge"));
    }
    
    return gm_ok_void();
}

/**
 * Get source type string
 */
static const char *get_source_type_string(gm_source_type_t source_type) {
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

/**
 * Format attributed edge with full attribution information
 */
gm_result_void_t gm_edge_attributed_format_with_attribution(
    const gm_edge_attributed_t *edge, char *buffer, size_t len) {
    
    if (!edge || !buffer || len < GM_FORMAT_BUFFER_SIZE) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "Invalid arguments"));
    }
    
    /* Format basic edge first */
    char basic_format[GM_FORMAT_BUFFER_SIZE];
    gm_result_void_t base_result = gm_edge_attributed_format(edge, basic_format,
                                                             sizeof(basic_format));
    if (!base_result.ok) {
        return base_result;
    }
    
    /* Get source type string */
    const char *source_str = get_source_type_string(edge->attribution.source_type);
    
    /* Format with attribution */
    int written;
    if (edge->attribution.source_type == GM_SOURCE_HUMAN) {
        /* For humans, don't show confidence (always 1.0) */
        written = snprintf(buffer, len, "%s [%s: %s]",
                          basic_format, source_str, edge->attribution.author);
    } else {
        /* For AI, show confidence */
        float confidence = gm_confidence_from_half_float(edge->confidence);
        written = snprintf(buffer, len, "%s [%s: %s, conf: %.2f]",
                          basic_format, source_str, edge->attribution.author,
                          confidence);
    }
    
    if (written < 0 || (size_t)written >= len) {
        return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL,
                                    "Buffer too small for formatted edge"));
    }
    
    return gm_ok_void();
}
