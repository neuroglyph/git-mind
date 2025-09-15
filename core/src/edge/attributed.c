/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/edge_attributed.h"
#include "gitmind/constants.h"
#include "gitmind/context.h"
#include "gitmind/error.h"
#include "gitmind/types/ulid.h"
#include "gitmind/security/string.h"
#include "gitmind/result.h"
#include "gitmind/types.h"
#include "gitmind/attribution.h"
#include <stdint.h>
#include "gitmind/security/memory.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <git2/oid.h>
#include "gitmind/cbor/cbor.h"
#include "gitmind/cbor/keys.h"

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

/* === CBOR encode/decode === */

gm_result_void_t gm_edge_attributed_encode_cbor(const gm_edge_attributed_t *e,
                                                uint8_t *buffer, size_t *len) {
    if (!e || !buffer || !len) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "Invalid arguments"));
    }
    size_t offset = 0;
    size_t avail = *len;
    if (avail < 1) return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL, "Buffer too small"));
    buffer[offset++] = (uint8_t)(0xA0 | (GM_CBOR_ATTR_EDGE_FIELDS_TOTAL & 0x1F));

    /* helper lambdas not allowed; use inline blocks */
    gm_result_size_t r;
    /* Legacy SHA fields */
    r = gm_cbor_write_uint(GM_CBOR_KEY_SRC_SHA, buffer + offset, avail - offset);
    if (!r.ok) return gm_err_void(r.u.err); offset += r.u.val;
    r = gm_cbor_write_bytes(buffer + offset, avail - offset, e->src_sha, GM_SHA1_SIZE);
    if (!r.ok) return gm_err_void(r.u.err); offset += r.u.val;

    r = gm_cbor_write_uint(GM_CBOR_KEY_TGT_SHA, buffer + offset, avail - offset);
    if (!r.ok) return gm_err_void(r.u.err); offset += r.u.val;
    r = gm_cbor_write_bytes(buffer + offset, avail - offset, e->tgt_sha, GM_SHA1_SIZE);
    if (!r.ok) return gm_err_void(r.u.err); offset += r.u.val;

    /* Numerics */
    r = gm_cbor_write_uint(GM_CBOR_KEY_REL_TYPE, buffer + offset, avail - offset);
    if (!r.ok) return gm_err_void(r.u.err); offset += r.u.val;
    r = gm_cbor_write_uint(e->rel_type, buffer + offset, avail - offset);
    if (!r.ok) return gm_err_void(r.u.err); offset += r.u.val;

    r = gm_cbor_write_uint(GM_CBOR_KEY_CONFIDENCE, buffer + offset, avail - offset);
    if (!r.ok) return gm_err_void(r.u.err); offset += r.u.val;
    r = gm_cbor_write_uint(e->confidence, buffer + offset, avail - offset);
    if (!r.ok) return gm_err_void(r.u.err); offset += r.u.val;

    r = gm_cbor_write_uint(GM_CBOR_KEY_TIMESTAMP, buffer + offset, avail - offset);
    if (!r.ok) return gm_err_void(r.u.err); offset += r.u.val;
    r = gm_cbor_write_uint(e->timestamp, buffer + offset, avail - offset);
    if (!r.ok) return gm_err_void(r.u.err); offset += r.u.val;

    /* Text */
    r = gm_cbor_write_uint(GM_CBOR_KEY_SRC_PATH, buffer + offset, avail - offset);
    if (!r.ok) return gm_err_void(r.u.err); offset += r.u.val;
    r = gm_cbor_write_text(buffer + offset, avail - offset, e->src_path);
    if (!r.ok) return gm_err_void(r.u.err); offset += r.u.val;

    r = gm_cbor_write_uint(GM_CBOR_KEY_TGT_PATH, buffer + offset, avail - offset);
    if (!r.ok) return gm_err_void(r.u.err); offset += r.u.val;
    r = gm_cbor_write_text(buffer + offset, avail - offset, e->tgt_path);
    if (!r.ok) return gm_err_void(r.u.err); offset += r.u.val;

    r = gm_cbor_write_uint(GM_CBOR_KEY_ULID, buffer + offset, avail - offset);
    if (!r.ok) return gm_err_void(r.u.err); offset += r.u.val;
    r = gm_cbor_write_text(buffer + offset, avail - offset, e->ulid);
    if (!r.ok) return gm_err_void(r.u.err); offset += r.u.val;

    /* OIDs */
    const uint8_t *src_raw = git_oid_raw(&e->src_oid);
    const uint8_t *tgt_raw = git_oid_raw(&e->tgt_oid);
    r = gm_cbor_write_uint(GM_CBOR_KEY_SRC_OID, buffer + offset, avail - offset);
    if (!r.ok) return gm_err_void(r.u.err); offset += r.u.val;
    r = gm_cbor_write_bytes(buffer + offset, avail - offset, src_raw ? src_raw : e->src_sha, GM_OID_RAWSZ);
    if (!r.ok) return gm_err_void(r.u.err); offset += r.u.val;

    r = gm_cbor_write_uint(GM_CBOR_KEY_TGT_OID, buffer + offset, avail - offset);
    if (!r.ok) return gm_err_void(r.u.err); offset += r.u.val;
    r = gm_cbor_write_bytes(buffer + offset, avail - offset, tgt_raw ? tgt_raw : e->tgt_sha, GM_OID_RAWSZ);
    if (!r.ok) return gm_err_void(r.u.err); offset += r.u.val;

    /* Attribution */
    r = gm_cbor_write_uint(GM_CBOR_KEY_SOURCE_TYPE, buffer + offset, avail - offset);
    if (!r.ok) return gm_err_void(r.u.err); offset += r.u.val;
    r = gm_cbor_write_uint((uint64_t)e->attribution.source_type, buffer + offset, avail - offset);
    if (!r.ok) return gm_err_void(r.u.err); offset += r.u.val;

    r = gm_cbor_write_uint(GM_CBOR_KEY_AUTHOR, buffer + offset, avail - offset);
    if (!r.ok) return gm_err_void(r.u.err); offset += r.u.val;
    r = gm_cbor_write_text(buffer + offset, avail - offset, e->attribution.author);
    if (!r.ok) return gm_err_void(r.u.err); offset += r.u.val;

    r = gm_cbor_write_uint(GM_CBOR_KEY_SESSION, buffer + offset, avail - offset);
    if (!r.ok) return gm_err_void(r.u.err); offset += r.u.val;
    r = gm_cbor_write_text(buffer + offset, avail - offset, e->attribution.session_id);
    if (!r.ok) return gm_err_void(r.u.err); offset += r.u.val;

    r = gm_cbor_write_uint(GM_CBOR_KEY_FLAGS, buffer + offset, avail - offset);
    if (!r.ok) return gm_err_void(r.u.err); offset += r.u.val;
    r = gm_cbor_write_uint((uint64_t)e->attribution.flags, buffer + offset, avail - offset);
    if (!r.ok) return gm_err_void(r.u.err); offset += r.u.val;

    r = gm_cbor_write_uint(GM_CBOR_KEY_LANE, buffer + offset, avail - offset);
    if (!r.ok) return gm_err_void(r.u.err); offset += r.u.val;
    r = gm_cbor_write_uint((uint64_t)e->lane, buffer + offset, avail - offset);
    if (!r.ok) return gm_err_void(r.u.err); offset += r.u.val;

    *len = offset;
    return gm_ok_void();
}

