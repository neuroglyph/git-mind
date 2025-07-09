/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/edge.h"
#include "gitmind/context.h"
#include "gitmind/error.h"
#include "gitmind/types.h"
#include "gitmind/cbor/cbor.h"
#include "gitmind/result.h"
#include "gitmind/types/ulid.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/* Result type is defined in edge.h */

/* Constants */
static const uint16_t DefaultConfidence = 0x3C00; /* 1.0 in IEEE-754 half-float */
static const uint8_t CborMapType = 0xA0; /* CBOR map type prefix */

/**
 * Get current timestamp in milliseconds
 */
static uint64_t get_timestamp_millis(gm_context_t *ctx) {
    struct timespec time_spec;
    if (ctx && ctx->time_ops && ctx->time_ops->clock_gettime) {
        ctx->time_ops->clock_gettime(CLOCK_REALTIME, &time_spec);
    } else {
        (void)clock_gettime(CLOCK_REALTIME, &time_spec);
    }
    return ((uint64_t)time_spec.tv_sec * MILLIS_PER_SECOND) +
           ((uint64_t)time_spec.tv_nsec / NANOS_PER_MILLI);
}

/**
 * Initialize edge with default values
 */
static void edge_init_defaults(gm_edge_t *edge) {
    (void)memset(edge, 0, sizeof(gm_edge_t));
    edge->confidence = DefaultConfidence;
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
 * Create an edge between two files
 */
gm_result_edge_t gm_edge_create(gm_context_t *ctx, const char *src_path,
                                const char *tgt_path, gm_rel_type_t rel_type) {
    /* Validate arguments */
    if (!ctx || !src_path || !tgt_path) {
        return (gm_result_edge_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERR_INVALID_ARGUMENT, "Invalid arguments")
        };
    }
    
    /* Check path lengths */
    if (strlen(src_path) >= GM_PATH_MAX || strlen(tgt_path) >= GM_PATH_MAX) {
        return (gm_result_edge_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERR_INVALID_ARGUMENT, "Path too long")
        };
    }
    
    /* Initialize edge */
    gm_edge_t edge;
    edge_init_defaults(&edge);
    
    /* Resolve source SHA */
    gm_result_void_t src_result = resolve_sha(ctx, src_path, edge.src_sha);
    if (!src_result.ok) {
        return (gm_result_edge_t){.ok = false, .u.err = src_result.u.err};
    }
    
    /* Resolve target SHA */
    gm_result_void_t tgt_result = resolve_sha(ctx, tgt_path, edge.tgt_sha);
    if (!tgt_result.ok) {
        return (gm_result_edge_t){.ok = false, .u.err = tgt_result.u.err};
    }
    
    /* Set fields */
    (void)strncpy(edge.src_path, src_path, GM_PATH_MAX - 1);
    edge.src_path[GM_PATH_MAX - 1] = '\0';
    
    (void)strncpy(edge.tgt_path, tgt_path, GM_PATH_MAX - 1);
    edge.tgt_path[GM_PATH_MAX - 1] = '\0';
    
    edge.rel_type = (uint16_t)rel_type;
    edge.timestamp = get_timestamp_millis(ctx);
    
    /* Generate ULID */
    gm_result_ulid_t ulid_result = gm_ulid_generate(edge.ulid);
    if (!ulid_result.ok) {
        return (gm_result_edge_t){.ok = false, .u.err = ulid_result.u.err};
    }
    
    return (gm_result_edge_t){.ok = true, .u.val = edge};
}

/**
 * Compare two edges for equality
 */
bool gm_edge_equal(const gm_edge_t *edge_a, const gm_edge_t *edge_b) {
    if (!edge_a || !edge_b) {
        return false;
    }
    
    /* Compare SHAs */
    if (memcmp(edge_a->src_sha, edge_b->src_sha, GM_SHA1_SIZE) != 0) {
        return false;
    }
    
    if (memcmp(edge_a->tgt_sha, edge_b->tgt_sha, GM_SHA1_SIZE) != 0) {
        return false;
    }
    
    /* Compare relationship type */
    return edge_a->rel_type == edge_b->rel_type;
}

/**
 * Get relationship type string
 */
static const char *get_rel_type_string(uint16_t rel_type) {
    switch ((gm_rel_type_t)rel_type) {
    case GM_REL_IMPLEMENTS:
        return "IMPLEMENTS";
    case GM_REL_REFERENCES:
        return "REFERENCES";
    case GM_REL_DEPENDS_ON:
        return "DEPENDS_ON";
    case GM_REL_AUGMENTS:
        return "AUGMENTS";
    default:
        return "CUSTOM";
    }
}

/**
 * Format an edge as a human-readable string
 */
