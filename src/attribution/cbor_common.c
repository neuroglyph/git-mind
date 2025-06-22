/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/cbor_common.h"

#include "gitmind.h"

#include "gitmind/constants_cbor.h"
#include "../util/gm_mem.h"

#include <string.h>

/*
 * CBOR common functions with strict SRP and reduced complexity
 * Each function has ONE responsibility and CCN < 10
 */

/* ========== Type Checking (SRP: validate CBOR type) ========== */

/* NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - intentional design */
static int validate_type(uint8_t initial_byte, uint8_t expected_type) {
    uint8_t type = initial_byte & CBOR_TYPE_MASK;
    return (type == expected_type) ? GM_OK : GM_INVALID_ARG;
}

/* ========== Length Decoding (SRP: decode CBOR length) ========== */

static int decode_uint8_length(const uint8_t *buf, size_t *offset, uint64_t *value) {
    *value = buf[(*offset)++];
    return GM_OK;
}

static int decode_uint16_length(const uint8_t *buf, size_t *offset, uint64_t *value) {
    *value = (buf[*offset] << SHIFT_8) | buf[*offset + 1];
    *offset += 2;
    return GM_OK;
}

static int decode_uint32_length(const uint8_t *buf, size_t *offset, uint64_t *value) {
    *value = ((uint32_t)buf[*offset] << SHIFT_24) |
             ((uint32_t)buf[*offset + 1] << SHIFT_16) |
             ((uint32_t)buf[*offset + 2] << SHIFT_8) | 
             buf[*offset + 3];
    *offset += 4;
    return GM_OK;
}

static int decode_uint64_length(const uint8_t *buf, size_t *offset, uint64_t *value) {
    *value = 0;
    for (int idx = 0; idx < BYTE_SIZE; idx++) {
        *value = (*value << SHIFT_8) | buf[(*offset)++];
    }
    return GM_OK;
}

static int decode_length_by_info(uint8_t info, const uint8_t *buf, 
                                size_t *offset, uint64_t *value) {
    if (info < CBOR_IMMEDIATE_THRESHOLD) {
        *value = info;
        return GM_OK;
    }
    
    switch (info) {
    case CBOR_UINT8_FOLLOWS:
        return decode_uint8_length(buf, offset, value);
    case CBOR_UINT16_FOLLOWS:
        return decode_uint16_length(buf, offset, value);
    case CBOR_UINT32_FOLLOWS:
        return decode_uint32_length(buf, offset, value);
    case CBOR_UINT64_FOLLOWS:
        return decode_uint64_length(buf, offset, value);
    default:
        return GM_INVALID_ARG;
    }
}

/* ========== Read Functions ========== */

int gm_cbor_read_uint(const uint8_t *buf, size_t *offset, uint64_t *value) {
    uint8_t initial = buf[(*offset)++];
    uint8_t info = initial & CBOR_ADDITIONAL_INFO_MASK;
    
    if (validate_type(initial, CBOR_TYPE_UNSIGNED) != GM_OK) {
        return GM_INVALID_ARG;
    }
    
    return decode_length_by_info(info, buf, offset, value);
}

/* Helper: Read length for bytes/text */
static int read_string_length(const uint8_t *buf, size_t *offset, 
                             uint8_t expected_type, size_t *len) {
    uint8_t initial = buf[(*offset)++];
    uint8_t info = initial & CBOR_ADDITIONAL_INFO_MASK;
    uint64_t len64;
    
    if (validate_type(initial, expected_type) != GM_OK) {
        return GM_INVALID_ARG;
    }
    
    /* Only support up to UINT16 lengths for strings */
    if (info >= CBOR_UINT32_FOLLOWS) {
        return GM_INVALID_ARG;
    }
    
    if (decode_length_by_info(info, buf, offset, &len64) != GM_OK) {
        return GM_INVALID_ARG;
    }
    
    *len = (size_t)len64;
    return GM_OK;
}

int gm_cbor_read_bytes(const uint8_t *buf, size_t *offset, uint8_t *data,
                       size_t expected_len) {
    size_t len;
    
    if (read_string_length(buf, offset, CBOR_TYPE_BYTES, &len) != GM_OK) {
        return GM_INVALID_ARG;
    }
    
    if (len != expected_len) {
        return GM_INVALID_ARG;
    }
    
    gm_memcpy(data, buf + *offset, len);
    *offset += len;
    
    return GM_OK;
}

