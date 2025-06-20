/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "../../include/gitmind.h"
#include "../../include/gitmind/constants_cbor.h"
#include <string.h>

/* CBOR decoding helpers - duplicated from cbor.c for now */
static int cbor_read_bytes(const uint8_t *buf, size_t *offset, uint8_t *out, size_t len) {
    memcpy(out, buf + *offset, len);
    *offset += len;
    return GM_OK;
}

static int cbor_read_uint(const uint8_t *buf, size_t *offset, uint64_t *value) {
    uint8_t type = buf[*offset];
    *offset += 1;
    
    if (type <= CBOR_MAX_IMMEDIATE) {
        *value = type;
    } else if (type == CBOR_UINT8_FOLLOWS) {
        *value = buf[*offset];
        *offset += 1;
    } else if (type == CBOR_UINT16_FOLLOWS) {
        *value = (buf[*offset] << SHIFT_8) | buf[*offset + 1];
        *offset += 2;
    } else if (type == CBOR_UINT32_FOLLOWS) {
        *value = ((uint32_t)buf[*offset] << SHIFT_24) | 
                ((uint32_t)buf[*offset + 1] << SHIFT_16) |
                ((uint32_t)buf[*offset + 2] << SHIFT_8) |
                buf[*offset + 3];
        *offset += 4;
    } else if (type == CBOR_UINT64_FOLLOWS) {
        *value = ((uint64_t)buf[*offset] << SHIFT_56) |
                ((uint64_t)buf[*offset + 1] << SHIFT_48) |
                ((uint64_t)buf[*offset + 2] << SHIFT_40) |
                ((uint64_t)buf[*offset + 3] << SHIFT_32) |
                ((uint64_t)buf[*offset + 4] << SHIFT_24) |
                ((uint64_t)buf[*offset + 5] << SHIFT_16) |
                ((uint64_t)buf[*offset + 6] << SHIFT_8) |
                buf[*offset + 7];
        *offset += 8;
    } else {
        return GM_INVALID_ARG;
    }
    
    return GM_OK;
}

static int cbor_read_text(const uint8_t *buf, size_t *offset, char *text, size_t max_len) {
    uint8_t type = buf[*offset];
    uint64_t len = 0;
    
    if ((type & CBOR_TYPE_MASK) != CBOR_TYPE_TEXT) {
        return GM_INVALID_ARG;
    }
    
    if (cbor_read_uint(buf, offset, &len) != GM_OK) {
        return GM_INVALID_ARG;
    }
    
    if (len >= max_len) {
        return GM_INVALID_ARG;
    }
    
    memcpy(text, buf + *offset, len);
    text[len] = '\0';
    *offset += len;
    
    return GM_OK;
}

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
    
    if (cbor_read_bytes(buffer, &offset, edge->src_sha, GM_SHA1_SIZE) != GM_OK) {
        return GM_INVALID_ARG;
    }
    
    /* 2. Target SHA (20 bytes) */
    if (offset + 1 + CBOR_SHA_SIZE > len) return GM_INVALID_ARG;
    if ((buffer[offset] & CBOR_TYPE_MASK) != CBOR_TYPE_BYTES) return GM_INVALID_ARG;
    offset++;  /* Skip byte string header */
    
    if (cbor_read_bytes(buffer, &offset, edge->tgt_sha, GM_SHA1_SIZE) != GM_OK) {
        return GM_INVALID_ARG;
    }
    
    /* 3. Relationship type */
    uint64_t rel_type;
    if (cbor_read_uint(buffer, &offset, &rel_type) != GM_OK) {
        return GM_INVALID_ARG;
    }
    edge->rel_type = (uint16_t)rel_type;
    
    /* 4. Confidence */
    uint64_t confidence;
    if (cbor_read_uint(buffer, &offset, &confidence) != GM_OK) {
        return GM_INVALID_ARG;
    }
    edge->confidence = (uint16_t)confidence;
    
    /* 5. Timestamp */
    if (cbor_read_uint(buffer, &offset, &edge->timestamp) != GM_OK) {
        return GM_INVALID_ARG;
    }
    
    /* 6. Source path */
    if (cbor_read_text(buffer, &offset, edge->src_path, GM_PATH_MAX) != GM_OK) {
        return GM_INVALID_ARG;
    }
    
    /* 7. Target path */
    if (cbor_read_text(buffer, &offset, edge->tgt_path, GM_PATH_MAX) != GM_OK) {
        return GM_INVALID_ARG;
    }
    
    /* Return consumed bytes */
    *consumed = offset - start_offset;
    
    return GM_OK;
}