gm_result_void_t gm_edge_format(const gm_edge_t *edge, char *buffer, size_t len) {
    if (!edge || !buffer || len == 0) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "Invalid arguments"));
    }
    
    const char *type_str = get_rel_type_string(edge->rel_type);
    
    int written = snprintf(buffer, len, "%s: %s -> %s",
                          type_str, edge->src_path, edge->tgt_path);
    
    if (written < 0 || (size_t)written >= len) {
        return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL,
                                    "Buffer too small for formatted edge"));
    }
    
    return gm_ok_void();
}

/**
 * CBOR encoding constants for edge fields
 */
#define CBOR_KEY_SRC_SHA    0
#define CBOR_KEY_TGT_SHA    1
#define CBOR_KEY_REL_TYPE   2
#define CBOR_KEY_CONFIDENCE 3
#define CBOR_KEY_TIMESTAMP  4
#define CBOR_KEY_SRC_PATH   5
#define CBOR_KEY_TGT_PATH   6
#define CBOR_KEY_ULID       7
#define CBOR_EDGE_FIELDS    8

/**
 * Write CBOR key-value pair for bytes
 */
static gm_result_void_t write_cbor_bytes(uint64_t key, uint8_t *buffer, 
                                       size_t available, size_t *offset,
                                       const uint8_t *data, size_t data_size) {
    gm_result_size_t key_result = gm_cbor_write_uint(key, buffer + *offset, available - *offset);
    if (!key_result.ok) {
        return gm_err_void(key_result.u.err);
    }
    *offset += key_result.u.val;
    
    gm_result_size_t val_result = gm_cbor_write_bytes(buffer + *offset, available - *offset, data, data_size);
    if (!val_result.ok) {
        return gm_err_void(val_result.u.err);
    }
    *offset += val_result.u.val;
    return gm_ok_void();
}

/**
 * Write CBOR key-value pair for uint64
 */
static gm_result_void_t write_cbor_uint64(uint64_t key, uint8_t *buffer, 
                                        size_t available, size_t *offset,
                                        uint64_t value) {
    gm_result_size_t key_result = gm_cbor_write_uint(key, buffer + *offset, available - *offset);
    if (!key_result.ok) {
        return gm_err_void(key_result.u.err);
    }
    *offset += key_result.u.val;
    
    gm_result_size_t val_result = gm_cbor_write_uint(value, buffer + *offset, available - *offset);
    if (!val_result.ok) {
        return gm_err_void(val_result.u.err);
    }
    *offset += val_result.u.val;
    return gm_ok_void();
}

/**
 * Write CBOR key-value pair for text
 */
static gm_result_void_t write_cbor_text(uint64_t key, uint8_t *buffer, 
                                      size_t available, size_t *offset,
                                      const char *text) {
    gm_result_size_t key_result = gm_cbor_write_uint(key, buffer + *offset, available - *offset);
    if (!key_result.ok) {
        return gm_err_void(key_result.u.err);
    }
    *offset += key_result.u.val;
    
    gm_result_size_t val_result = gm_cbor_write_text(buffer + *offset, available - *offset, text);
    if (!val_result.ok) {
        return gm_err_void(val_result.u.err);
    }
    *offset += val_result.u.val;
    return gm_ok_void();
}

/**
 * Write CBOR map header
 */
static gm_result_void_t write_cbor_map_header(uint8_t *buffer, size_t available, size_t *offset) {
    if (available < 1) {
        return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL,
                                    "Buffer too small for CBOR map"));
    }
    buffer[(*offset)++] = CborMapType | CBOR_EDGE_FIELDS;
    return gm_ok_void();
}

/**
 * Encode edge to CBOR format
 */
gm_result_void_t gm_edge_encode_cbor(const gm_edge_t *edge, uint8_t *buffer,
                                     size_t *len) {
    if (!edge || !buffer || !len) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "Invalid arguments"));
    }
    
    size_t offset = 0;
    size_t available = *len;
    
    /* Write map header */
    gm_result_void_t result = write_cbor_map_header(buffer, available, &offset);
    if (!result.ok) {
        return result;
    }
    
    /* Write SHA fields */
    result = write_cbor_bytes(CBOR_KEY_SRC_SHA, buffer, available, &offset, edge->src_sha, GM_SHA1_SIZE);
    if (!result.ok) {
        return result;
    }
    
    result = write_cbor_bytes(CBOR_KEY_TGT_SHA, buffer, available, &offset, edge->tgt_sha, GM_SHA1_SIZE);
    if (!result.ok) {
        return result;
    }
    
    /* Write numeric fields */
    result = write_cbor_uint64(CBOR_KEY_REL_TYPE, buffer, available, &offset, edge->rel_type);
    if (!result.ok) {
        return result;
    }
    
    result = write_cbor_uint64(CBOR_KEY_CONFIDENCE, buffer, available, &offset, edge->confidence);
    if (!result.ok) {
        return result;
    }
    
    result = write_cbor_uint64(CBOR_KEY_TIMESTAMP, buffer, available, &offset, edge->timestamp);
    if (!result.ok) {
        return result;
    }
    
    /* Write string fields */
    result = write_cbor_text(CBOR_KEY_SRC_PATH, buffer, available, &offset, edge->src_path);
    if (!result.ok) {
        return result;
    }
    
    result = write_cbor_text(CBOR_KEY_TGT_PATH, buffer, available, &offset, edge->tgt_path);
    if (!result.ok) {
        return result;
    }
    
    result = write_cbor_text(CBOR_KEY_ULID, buffer, available, &offset, edge->ulid);
    if (!result.ok) {
        return result;
    }
    
    *len = offset;
    return gm_ok_void();
}

