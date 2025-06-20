/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "../../include/gitmind.h"
#include "../../include/gitmind/constants_cbor.h"
#include "../../include/gitmind/cbor_common.h"
#include <string.h>

/* Validate CBOR array header */
static int validate_array_header(const uint8_t *buffer, size_t len, size_t expected_size) {
    if (len < 1) {
        return GM_INVALID_ARG;
    }
    
    uint8_t header = buffer[0];
    if ((header & CBOR_TYPE_MASK) != CBOR_TYPE_ARRAY || 
        (header & CBOR_ADDITIONAL_INFO_MASK) != expected_size) {
        return GM_INVALID_ARG;
    }
    
    return GM_OK;
}

/* Decode SHA field */
static int decode_sha_field(const uint8_t *buffer, size_t *offset, uint8_t *sha) {
    if ((buffer[*offset] & CBOR_TYPE_MASK) != CBOR_TYPE_BYTES) {
        return GM_INVALID_ARG;
    }
    (*offset)++;  /* Skip byte string header */
    
    return gm_cbor_read_bytes(buffer, offset, sha, GM_SHA1_SIZE);
}

/* Decode edge metadata fields */
static int decode_edge_metadata(const uint8_t *buffer, size_t *offset, gm_edge_t *edge) {
    uint64_t temp;
    
    /* Relationship type */
    if (gm_cbor_read_uint(buffer, offset, &temp) != GM_OK) {
        return GM_INVALID_ARG;
    }
    edge->rel_type = (uint16_t)temp;
    
    /* Confidence */
    if (gm_cbor_read_uint(buffer, offset, &temp) != GM_OK) {
        return GM_INVALID_ARG;
    }
    edge->confidence = (uint16_t)temp;
    
    /* Timestamp */
    return gm_cbor_read_uint(buffer, offset, &edge->timestamp);
}

/* Decode path fields */
static int decode_path_fields(const uint8_t *buffer, size_t *offset, gm_edge_t *edge) {
    /* Source path */
    if (gm_cbor_read_text(buffer, offset, edge->src_path, GM_PATH_MAX) != GM_OK) {
        return GM_INVALID_ARG;
    }
    
    /* Target path */
    return gm_cbor_read_text(buffer, offset, edge->tgt_path, GM_PATH_MAX);
}

/* Decode CBOR to edge with consumed bytes */
int gm_edge_decode_cbor_ex(const uint8_t *buffer, size_t len, gm_edge_t *edge, size_t *consumed) {
    if (!buffer || !edge || !consumed || len == 0) {
        return GM_INVALID_ARG;
    }
    
    size_t offset = 0;
    int rc;
    
    /* Initialize edge structure */
    memset(edge, 0, sizeof(gm_edge_t));
    
    /* Validate array header */
    rc = validate_array_header(buffer, len, CBOR_ARRAY_SIZE_EDGE);
    if (rc != GM_OK) {
        return rc;
    }
    offset++;
    
    /* Decode SHA fields */
    rc = decode_sha_field(buffer, &offset, edge->src_sha);
    if (rc != GM_OK) {
        return rc;
    }
    
    rc = decode_sha_field(buffer, &offset, edge->tgt_sha);
    if (rc != GM_OK) {
        return rc;
    }
    
    /* Decode metadata */
    rc = decode_edge_metadata(buffer, &offset, edge);
    if (rc != GM_OK) {
        return rc;
    }
    
    /* Decode paths */
    rc = decode_path_fields(buffer, &offset, edge);
    if (rc != GM_OK) {
        return rc;
    }
    
    /* Return consumed bytes */
    *consumed = offset;
    
    return GM_OK;
}