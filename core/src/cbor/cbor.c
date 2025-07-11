/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <gitmind/cbor/cbor.h>
#include <gitmind/cbor/constants_cbor.h>
#include <gitmind/error.h>
#include <gitmind/result.h>

#include <stdint.h>
#include <string.h>

/* Error code constants */
static const int GmErrorCborTypeMismatch = 6001;
static const int GmErrorCborBufferTooSmall = 6002;
static const int GmErrorCborInvalidData = 6003;
static const int GmErrorCborOverflow = 6004;

/* Size constants */
static const size_t CborUint64Size = 8;
static const size_t CborUint32HeaderSize = 5;
static const size_t CborUint64HeaderSize = 9;

/* Helper to check buffer bounds */
static bool check_read_bounds(size_t offset, size_t read_size, size_t max_size) {
    if (offset > max_size) {
        return false;
    }
    if (read_size > max_size - offset) {
        return false;
    }
    return true;
}

/* Helper to read uint8 value */
static gm_result_uint64_t read_uint8_value(const uint8_t *buf, size_t *offset, size_t max_size) {
    if (!check_read_bounds(*offset, 1, max_size)) {
        return (gm_result_uint64_t){
            .ok = false,
            .u.err = GM_ERROR(GmErrorCborBufferTooSmall, "Buffer underrun reading uint8")
        };
    }
    return (gm_result_uint64_t){.ok = true, .u.val = buf[(*offset)++]};
}

/* Helper to read uint16 value */
static gm_result_uint64_t read_uint16_value(const uint8_t *buf, size_t *offset, size_t max_size) {
    if (!check_read_bounds(*offset, 2, max_size)) {
        return (gm_result_uint64_t){
            .ok = false,
            .u.err = GM_ERROR(GmErrorCborBufferTooSmall, "Buffer underrun reading uint16")
        };
    }
    uint64_t value = (uint64_t)(((uint16_t)buf[*offset] << SHIFT_8) | (uint16_t)buf[*offset + 1]);
    *offset += 2;
    return (gm_result_uint64_t){.ok = true, .u.val = value};
}

/* Helper to read uint32 value */
static gm_result_uint64_t read_uint32_value(const uint8_t *buf, size_t *offset, size_t max_size) {
    if (!check_read_bounds(*offset, 4, max_size)) {
        return (gm_result_uint64_t){
            .ok = false,
            .u.err = GM_ERROR(GmErrorCborBufferTooSmall, "Buffer underrun reading uint32")
        };
    }
    uint64_t value = ((uint32_t)buf[*offset] << SHIFT_24) |
                     ((uint32_t)buf[*offset + 1] << SHIFT_16) |
                     ((uint32_t)buf[*offset + 2] << SHIFT_8) | 
                     (uint32_t)buf[*offset + 3];
    *offset += 4;
    return (gm_result_uint64_t){.ok = true, .u.val = value};
}

/* Helper to read uint64 value */
static gm_result_uint64_t read_uint64_value(const uint8_t *buf, size_t *offset, size_t max_size) {
    if (!check_read_bounds(*offset, CborUint64Size, max_size)) {
        return (gm_result_uint64_t){
            .ok = false,
            .u.err = GM_ERROR(GmErrorCborBufferTooSmall, "Buffer underrun reading uint64")
        };
    }
    uint64_t value = 0;
    for (int i = 0; i < BYTE_SIZE; i++) {
        value = (value << SHIFT_8) | buf[(*offset)++];
    }
    return (gm_result_uint64_t){.ok = true, .u.val = value};
}

/* Helper to read CBOR uint value based on info type */
/* NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - parameter order is intentional for consistency */
static gm_result_uint64_t read_uint_value(const uint8_t *buf, size_t *offset, 
                                          size_t max_size, uint8_t additional_info) { /* NOLINT(bugprone-easily-swappable-parameters) */
    if (additional_info < CBOR_IMMEDIATE_THRESHOLD) {
        return (gm_result_uint64_t){.ok = true, .u.val = additional_info};
    }
    
    if (additional_info == CBOR_UINT8_FOLLOWS) {
        return read_uint8_value(buf, offset, max_size);
    }
    
    if (additional_info == CBOR_UINT16_FOLLOWS) {
        return read_uint16_value(buf, offset, max_size);
    }
    
    if (additional_info == CBOR_UINT32_FOLLOWS) {
        return read_uint32_value(buf, offset, max_size);
    }
    
    if (additional_info == CBOR_UINT64_FOLLOWS) {
        return read_uint64_value(buf, offset, max_size);
    }
    
    return (gm_result_uint64_t){
        .ok = false,
        .u.err = GM_ERROR(GmErrorCborInvalidData, 
                         "Invalid additional info: 0x%02x", additional_info)
    };
}

