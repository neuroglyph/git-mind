/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <gitmind/cbor/cbor.h>
#include <gitmind/cbor/constants_cbor.h>
#include <gitmind/error.h>

#include <string.h>

/* Error code constants */
static const int GM_ERROR_CBOR_TYPE_MISMATCH = 6001;
static const int GM_ERROR_CBOR_BUFFER_TOO_SMALL = 6002;
static const int GM_ERROR_CBOR_INVALID_DATA = 6003;
static const int GM_ERROR_CBOR_OVERFLOW = 6004;

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

/* Read CBOR unsigned integer with bounds checking */
gm_result_uint64_t gm_cbor_read_uint(const uint8_t *buf, size_t *offset, size_t max_size) {
    if (!buf || !offset) {
        return (gm_result_uint64_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_CBOR_INVALID_DATA, "NULL buffer or offset")
        };
    }

    if (!check_read_bounds(*offset, 1, max_size)) {
        return (gm_result_uint64_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_CBOR_BUFFER_TOO_SMALL, "Buffer underrun reading type byte")
        };
    }

    uint8_t initial = buf[(*offset)++];
    uint8_t type = initial & CBOR_TYPE_MASK;
    uint8_t info = initial & CBOR_ADDITIONAL_INFO_MASK;
    uint64_t value = 0;

    if (type != CBOR_TYPE_UNSIGNED) {
        return (gm_result_uint64_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_CBOR_TYPE_MISMATCH, 
                             "Expected unsigned integer, got type 0x%02x", type)
        };
    }

    if (info < CBOR_IMMEDIATE_THRESHOLD) {
        value = info;
    } else if (info == CBOR_UINT8_FOLLOWS) {
        if (!check_read_bounds(*offset, 1, max_size)) {
            return (gm_result_uint64_t){
                .ok = false,
                .u.err = GM_ERROR(GM_ERROR_CBOR_BUFFER_TOO_SMALL, "Buffer underrun reading uint8")
            };
        }
        value = buf[(*offset)++];
    } else if (info == CBOR_UINT16_FOLLOWS) {
        if (!check_read_bounds(*offset, 2, max_size)) {
            return (gm_result_uint64_t){
                .ok = false,
                .u.err = GM_ERROR(GM_ERROR_CBOR_BUFFER_TOO_SMALL, "Buffer underrun reading uint16")
            };
        }
        value = (uint64_t)(((uint16_t)buf[*offset] << SHIFT_8) | (uint16_t)buf[*offset + 1]);
        *offset += 2;
    } else if (info == CBOR_UINT32_FOLLOWS) {
        if (!check_read_bounds(*offset, 4, max_size)) {
            return (gm_result_uint64_t){
                .ok = false,
                .u.err = GM_ERROR(GM_ERROR_CBOR_BUFFER_TOO_SMALL, "Buffer underrun reading uint32")
            };
        }
        value = ((uint32_t)buf[*offset] << SHIFT_24) |
                ((uint32_t)buf[*offset + 1] << SHIFT_16) |
                ((uint32_t)buf[*offset + 2] << SHIFT_8) | 
                (uint32_t)buf[*offset + 3];
        *offset += 4;
    } else if (info == CBOR_UINT64_FOLLOWS) {
        if (!check_read_bounds(*offset, 8, max_size)) {
            return (gm_result_uint64_t){
                .ok = false,
                .u.err = GM_ERROR(GM_ERROR_CBOR_BUFFER_TOO_SMALL, "Buffer underrun reading uint64")
            };
        }
        for (int i = 0; i < BYTE_SIZE; i++) {
            value = (value << SHIFT_8) | buf[(*offset)++];
        }
    } else {
        return (gm_result_uint64_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_CBOR_INVALID_DATA, 
                             "Invalid additional info: 0x%02x", info)
        };
    }

    return (gm_result_uint64_t){.ok = true, .u.val = value};
}

