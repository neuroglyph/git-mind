/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/cbor_common.h"
#include "gitmind/constants_cbor.h"
#include "gitmind.h"
#include <string.h>

/* Read CBOR unsigned integer */
int gm_cbor_read_uint(const uint8_t *buf, size_t *offset, uint64_t *value) {
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
int gm_cbor_read_bytes(const uint8_t *buf, size_t *offset, uint8_t *data, size_t expected_len) {
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
int gm_cbor_read_text(const uint8_t *buf, size_t *offset, char *text, size_t max_len) {
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

/* Write CBOR unsigned integer */
size_t gm_cbor_write_uint(uint8_t *buf, uint64_t value) {
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
size_t gm_cbor_write_bytes(uint8_t *buf, const uint8_t *data, size_t len) {
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
size_t gm_cbor_write_text(uint8_t *buf, const char *text) {
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