/* Read CBOR unsigned integer with bounds checking */
gm_result_uint64_t gm_cbor_read_uint(const uint8_t *buf, size_t *offset, size_t max_size) {
    if (!buf || !offset) {
        return (gm_result_uint64_t){
            .ok = false,
            .u.err = GM_ERROR(GmErrorCborInvalidData, "NULL buffer or offset")
        };
    }

    if (!check_read_bounds(*offset, 1, max_size)) {
        return (gm_result_uint64_t){
            .ok = false,
            .u.err = GM_ERROR(GmErrorCborBufferTooSmall, "Buffer underrun reading type byte")
        };
    }

    uint8_t initial = buf[(*offset)++];
    uint8_t type = initial & CBOR_TYPE_MASK;
    uint8_t info = initial & CBOR_ADDITIONAL_INFO_MASK;

    if (type != CBOR_TYPE_UNSIGNED) {
        return (gm_result_uint64_t){
            .ok = false,
            .u.err = GM_ERROR(GmErrorCborTypeMismatch, 
                             "Expected unsigned integer, got type 0x%02x", type)
        };
    }

    return read_uint_value(buf, offset, max_size, info);
}

/* Helper to read CBOR length from additional info */
/* NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - parameter order is intentional for consistency */
static gm_result_size_t read_cbor_length(const uint8_t *buf, size_t *offset, 
                                         size_t max_size, uint8_t additional_info) { /* NOLINT(bugprone-easily-swappable-parameters) */
    if (additional_info < CBOR_IMMEDIATE_THRESHOLD) {
        return (gm_result_size_t){.ok = true, .u.val = additional_info};
    }
    
    if (additional_info == CBOR_UINT8_FOLLOWS) {
        if (!check_read_bounds(*offset, 1, max_size)) {
            return (gm_result_size_t){
                .ok = false,
                .u.err = GM_ERROR(GmErrorCborBufferTooSmall, "Buffer underrun reading length")
            };
        }
        return (gm_result_size_t){.ok = true, .u.val = buf[(*offset)++]};
    }
    
    if (additional_info == CBOR_UINT16_FOLLOWS) {
        if (!check_read_bounds(*offset, 2, max_size)) {
            return (gm_result_size_t){
                .ok = false,
                .u.err = GM_ERROR(GmErrorCborBufferTooSmall, "Buffer underrun reading length")
            };
        }
        size_t len = ((size_t)buf[*offset] << SHIFT_8) | (size_t)buf[*offset + 1];
        *offset += 2;
        return (gm_result_size_t){.ok = true, .u.val = len};
    }
    
    return (gm_result_size_t){
        .ok = false,
        .u.err = GM_ERROR(GmErrorCborInvalidData, 
                         "Invalid additional info: 0x%02x", additional_info)
    };
}

/* Read CBOR byte string with bounds checking */
gm_result_void_t gm_cbor_read_bytes(const uint8_t *buf, size_t *offset, size_t max_size,
                                    uint8_t *data, size_t expected_len) {
    if (!buf || !offset || !data) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GmErrorCborInvalidData, "NULL buffer, offset, or data")
        };
    }

    if (!check_read_bounds(*offset, 1, max_size)) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GmErrorCborBufferTooSmall, "Buffer underrun reading type byte")
        };
    }

    uint8_t initial = buf[(*offset)++];
    uint8_t type = initial & CBOR_TYPE_MASK;
    uint8_t info = initial & CBOR_ADDITIONAL_INFO_MASK;

    if (type != CBOR_TYPE_BYTES) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GmErrorCborTypeMismatch, 
                             "Expected byte string, got type 0x%02x", type)
        };
    }

    gm_result_size_t len_result = read_cbor_length(buf, offset, max_size, info);
    if (!len_result.ok) {
        return (gm_result_void_t){.ok = false, .u.err = len_result.u.err};
    }
    size_t len = len_result.u.val;

    if (len != expected_len) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GmErrorCborInvalidData, 
                             "Length mismatch: expected %zu, got %zu", expected_len, len)
        };
    }

    if (!check_read_bounds(*offset, len, max_size)) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GmErrorCborBufferTooSmall, 
                             "Buffer underrun reading %zu bytes", len)
        };
    }

    memcpy(data, buf + *offset, len);
    *offset += len;

    return (gm_result_void_t){.ok = true};
}

