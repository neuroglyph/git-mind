/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind.h"
#include "gitmind/constants_cbor.h"
#include <string.h>

/* Write CBOR unsigned integer */
static size_t cbor_write_uint(uint8_t *buf, uint64_t value) {
    if (value < CBOR_IMMEDIATE_THRESHOLD) {
        buf[0] = CBOR_TYPE_UNSIGNED | (uint8_t)value;
        return 1;
    } else if (value <= UINT8_MAX) {
        buf[0] = CBOR_TYPE_UNSIGNED | CBOR_UINT8_FOLLOWS;
        buf[1] = (uint8_t)value;
        return 2;
    } else if (value <= UINT16_MAX) {
        buf[0] = CBOR_TYPE_UNSIGNED | CBOR_UINT16_FOLLOWS;
        buf[1] = (value >> SHIFT_8) & BYTE_MASK;
        buf[2] = value & BYTE_MASK;
        return 3;
    } else if (value <= UINT32_MAX) {
        buf[0] = CBOR_TYPE_UNSIGNED | CBOR_UINT32_FOLLOWS;
        buf[1] = (value >> SHIFT_24) & BYTE_MASK;
        buf[2] = (value >> SHIFT_16) & BYTE_MASK;
        buf[3] = (value >> SHIFT_8) & BYTE_MASK;
        buf[4] = value & BYTE_MASK;
        return 5;
    } else {
        buf[0] = CBOR_TYPE_UNSIGNED | CBOR_UINT64_FOLLOWS;
        for (int i = 0; i < BYTE_SIZE; i++) {
            buf[1 + i] = (value >> (SHIFT_56 - i * BYTE_SIZE)) & BYTE_MASK;
        }
        return 9;
    }
}

/* Write CBOR byte string */
static size_t cbor_write_bytes(uint8_t *buf, const uint8_t *data, size_t len) {
    size_t offset = 0;
    
    if (len < CBOR_IMMEDIATE_THRESHOLD) {
        buf[0] = CBOR_TYPE_BYTES | (uint8_t)len;
        offset = 1;
    } else if (len <= UINT8_MAX) {
        buf[0] = CBOR_TYPE_BYTES | CBOR_UINT8_FOLLOWS;
        buf[1] = (uint8_t)len;
        offset = 2;
    } else {
        buf[0] = CBOR_TYPE_BYTES | CBOR_UINT16_FOLLOWS;
        buf[1] = (len >> SHIFT_8) & BYTE_MASK;
        buf[2] = len & BYTE_MASK;
        offset = 3;
    }
    
    memcpy(buf + offset, data, len);
    return offset + len;
}

/* Write CBOR text string */
static size_t cbor_write_text(uint8_t *buf, const char *text) {
    size_t len = strlen(text);
    size_t offset = 0;
    
    if (len < CBOR_IMMEDIATE_THRESHOLD) {
        buf[0] = CBOR_TYPE_TEXT | (uint8_t)len;
        offset = 1;
    } else if (len <= UINT8_MAX) {
        buf[0] = CBOR_TYPE_TEXT | CBOR_UINT8_FOLLOWS;
        buf[1] = (uint8_t)len;
        offset = 2;
    } else {
        buf[0] = CBOR_TYPE_TEXT | CBOR_UINT16_FOLLOWS;
        buf[1] = (len >> SHIFT_8) & BYTE_MASK;
        buf[2] = len & BYTE_MASK;
        offset = 3;
    }
    
    memcpy(buf + offset, text, len);
    return offset + len;
}

/* Encode edge to CBOR */
int gm_edge_encode_cbor(const gm_edge_t *edge, uint8_t *buffer, size_t *len) {
    if (!edge || !buffer || !len) {
        return GM_INVALID_ARG;
    }
    
    size_t offset = 0;
    
    /* Array header with 7 elements */
    buffer[offset++] = CBOR_TYPE_ARRAY | CBOR_ARRAY_SIZE_EDGE;
    
    /* 1. Source SHA */
    offset += cbor_write_bytes(buffer + offset, edge->src_sha, GM_SHA1_SIZE);
    
    /* 2. Target SHA */
    offset += cbor_write_bytes(buffer + offset, edge->tgt_sha, GM_SHA1_SIZE);
    
    /* 3. Relationship type */
    offset += cbor_write_uint(buffer + offset, edge->rel_type);
    
    /* 4. Confidence */
    offset += cbor_write_uint(buffer + offset, edge->confidence);
    
    /* 5. Timestamp */
    offset += cbor_write_uint(buffer + offset, edge->timestamp);
    
    /* 6. Source path */
    offset += cbor_write_text(buffer + offset, edge->src_path);
    
    /* 7. Target path */
    offset += cbor_write_text(buffer + offset, edge->tgt_path);
    
    *len = offset;
    return GM_OK;
}

