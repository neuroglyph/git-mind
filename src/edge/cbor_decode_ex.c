/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "../../include/gitmind.h"
#include "../../include/gitmind/constants_cbor.h"
#include "../../include/gitmind/cbor_common.h"
#include <string.h>

/* Decode CBOR to edge with consumed bytes */
int gm_edge_decode_cbor_ex(const uint8_t *buffer, size_t len, gm_edge_t *edge, size_t *consumed) {
    if (!buffer || !edge || !consumed || len == 0) {
        return GM_INVALID_ARG;
    }
    
    size_t offset = 0;
    size_t start_offset = 0;
    
    /* Initialize edge structure */
    memset(edge, 0, sizeof(gm_edge_t));
    
    /* Need at least one byte for array header */
    if (len < 1) {
        return GM_INVALID_ARG;
    }
    
    /* Check array header */
    uint8_t header = buffer[offset];
    if ((header & CBOR_TYPE_MASK) != CBOR_TYPE_ARRAY || (header & CBOR_ADDITIONAL_INFO_MASK) != CBOR_ARRAY_SIZE_EDGE) {
        return GM_INVALID_ARG;
    }
    offset++;
    
    /* 1. Source SHA (20 bytes) */
    if (offset + 1 + CBOR_SHA_SIZE > len) return GM_INVALID_ARG;
    if ((buffer[offset] & CBOR_TYPE_MASK) != CBOR_TYPE_BYTES) return GM_INVALID_ARG;
    offset++;  /* Skip byte string header */
    
    if (gm_cbor_read_bytes(buffer, &offset, edge->src_sha, GM_SHA1_SIZE) != GM_OK) {
        return GM_INVALID_ARG;
    }
    
    /* 2. Target SHA (20 bytes) */
    if (offset + 1 + CBOR_SHA_SIZE > len) return GM_INVALID_ARG;
    if ((buffer[offset] & CBOR_TYPE_MASK) != CBOR_TYPE_BYTES) return GM_INVALID_ARG;
    offset++;  /* Skip byte string header */
    
    if (gm_cbor_read_bytes(buffer, &offset, edge->tgt_sha, GM_SHA1_SIZE) != GM_OK) {
        return GM_INVALID_ARG;
    }
    
    /* 3. Relationship type */
    uint64_t rel_type;
    if (gm_cbor_read_uint(buffer, &offset, &rel_type) != GM_OK) {
        return GM_INVALID_ARG;
    }
    edge->rel_type = (uint16_t)rel_type;
    
    /* 4. Confidence */
    uint64_t confidence;
    if (gm_cbor_read_uint(buffer, &offset, &confidence) != GM_OK) {
        return GM_INVALID_ARG;
    }
    edge->confidence = (uint16_t)confidence;
    
    /* 5. Timestamp */
    if (gm_cbor_read_uint(buffer, &offset, &edge->timestamp) != GM_OK) {
        return GM_INVALID_ARG;
    }
    
    /* 6. Source path */
    if (gm_cbor_read_text(buffer, &offset, edge->src_path, GM_PATH_MAX) != GM_OK) {
        return GM_INVALID_ARG;
    }
    
    /* 7. Target path */
    if (gm_cbor_read_text(buffer, &offset, edge->tgt_path, GM_PATH_MAX) != GM_OK) {
        return GM_INVALID_ARG;
    }
    
    /* Return consumed bytes */
    *consumed = offset - start_offset;
    
    return GM_OK;
}