static int decode_attr_ex_impl(const uint8_t *buffer, size_t len,
                               gm_edge_attributed_t *e, size_t *consumed) {
    if (!buffer || !len || !e || !consumed) return GM_ERR_INVALID_ARGUMENT;
    size_t offset = 0;
    if (offset >= len) return GM_ERR_INVALID_FORMAT;
    uint8_t initial = buffer[offset++];
    if ((initial & 0xE0) != 0xA0) return GM_ERR_INVALID_FORMAT;
    uint8_t addl = (uint8_t)(initial & 0x1F);
    if (addl >= 24) return GM_ERR_INVALID_FORMAT;
    uint32_t fields = addl;

    gm_edge_attributed_t out = {0};
    for (uint32_t i = 0; i < fields; i++) {
        gm_result_uint64_t keyr = gm_cbor_read_uint(buffer, &offset, len);
        if (!keyr.ok) return GM_ERR_INVALID_FORMAT;
        uint64_t key = keyr.u.val;

        switch (key) {
        case GM_CBOR_KEY_SRC_SHA: {
            gm_result_void_t rr = gm_cbor_read_bytes(buffer, &offset, len, out.src_sha, GM_SHA1_SIZE);
            if (!rr.ok) return GM_ERR_INVALID_FORMAT; break; }
        case GM_CBOR_KEY_TGT_SHA: {
            gm_result_void_t rr = gm_cbor_read_bytes(buffer, &offset, len, out.tgt_sha, GM_SHA1_SIZE);
            if (!rr.ok) return GM_ERR_INVALID_FORMAT; break; }
        case GM_CBOR_KEY_REL_TYPE: {
            gm_result_uint64_t rr = gm_cbor_read_uint(buffer, &offset, len);
            if (!rr.ok) return GM_ERR_INVALID_FORMAT; out.rel_type = (uint16_t)rr.u.val; break; }
        case GM_CBOR_KEY_CONFIDENCE: {
            gm_result_uint64_t rr = gm_cbor_read_uint(buffer, &offset, len);
            if (!rr.ok) return GM_ERR_INVALID_FORMAT; out.confidence = (uint16_t)rr.u.val; break; }
        case GM_CBOR_KEY_TIMESTAMP: {
            gm_result_uint64_t rr = gm_cbor_read_uint(buffer, &offset, len);
            if (!rr.ok) return GM_ERR_INVALID_FORMAT; out.timestamp = rr.u.val; break; }
        case GM_CBOR_KEY_SRC_PATH: {
            gm_result_void_t rr = gm_cbor_read_text(buffer, &offset, len, out.src_path, GM_PATH_MAX);
            if (!rr.ok) return GM_ERR_INVALID_FORMAT; break; }
        case GM_CBOR_KEY_TGT_PATH: {
            gm_result_void_t rr = gm_cbor_read_text(buffer, &offset, len, out.tgt_path, GM_PATH_MAX);
            if (!rr.ok) return GM_ERR_INVALID_FORMAT; break; }
        case GM_CBOR_KEY_ULID: {
            gm_result_void_t rr = gm_cbor_read_text(buffer, &offset, len, out.ulid, GM_ULID_SIZE + 1);
            if (!rr.ok) return GM_ERR_INVALID_FORMAT; break; }
        case GM_CBOR_KEY_SRC_OID: {
            uint8_t raw[GM_OID_RAWSZ]; gm_result_void_t rr = gm_cbor_read_bytes(buffer, &offset, len, raw, GM_OID_RAWSZ);
            if (!rr.ok) return GM_ERR_INVALID_FORMAT; git_oid_fromraw(&out.src_oid, raw); break; }
        case GM_CBOR_KEY_TGT_OID: {
            uint8_t raw[GM_OID_RAWSZ]; gm_result_void_t rr = gm_cbor_read_bytes(buffer, &offset, len, raw, GM_OID_RAWSZ);
            if (!rr.ok) return GM_ERR_INVALID_FORMAT; git_oid_fromraw(&out.tgt_oid, raw); break; }
        case GM_CBOR_KEY_SOURCE_TYPE: {
            gm_result_uint64_t rr = gm_cbor_read_uint(buffer, &offset, len);
            if (!rr.ok) return GM_ERR_INVALID_FORMAT; out.attribution.source_type = (gm_source_type_t)rr.u.val; break; }
        case GM_CBOR_KEY_AUTHOR: {
            gm_result_void_t rr = gm_cbor_read_text(buffer, &offset, len, out.attribution.author, sizeof out.attribution.author);
            if (!rr.ok) return GM_ERR_INVALID_FORMAT; break; }
        case GM_CBOR_KEY_SESSION: {
            gm_result_void_t rr = gm_cbor_read_text(buffer, &offset, len, out.attribution.session_id, sizeof out.attribution.session_id);
            if (!rr.ok) return GM_ERR_INVALID_FORMAT; break; }
        case GM_CBOR_KEY_FLAGS: {
            gm_result_uint64_t rr = gm_cbor_read_uint(buffer, &offset, len);
            if (!rr.ok) return GM_ERR_INVALID_FORMAT; out.attribution.flags = (uint32_t)rr.u.val; break; }
        case GM_CBOR_KEY_LANE: {
            gm_result_uint64_t rr = gm_cbor_read_uint(buffer, &offset, len);
            if (!rr.ok) return GM_ERR_INVALID_FORMAT; out.lane = (gm_lane_type_t)rr.u.val; break; }
        default:
            return GM_ERR_INVALID_FORMAT;
        }
    }

    if (git_oid_iszero(&out.src_oid)) git_oid_fromraw(&out.src_oid, out.src_sha);
    if (git_oid_iszero(&out.tgt_oid)) git_oid_fromraw(&out.tgt_oid, out.tgt_sha);

    *e = out; *consumed = offset; return GM_OK;
}