/* Read CBOR byte string with bounds checking */
gm_result_void_t gm_cbor_read_bytes(const uint8_t *buf, size_t *offset, size_t max_size,
                                    uint8_t *data, size_t expected_len) {
    if (!buf || !offset || !data) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_CBOR_INVALID_DATA, "NULL buffer, offset, or data")
        };
    }

    if (!check_read_bounds(*offset, 1, max_size)) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_CBOR_BUFFER_TOO_SMALL, "Buffer underrun reading type byte")
        };
    }

    uint8_t initial = buf[(*offset)++];
    uint8_t type = initial & CBOR_TYPE_MASK;
    uint8_t info = initial & CBOR_ADDITIONAL_INFO_MASK;
    size_t len = 0;

    if (type != CBOR_TYPE_BYTES) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_CBOR_TYPE_MISMATCH, 
                             "Expected byte string, got type 0x%02x", type)
        };
    }

    if (info < CBOR_IMMEDIATE_THRESHOLD) {
        len = info;
    } else if (info == CBOR_UINT8_FOLLOWS) {
        if (!check_read_bounds(*offset, 1, max_size)) {
            return (gm_result_void_t){
                .ok = false,
                .u.err = GM_ERROR(GM_ERROR_CBOR_BUFFER_TOO_SMALL, "Buffer underrun reading length")
            };
        }
        len = buf[(*offset)++];
    } else if (info == CBOR_UINT16_FOLLOWS) {
        if (!check_read_bounds(*offset, 2, max_size)) {
            return (gm_result_void_t){
                .ok = false,
                .u.err = GM_ERROR(GM_ERROR_CBOR_BUFFER_TOO_SMALL, "Buffer underrun reading length")
            };
        }
        len = ((size_t)buf[*offset] << SHIFT_8) | (size_t)buf[*offset + 1];
        *offset += 2;
    } else {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_CBOR_INVALID_DATA, 
                             "Invalid additional info: 0x%02x", info)
        };
    }

    if (len != expected_len) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_CBOR_INVALID_DATA, 
                             "Length mismatch: expected %zu, got %zu", expected_len, len)
        };
    }

    if (!check_read_bounds(*offset, len, max_size)) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_CBOR_BUFFER_TOO_SMALL, 
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
            .u.err = GM_ERROR(GM_ERROR_CBOR_INVALID_DATA, "NULL buffer, offset, or text")
        };
    }

    if (max_text_len == 0) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_CBOR_INVALID_DATA, "Text buffer size is zero")
        };
    }

    if (!check_read_bounds(*offset, 1, max_size)) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_CBOR_BUFFER_TOO_SMALL, "Buffer underrun reading type byte")
        };
    }

    uint8_t initial = buf[(*offset)++];
    uint8_t type = initial & CBOR_TYPE_MASK;
    uint8_t info = initial & CBOR_ADDITIONAL_INFO_MASK;
    size_t len = 0;

    if (type != CBOR_TYPE_TEXT) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_CBOR_TYPE_MISMATCH, 
                             "Expected text string, got type 0x%02x", type)
        };
    }

    if (info < CBOR_IMMEDIATE_THRESHOLD) {
        len = info;
    } else if (info == CBOR_UINT8_FOLLOWS) {
        if (!check_read_bounds(*offset, 1, max_size)) {
            return (gm_result_void_t){
                .ok = false,
                .u.err = GM_ERROR(GM_ERROR_CBOR_BUFFER_TOO_SMALL, "Buffer underrun reading length")
            };
        }
        len = buf[(*offset)++];
    } else if (info == CBOR_UINT16_FOLLOWS) {
        if (!check_read_bounds(*offset, 2, max_size)) {
            return (gm_result_void_t){
                .ok = false,
                .u.err = GM_ERROR(GM_ERROR_CBOR_BUFFER_TOO_SMALL, "Buffer underrun reading length")
            };
        }
        len = ((size_t)buf[*offset] << SHIFT_8) | (size_t)buf[*offset + 1];
        *offset += 2;
    } else {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_CBOR_INVALID_DATA, 
                             "Invalid additional info: 0x%02x", info)
        };
    }

    if (len >= max_text_len) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_CBOR_OVERFLOW, 
                             "Text too long: %zu bytes, buffer size %zu", len, max_text_len)
        };
    }

    if (!check_read_bounds(*offset, len, max_size)) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_CBOR_BUFFER_TOO_SMALL, 
                             "Buffer underrun reading %zu bytes", len)
        };
    }

    memcpy(text, buf + *offset, len);
    text[len] = '\0';
    *offset += len;

    return (gm_result_void_t){.ok = true};
}