/* Read CBOR text string with bounds checking */
gm_result_void_t gm_cbor_read_text(const uint8_t *buf, size_t *offset, size_t max_size,
                                   char *text, size_t max_text_len) {
    if (!buf || !offset || !text) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GmErrorCborInvalidData, "NULL buffer, offset, or text")
        };
    }

    if (max_text_len == 0) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GmErrorCborInvalidData, "Text buffer size is zero")
        };
    }

    if (!check_read_bounds(*offset, 1, max_size)) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GmErrorCborBufferTooSmall, "Buffer underrun reading type byte")
        };
    }

    uint8_t initial = buf[(*offset)++];
    uint8_t type = initial & CBOR_TYPE_MASK;
    uint8_t info = initial & CBOR_ADDITIONAL_INFO_MASK;

    if (type != CBOR_TYPE_TEXT) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GmErrorCborTypeMismatch, 
                             "Expected text string, got type 0x%02x", type)
        };
    }

    gm_result_size_t len_result = read_cbor_length(buf, offset, max_size, info);
    if (!len_result.ok) {
        return (gm_result_void_t){.ok = false, .u.err = len_result.u.err};
    }
    size_t len = len_result.u.val;

    if (len >= max_text_len) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GmErrorCborOverflow, 
                             "Text too long: %zu bytes, buffer size %zu", len, max_text_len)
        };
    }

    if (!check_read_bounds(*offset, len, max_size)) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GmErrorCborBufferTooSmall, 
                             "Buffer underrun reading %zu bytes", len)
        };
    }

    memcpy(text, buf + *offset, len);
    text[len] = '\0';
    *offset += len;

    return (gm_result_void_t){.ok = true};
}

/* Write CBOR unsigned integer with bounds checking */
gm_result_size_t gm_cbor_write_uint(uint64_t value, uint8_t *buf, size_t buf_size) {
    if (!buf) {
        return (gm_result_size_t){
            .ok = false,
            .u.err = GM_ERROR(GmErrorCborInvalidData, "NULL buffer")
        };
    }

    size_t required_size = 0;
    
    if (value < CBOR_IMMEDIATE_THRESHOLD) {
        required_size = 1;
    } else if (value <= UINT8_MAX) {
        required_size = 2;
    } else if (value <= UINT16_MAX) {
        required_size = 3;
    } else if (value <= UINT32_MAX) {
        required_size = CborUint32HeaderSize;
    } else {
        required_size = CborUint64HeaderSize;
    }

    if (buf_size < required_size) {
        return (gm_result_size_t){
            .ok = false,
            .u.err = GM_ERROR(GmErrorCborBufferTooSmall, 
                             "Need %zu bytes, have %zu", required_size, buf_size)
        };
    }

    if (value < CBOR_IMMEDIATE_THRESHOLD) {
        buf[0] = CBOR_TYPE_UNSIGNED | (uint8_t)value;
        return (gm_result_size_t){.ok = true, .u.val = 1};
    }
    
    if (value <= UINT8_MAX) {
        buf[0] = CBOR_TYPE_UNSIGNED | CBOR_UINT8_FOLLOWS;
        buf[1] = (uint8_t)value;
        return (gm_result_size_t){.ok = true, .u.val = 2};
    }
    
    if (value <= UINT16_MAX) {
        buf[0] = CBOR_TYPE_UNSIGNED | CBOR_UINT16_FOLLOWS;
        buf[1] = (uint8_t)((value >> SHIFT_8) & BYTE_MASK);
        buf[2] = (uint8_t)(value & BYTE_MASK);
        return (gm_result_size_t){.ok = true, .u.val = 3};
    }
    
    if (value <= UINT32_MAX) {
        buf[0] = CBOR_TYPE_UNSIGNED | CBOR_UINT32_FOLLOWS;
        buf[1] = (uint8_t)((value >> SHIFT_24) & BYTE_MASK);
        buf[2] = (uint8_t)((value >> SHIFT_16) & BYTE_MASK);
        buf[3] = (uint8_t)((value >> SHIFT_8) & BYTE_MASK);
        buf[4] = (uint8_t)(value & BYTE_MASK);
        return (gm_result_size_t){.ok = true, .u.val = CborUint32HeaderSize};
    }
    
    buf[0] = CBOR_TYPE_UNSIGNED | CBOR_UINT64_FOLLOWS;
    for (int i = 0; i < BYTE_SIZE; i++) {
        buf[1 + i] = (uint8_t)((value >> (SHIFT_56 - i * BYTE_SIZE)) & BYTE_MASK);
    }
    return (gm_result_size_t){.ok = true, .u.val = CborUint64HeaderSize};
}