/**
 * Helper to decode SHA bytes from CBOR
 */
static gm_result_void_t decode_cbor_sha(const uint8_t *buffer, size_t *offset, size_t len, uint8_t *sha) {
    return gm_cbor_read_bytes(buffer, offset, len, sha, GM_SHA1_SIZE);
}

/**
 * Helper to decode uint64 from CBOR
 */
static gm_result_uint64_t decode_cbor_uint64(const uint8_t *buffer, size_t *offset, size_t len) {
    return gm_cbor_read_uint(buffer, offset, len);
}

/**
 * Helper to decode text from CBOR
 */
static gm_result_void_t decode_cbor_text(const uint8_t *buffer, size_t *offset, size_t len, char *text, size_t max_len) {
    return gm_cbor_read_text(buffer, offset, len, text, max_len);
}

/**
 * Helper to decode one CBOR key-value pair
 */
static gm_result_void_t decode_cbor_field(const uint8_t *buffer, size_t *offset, size_t len, gm_edge_t *edge) {
    /* Read key */
    gm_result_uint64_t key_result = decode_cbor_uint64(buffer, offset, len);
    if (!key_result.ok) {
        return gm_err_void(key_result.u.err);
    }
    uint64_t key = key_result.u.val;
    
    /* Read value based on key */
    switch (key) {
    case CBOR_KEY_SRC_SHA: {
        return decode_cbor_sha(buffer, offset, len, edge->src_sha);
    }
    case CBOR_KEY_TGT_SHA: {
        return decode_cbor_sha(buffer, offset, len, edge->tgt_sha);
    }
    case CBOR_KEY_REL_TYPE: {
        gm_result_uint64_t result = decode_cbor_uint64(buffer, offset, len);
        if (!result.ok) {
            return gm_err_void(result.u.err);
        }
        edge->rel_type = (uint16_t)result.u.val;
        return gm_ok_void();
    }
    case CBOR_KEY_CONFIDENCE: {
        gm_result_uint64_t result = decode_cbor_uint64(buffer, offset, len);
        if (!result.ok) {
            return gm_err_void(result.u.err);
        }
        edge->confidence = (uint16_t)result.u.val;
        return gm_ok_void();
    }
    case CBOR_KEY_TIMESTAMP: {
        gm_result_uint64_t result = decode_cbor_uint64(buffer, offset, len);
        if (!result.ok) {
            return gm_err_void(result.u.err);
        }
        edge->timestamp = result.u.val;
        return gm_ok_void();
    }
    case CBOR_KEY_SRC_PATH: {
        return decode_cbor_text(buffer, offset, len, edge->src_path, GM_PATH_MAX);
    }
    case CBOR_KEY_TGT_PATH: {
        return decode_cbor_text(buffer, offset, len, edge->tgt_path, GM_PATH_MAX);
    }
    case CBOR_KEY_ULID: {
        return decode_cbor_text(buffer, offset, len, edge->ulid, GM_ULID_SIZE + 1);
    }
    default:
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_FORMAT, "Unknown CBOR key"));
    }
}

/**
 * Decode edge from CBOR format
 */
gm_result_edge_t gm_edge_decode_cbor(const uint8_t *buffer, size_t len) {
    if (!buffer || len == 0) {
        return (gm_result_edge_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERR_INVALID_ARGUMENT, "Invalid arguments")
        };
    }
    
    size_t offset = 0;
    
    /* Check map header */
    if (buffer[offset] != (CborMapType | CBOR_EDGE_FIELDS)) {
        return (gm_result_edge_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERR_INVALID_FORMAT, "Invalid CBOR map header")
        };
    }
    offset++;
    
    gm_edge_t edge;
    edge_init_defaults(&edge);
    
    /* Read key-value pairs */
    for (int i = 0; i < CBOR_EDGE_FIELDS; i++) {
        gm_result_void_t result = decode_cbor_field(buffer, &offset, len, &edge);
        if (!result.ok) {
            return (gm_result_edge_t){.ok = false, .u.err = result.u.err};
        }
    }
    
    return (gm_result_edge_t){.ok = true, .u.val = edge};
}