/* Write CBOR unsigned integer with bounds checking */
gm_result_size_t gm_cbor_write_uint(uint8_t *buf, size_t buf_size, uint64_t value) {
    if (!buf) {
        return (gm_result_size_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_CBOR_INVALID_DATA, "NULL buffer")
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
        required_size = 5;
    } else {
        required_size = 9;
    }

    if (buf_size < required_size) {
        return (gm_result_size_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_CBOR_BUFFER_TOO_SMALL, 
                             "Need %zu bytes, have %zu", required_size, buf_size)
        };
    }

    if (value < CBOR_IMMEDIATE_THRESHOLD) {
        buf[0] = CBOR_TYPE_UNSIGNED | (uint8_t)value;
        return (gm_result_size_t){.ok = true, .u.val = 1};
    } else if (value <= UINT8_MAX) {
        buf[0] = CBOR_TYPE_UNSIGNED | CBOR_UINT8_FOLLOWS;
        buf[1] = (uint8_t)value;
        return (gm_result_size_t){.ok = true, .u.val = 2};
    } else if (value <= UINT16_MAX) {
        buf[0] = CBOR_TYPE_UNSIGNED | CBOR_UINT16_FOLLOWS;
        buf[1] = (uint8_t)((value >> SHIFT_8) & BYTE_MASK);
        buf[2] = (uint8_t)(value & BYTE_MASK);
        return (gm_result_size_t){.ok = true, .u.val = 3};
    } else if (value <= UINT32_MAX) {
        buf[0] = CBOR_TYPE_UNSIGNED | CBOR_UINT32_FOLLOWS;
        buf[1] = (uint8_t)((value >> SHIFT_24) & BYTE_MASK);
        buf[2] = (uint8_t)((value >> SHIFT_16) & BYTE_MASK);
        buf[3] = (uint8_t)((value >> SHIFT_8) & BYTE_MASK);
        buf[4] = (uint8_t)(value & BYTE_MASK);
        return (gm_result_size_t){.ok = true, .u.val = 5};
    } else {
        buf[0] = CBOR_TYPE_UNSIGNED | CBOR_UINT64_FOLLOWS;
        for (int i = 0; i < BYTE_SIZE; i++) {
            buf[1 + i] = (uint8_t)((value >> (SHIFT_56 - i * BYTE_SIZE)) & BYTE_MASK);
        }
        return (gm_result_size_t){.ok = true, .u.val = 9};
    }
}

/* Write CBOR byte string with bounds checking */
gm_result_size_t gm_cbor_write_bytes(uint8_t *buf, size_t buf_size,
                                     const uint8_t *data, size_t data_len) {
    if (!buf || !data) {
        return (gm_result_size_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_CBOR_INVALID_DATA, "NULL buffer or data")
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
            .u.err = GM_ERROR(GM_ERROR_CBOR_OVERFLOW, 
                             "Data length %zu exceeds maximum", data_len)
        };
    }

    size_t total_size = header_size + data_len;
    if (buf_size < total_size) {
        return (gm_result_size_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_CBOR_BUFFER_TOO_SMALL, 
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
            .u.err = GM_ERROR(GM_ERROR_CBOR_INVALID_DATA, "NULL buffer or text")
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
            .u.err = GM_ERROR(GM_ERROR_CBOR_OVERFLOW, 
                             "Text length %zu exceeds maximum", text_len)
        };
    }

    size_t total_size = header_size + text_len;
    if (buf_size < total_size) {
        return (gm_result_size_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_CBOR_BUFFER_TOO_SMALL, 
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

    memcpy(buf + header_size, text, text_len);
    return (gm_result_size_t){.ok = true, .u.val = total_size};
}