/* Write CBOR byte string with bounds checking */
gm_result_size_t gm_cbor_write_bytes(uint8_t *buf, size_t buf_size,
                                     const uint8_t *data, size_t data_len) {
    if (!buf || !data) {
        return (gm_result_size_t){
            .ok = false,
            .u.err = GM_ERROR(GmErrorCborInvalidData, "NULL buffer or data")
        };
    }

    size_t header_size = 0;
    
    if (data_len < CBOR_IMMEDIATE_THRESHOLD) {
        header_size = 1;
    } else if (data_len <= UINT8_MAX) {
        header_size = 2;
    } else if (data_len <= UINT16_MAX) {
        header_size = 3;
    } else {
        return (gm_result_size_t){
            .ok = false,
            .u.err = GM_ERROR(GmErrorCborOverflow, 
                             "Data length %zu exceeds maximum", data_len)
        };
    }

    size_t total_size = header_size + data_len;
    if (buf_size < total_size) {
        return (gm_result_size_t){
            .ok = false,
            .u.err = GM_ERROR(GmErrorCborBufferTooSmall, 
                             "Need %zu bytes, have %zu", total_size, buf_size)
        };
    }

    if (data_len < CBOR_IMMEDIATE_THRESHOLD) {
        buf[0] = CBOR_TYPE_BYTES | (uint8_t)data_len;
    } else if (data_len <= UINT8_MAX) {
        buf[0] = CBOR_TYPE_BYTES | CBOR_UINT8_FOLLOWS;
        buf[1] = (uint8_t)data_len;
    } else {
        buf[0] = CBOR_TYPE_BYTES | CBOR_UINT16_FOLLOWS;
        buf[1] = (uint8_t)((data_len >> SHIFT_8) & BYTE_MASK);
        buf[2] = (uint8_t)(data_len & BYTE_MASK);
    }

    memcpy(buf + header_size, data, data_len);
    return (gm_result_size_t){.ok = true, .u.val = total_size};
}

/* Write CBOR text string with bounds checking */
gm_result_size_t gm_cbor_write_text(uint8_t *buf, size_t buf_size, const char *text) {
    if (!buf || !text) {
        return (gm_result_size_t){
            .ok = false,
            .u.err = GM_ERROR(GmErrorCborInvalidData, "NULL buffer or text")
        };
    }

    size_t text_len = strlen(text);
    size_t header_size = 0;
    
    if (text_len < CBOR_IMMEDIATE_THRESHOLD) {
        header_size = 1;
    } else if (text_len <= UINT8_MAX) {
        header_size = 2;
    } else if (text_len <= UINT16_MAX) {
        header_size = 3;
    } else {
        return (gm_result_size_t){
            .ok = false,
            .u.err = GM_ERROR(GmErrorCborOverflow, 
                             "Text length %zu exceeds maximum", text_len)
        };
    }

    size_t total_size = header_size + text_len;
    if (buf_size < total_size) {
        return (gm_result_size_t){
            .ok = false,
            .u.err = GM_ERROR(GmErrorCborBufferTooSmall, 
                             "Need %zu bytes, have %zu", total_size, buf_size)
        };
    }

    if (text_len < CBOR_IMMEDIATE_THRESHOLD) {
        buf[0] = CBOR_TYPE_TEXT | (uint8_t)text_len;
    } else if (text_len <= UINT8_MAX) {
        buf[0] = CBOR_TYPE_TEXT | CBOR_UINT8_FOLLOWS;
        buf[1] = (uint8_t)text_len;
    } else {
        buf[0] = CBOR_TYPE_TEXT | CBOR_UINT16_FOLLOWS;
        buf[1] = (uint8_t)((text_len >> SHIFT_8) & BYTE_MASK);
        buf[2] = (uint8_t)(text_len & BYTE_MASK);
    }

    /* CBOR text strings have explicit length and don't require null termination */
    memcpy(buf + header_size, text, text_len);
    return (gm_result_size_t){.ok = true, .u.val = total_size};
}