int gm_cbor_read_text(const uint8_t *buf, size_t *offset, char *text,
                      size_t max_len) {
    size_t len;
    
    if (read_string_length(buf, offset, CBOR_TYPE_TEXT, &len) != GM_OK) {
        return GM_INVALID_ARG;
    }
    
    if (len >= max_len) {
        return GM_INVALID_ARG;
    }
    
    gm_memcpy(text, buf + *offset, len);
    text[len] = '\0';
    *offset += len;
    
    return GM_OK;
}

/* ========== Write Functions ========== */

/* Helper: Encode length header */
/* NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - intentional design */
static size_t encode_uint8_header(uint8_t *buf, uint8_t cbor_type, uint8_t data_value) {
    buf[0] = cbor_type | CBOR_UINT8_FOLLOWS;
    buf[1] = data_value;
    return 2;
}

/* NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - intentional design */
static size_t encode_uint16_header(uint8_t *buf, uint8_t cbor_type, uint16_t data_value) {
    buf[0] = cbor_type | CBOR_UINT16_FOLLOWS;
    buf[1] = (data_value >> SHIFT_8) & BYTE_MASK;
    buf[2] = data_value & BYTE_MASK;
    return 3;
}

static size_t encode_immediate(uint8_t *buf, uint8_t cbor_type, uint8_t immediate_val) {
    buf[0] = cbor_type | immediate_val;
    return 1;
}

static size_t encode_uint32(uint8_t *buf, uint32_t value) {
    buf[0] = CBOR_TYPE_UNSIGNED | CBOR_UINT32_FOLLOWS;
    buf[1] = (value >> SHIFT_24) & BYTE_MASK;
    buf[2] = (value >> SHIFT_16) & BYTE_MASK;
    buf[3] = (value >> SHIFT_8) & BYTE_MASK;
    buf[4] = value & BYTE_MASK;
    return 5;
}

static size_t encode_uint64(uint8_t *buf, uint64_t value) {
    buf[0] = CBOR_TYPE_UNSIGNED | CBOR_UINT64_FOLLOWS;
    for (int idx = 0; idx < BYTE_SIZE; idx++) {
        buf[1 + idx] = (value >> (SHIFT_56 - idx * BYTE_SIZE)) & BYTE_MASK;
    }
    return 9;
}

size_t gm_cbor_write_uint(uint8_t *buf, uint64_t value) {
    if (value < CBOR_IMMEDIATE_THRESHOLD) {
        return encode_immediate(buf, CBOR_TYPE_UNSIGNED, (uint8_t)value);
    }
    
    if (value <= UINT8_MAX) {
        return encode_uint8_header(buf, CBOR_TYPE_UNSIGNED, (uint8_t)value);
    }
    
    if (value <= UINT16_MAX) {
        return encode_uint16_header(buf, CBOR_TYPE_UNSIGNED, (uint16_t)value);
    }
    
    if (value <= UINT32_MAX) {
        return encode_uint32(buf, (uint32_t)value);
    }
    
    return encode_uint64(buf, value);
}

/* Helper: Write string with length header */
static size_t write_string_header(uint8_t *buf, uint8_t cbor_type, size_t length) {
    if (length < CBOR_IMMEDIATE_THRESHOLD) {
        return encode_immediate(buf, cbor_type, (uint8_t)length);
    }
    
    if (length <= UINT8_MAX) {
        return encode_uint8_header(buf, cbor_type, (uint8_t)length);
    }
    
    return encode_uint16_header(buf, cbor_type, (uint16_t)length);
}

size_t gm_cbor_write_bytes(uint8_t *buf, const uint8_t *data, size_t len) {
    size_t offset = write_string_header(buf, CBOR_TYPE_BYTES, len);
    gm_memcpy(buf + offset, data, len);
    return offset + len;
}

size_t gm_cbor_write_text(uint8_t *buf, const char *text) {
    size_t len = strlen(text);
    size_t offset = write_string_header(buf, CBOR_TYPE_TEXT, len);
    gm_memcpy(buf + offset, text, len);
    return offset + len;
}