/* Read CBOR unsigned integer */
static int cbor_read_uint(const uint8_t *buf, size_t *offset, uint64_t *value) {
    uint8_t initial = buf[(*offset)++];
    uint8_t type = initial & CBOR_TYPE_MASK;
    uint8_t info = initial & CBOR_ADDITIONAL_INFO_MASK;
    
    if (type != CBOR_TYPE_UNSIGNED) {
        return GM_INVALID_ARG;
    }
    
    if (info < CBOR_IMMEDIATE_THRESHOLD) {
        *value = info;
    } else if (info == CBOR_UINT8_FOLLOWS) {
        *value = buf[(*offset)++];
    } else if (info == CBOR_UINT16_FOLLOWS) {
        *value = (buf[*offset] << SHIFT_8) | buf[*offset + 1];
        *offset += 2;
    } else if (info == CBOR_UINT32_FOLLOWS) {
        *value = ((uint32_t)buf[*offset] << SHIFT_24) | 
                 ((uint32_t)buf[*offset + 1] << SHIFT_16) |
                 ((uint32_t)buf[*offset + 2] << SHIFT_8) |
                 buf[*offset + 3];
        *offset += 4;
    } else if (info == CBOR_UINT64_FOLLOWS) {
        *value = 0;
        for (int i = 0; i < BYTE_SIZE; i++) {
            *value = (*value << SHIFT_8) | buf[(*offset)++];
        }
    } else {
        return GM_INVALID_ARG;
    }
    
    return GM_OK;
}

/* Read CBOR byte string */
static int cbor_read_bytes(const uint8_t *buf, size_t *offset, uint8_t *data, size_t expected_len) {
    uint8_t initial = buf[(*offset)++];
    uint8_t type = initial & CBOR_TYPE_MASK;
    uint8_t info = initial & CBOR_ADDITIONAL_INFO_MASK;
    size_t len;
    
    if (type != CBOR_TYPE_BYTES) {
        return GM_INVALID_ARG;
    }
    
    if (info < CBOR_IMMEDIATE_THRESHOLD) {
        len = info;
    } else if (info == CBOR_UINT8_FOLLOWS) {
        len = buf[(*offset)++];
    } else if (info == CBOR_UINT16_FOLLOWS) {
        len = (buf[*offset] << SHIFT_8) | buf[*offset + 1];
        *offset += 2;
    } else {
        return GM_INVALID_ARG;
    }
    
    if (len != expected_len) {
        return GM_INVALID_ARG;
    }
    
    memcpy(data, buf + *offset, len);
    *offset += len;
    
    return GM_OK;
}

/* Read CBOR text string */
static int cbor_read_text(const uint8_t *buf, size_t *offset, char *text, size_t max_len) {
    uint8_t initial = buf[(*offset)++];
    uint8_t type = initial & CBOR_TYPE_MASK;
    uint8_t info = initial & CBOR_ADDITIONAL_INFO_MASK;
    size_t len;
    
    if (type != CBOR_TYPE_TEXT) {
        return GM_INVALID_ARG;
    }
    
    if (info < CBOR_IMMEDIATE_THRESHOLD) {
        len = info;
    } else if (info == CBOR_UINT8_FOLLOWS) {
        len = buf[(*offset)++];
    } else if (info == CBOR_UINT16_FOLLOWS) {
        len = (buf[*offset] << SHIFT_8) | buf[*offset + 1];
        *offset += 2;
    } else {
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

/* Decode CBOR to edge */
int gm_edge_decode_cbor(const uint8_t *buffer, size_t len, gm_edge_t *edge) {
    if (!buffer || !edge || len == 0) {
        return GM_INVALID_ARG;
    }
    
    size_t offset = 0;
    
    /* Initialize edge structure */
    memset(edge, 0, sizeof(gm_edge_t));
    
    /* Need at least one byte for array header */
    if (len < 1) {
        return GM_INVALID_ARG;
    }
    
    /* Check array header */
    /* Check array header */
    if (buffer[offset] != (CBOR_TYPE_ARRAY | CBOR_ARRAY_SIZE_EDGE)) {
        return GM_INVALID_ARG;
    }
    offset++;
    
    /* 1. Source SHA */
    if (cbor_read_bytes(buffer, &offset, edge->src_sha, GM_SHA1_SIZE) != GM_OK) {
        return GM_INVALID_ARG;
    }
    
    /* 2. Target SHA */
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
    
    return GM_OK;
}