int gm_edge_attributed_decode_cbor_ex(const uint8_t *buffer, size_t len,
                                      gm_edge_attributed_t *edge_out,
                                      size_t *consumed) {
    return decode_attr_ex_impl(buffer, len, edge_out, consumed);
}

gm_result_edge_attributed_t gm_edge_attributed_decode_cbor(const uint8_t *buffer,
                                                           size_t len) {
    gm_edge_attributed_t e;
    size_t used = 0;
    int rc = decode_attr_ex_impl(buffer, len, &e, &used);
    if (rc != GM_OK) {
        return (gm_result_edge_attributed_t){ .ok = false, .u.err = GM_ERROR(rc, "Decode failed") };
    }
    return (gm_result_edge_attributed_t){ .ok = true, .u.val = e };
}

/**
 * Create an attributed edge with full metadata
 */
gm_result_edge_attributed_t gm_edge_attributed_create(
    gm_context_t *ctx, const char *src_path, const char *tgt_path,
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    gm_rel_type_t relationship_type, uint16_t confidence_value,
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
    gm_edge_attributed_t edge = {0};
    
    /* Resolve source SHA and OID */
    gm_result_void_t src_result = resolve_sha(ctx, src_path, edge.src_sha);
    if (!src_result.ok) {
        return (gm_result_edge_attributed_t){.ok = false, .u.err = src_result.u.err};
    }
    git_oid_fromraw(&edge.src_oid, edge.src_sha);
    
    /* Resolve target SHA and OID */
    gm_result_void_t tgt_result = resolve_sha(ctx, tgt_path, edge.tgt_sha);
    if (!tgt_result.ok) {
        return (gm_result_edge_attributed_t){.ok = false, .u.err = tgt_result.u.err};
    }
    git_oid_fromraw(&edge.tgt_oid, edge.tgt_sha);
    
    /* Set basic fields */
    edge.rel_type = (uint16_t)relationship_type;
    edge.confidence = confidence_value;
    edge.timestamp = get_timestamp_millis(ctx);
    
    /* Copy paths - lengths already validated above */
    size_t src_len = strlen(src_path);
    size_t tgt_len = strlen(tgt_path);
    
    assert(src_len + 1 <= GM_PATH_MAX);  /* +1 for null terminator */
    assert(tgt_len + 1 <= GM_PATH_MAX);
    
    gm_memcpy_safe(edge.src_path, sizeof edge.src_path, src_path, src_len + 1);
    gm_memcpy_safe(edge.tgt_path, sizeof edge.tgt_path, tgt_path, tgt_len + 1);
    
    /* Generate ULID */
    gm_result_ulid_t ulid_result = gm_ulid_generate(edge.ulid);
    if (!ulid_result.ok) {
        return (gm_result_edge_attributed_t){.ok = false, .u.err = ulid_result.u.err};
    }
    
    /* Copy attribution and lane */
    edge.attribution = *attribution;
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
    
    int written = gm_snprintf(buffer, len, "%s ──%s──> %s",
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
    const char *edge_base_format = basic_format;
    const char *source_type_name = source_str;
    const char *author_name = edge->attribution.author;
    
    if (edge->attribution.source_type == GM_SOURCE_HUMAN) {
        /* For humans, don't show confidence (always 1.0) */
        written = gm_snprintf(buffer, len, "%s [%s: %s]",
                          edge_base_format, source_type_name, author_name);
    } else {
        /* For AI, show confidence */
        float confidence = gm_confidence_from_half_float(edge->confidence);
        written = gm_snprintf(buffer, len, "%s [%s: %s, conf: %.2f]",
                          edge_base_format, source_type_name, author_name,
                          (double)confidence);
    }
    
    if (written < 0 || (size_t)written >= len) {
        return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL,
                                    "Buffer too small for formatted edge"));
    }
    
    return gm_ok_void();
}
