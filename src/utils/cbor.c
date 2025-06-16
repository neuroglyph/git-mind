// SPDX-License-Identifier: Apache-2.0
// © 2025 J. Kirby Ross / Neuroglyph Collective

#include "gitmind.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// CBOR major types
#define CBOR_MAJOR_UINT 0
#define CBOR_MAJOR_BYTES 2
#define CBOR_MAJOR_MAP 5
#define CBOR_MAJOR_FLOAT 7

// CBOR tags for our edge format
#define CBOR_TAG_TARGET_SHA 0x00
#define CBOR_TAG_CONFIDENCE 0x01
#define CBOR_TAG_TIMESTAMP 0x02
#define CBOR_TAG_EXTRA 0x03

// Write CBOR unsigned integer
static int cbor_write_uint(unsigned char* buf, size_t* pos, size_t max_len, uint64_t value) {
    if (value < 24) {
        if (*pos + 1 > max_len) return GM_ERR_MEMORY;
        buf[(*pos)++] = CBOR_MAJOR_UINT << 5 | (unsigned char)value;
    } else if (value <= 0xFF) {
        if (*pos + 2 > max_len) return GM_ERR_MEMORY;
        buf[(*pos)++] = CBOR_MAJOR_UINT << 5 | 24;
        buf[(*pos)++] = (unsigned char)value;
    } else if (value <= 0xFFFF) {
        if (*pos + 3 > max_len) return GM_ERR_MEMORY;
        buf[(*pos)++] = CBOR_MAJOR_UINT << 5 | 25;
        buf[(*pos)++] = (value >> 8) & 0xFF;
        buf[(*pos)++] = value & 0xFF;
    } else if (value <= 0xFFFFFFFF) {
        if (*pos + 5 > max_len) return GM_ERR_MEMORY;
        buf[(*pos)++] = CBOR_MAJOR_UINT << 5 | 26;
        buf[(*pos)++] = (value >> 24) & 0xFF;
        buf[(*pos)++] = (value >> 16) & 0xFF;
        buf[(*pos)++] = (value >> 8) & 0xFF;
        buf[(*pos)++] = value & 0xFF;
    } else {
        if (*pos + 9 > max_len) return GM_ERR_MEMORY;
        buf[(*pos)++] = CBOR_MAJOR_UINT << 5 | 27;
        for (int i = 7; i >= 0; i--) {
            buf[(*pos)++] = (value >> (i * 8)) & 0xFF;
        }
    }
    
    return GM_OK;
}

// Write CBOR bytes
static int cbor_write_bytes(unsigned char* buf, size_t* pos, size_t max_len, 
                           const unsigned char* data, size_t data_len) {
    // Write major type and length
    if (data_len < 24) {
        if (*pos + 1 + data_len > max_len) return GM_ERR_MEMORY;
        buf[(*pos)++] = CBOR_MAJOR_BYTES << 5 | (unsigned char)data_len;
    } else if (data_len <= 0xFF) {
        if (*pos + 2 + data_len > max_len) return GM_ERR_MEMORY;
        buf[(*pos)++] = CBOR_MAJOR_BYTES << 5 | 24;
        buf[(*pos)++] = (unsigned char)data_len;
    } else {
        return GM_ERR_MEMORY; // Too large for our use case
    }
    
    // Write data
    memcpy(buf + *pos, data, data_len);
    *pos += data_len;
    
    return GM_OK;
}

// Convert float to IEEE-754 half precision
static uint16_t float_to_half(float value) {
    union { float f; uint32_t i; } v;
    v.f = value;
    uint32_t i = v.i;
    
    int sign = (i >> 31) & 0x0001;
    int exp = (i >> 23) & 0x00FF;
    int frac = i & 0x007FFFFF;
    
    // Handle special cases
    if (exp == 0xFF) {
        // Infinity or NaN
        if (frac == 0) {
            // Infinity
            return (sign << 15) | 0x7C00;
        } else {
            // NaN
            return (sign << 15) | 0x7C00 | (frac >> 13);
        }
    } else if (exp == 0) {
        // Zero or denormalized
        return (sign << 15);
    }
    
    // Normalized number
    exp = exp - 127 + 15;  // Adjust exponent bias
    
    if (exp >= 31) {
        // Overflow to infinity
        return (sign << 15) | 0x7C00;
    } else if (exp <= 0) {
        // Underflow to zero
        return (sign << 15);
    }
    
    // Normal case
    return (sign << 15) | (exp << 10) | (frac >> 13);
}

// Write CBOR half-precision float (16-bit)
static int cbor_write_half_float(unsigned char* buf, size_t* pos, size_t max_len, float value) {
    if (*pos + 3 > max_len) return GM_ERR_MEMORY;
    
    uint16_t half = float_to_half(value);
    
    buf[(*pos)++] = CBOR_MAJOR_FLOAT << 5 | 25; // Half-precision float
    buf[(*pos)++] = (half >> 8) & 0xFF;
    buf[(*pos)++] = half & 0xFF;
    
    return GM_OK;
}

// Encode edge to CBOR
int gm_cbor_encode_edge(const char* target_sha, float confidence, 
                       time_t timestamp, unsigned char* out_buf, 
                       size_t* out_len, size_t max_len) {
    if (!target_sha || !out_buf || !out_len) {
        gm_set_error("Invalid arguments");
        return GM_ERR_INVALID_ARG;
    }
    
    size_t pos = 0;
    int ret;
    
    // Start with map (we'll count items and update later)
    if (pos >= max_len) return GM_ERR_MEMORY;
    size_t map_pos = pos;
    out_buf[pos++] = CBOR_MAJOR_MAP << 5 | 0; // Placeholder
    
    int map_items = 0;
    
    // Tag 0x00: target_sha (required)
    ret = cbor_write_uint(out_buf, &pos, max_len, CBOR_TAG_TARGET_SHA);
    if (ret != GM_OK) return ret;
    
    // Convert hex SHA to bytes (support both SHA-1 and SHA-256)
    size_t sha_len = strlen(target_sha);
    unsigned char sha_bytes[32];  // Max size for SHA-256
    size_t byte_len;
    
    if (sha_len == 40) {
        // SHA-1
        byte_len = 20;
    } else if (sha_len == 64) {
        // SHA-256
        byte_len = 32;
    } else {
        gm_set_error("Invalid SHA length: %zu", sha_len);
        return GM_ERR_INVALID_ARG;
    }
    
    for (size_t i = 0; i < byte_len; i++) {
        char hex[3] = {target_sha[i*2], target_sha[i*2+1], '\0'};
        sha_bytes[i] = (unsigned char)strtol(hex, NULL, 16);
    }
    
    ret = cbor_write_bytes(out_buf, &pos, max_len, sha_bytes, byte_len);
    if (ret != GM_OK) return ret;
    map_items++;
    
    // Tag 0x01: confidence (optional, only if not 1.0)
    if (confidence != 1.0f) {
        ret = cbor_write_uint(out_buf, &pos, max_len, CBOR_TAG_CONFIDENCE);
        if (ret != GM_OK) return ret;
        
        ret = cbor_write_half_float(out_buf, &pos, max_len, confidence);
        if (ret != GM_OK) return ret;
        map_items++;
    }
    
    // Tag 0x02: timestamp (optional, only if provided)
    if (timestamp > 0) {
        ret = cbor_write_uint(out_buf, &pos, max_len, CBOR_TAG_TIMESTAMP);
        if (ret != GM_OK) return ret;
        
        ret = cbor_write_uint(out_buf, &pos, max_len, (uint64_t)timestamp);
        if (ret != GM_OK) return ret;
        map_items++;
    }
    
    // Update map item count
    out_buf[map_pos] = CBOR_MAJOR_MAP << 5 | map_items;
    
    *out_len = pos;
    return GM_OK;
}

// Decode edge from CBOR (simplified - just extract target SHA)
int gm_cbor_decode_edge(const unsigned char* cbor_data, size_t data_len,
                       char* out_target_sha, float* out_confidence,
                       time_t* out_timestamp) {
    if (!cbor_data || !out_target_sha) {
        gm_set_error("Invalid arguments");
        return GM_ERR_INVALID_ARG;
    }
    
    size_t pos = 0;
    
    // Read map header
    if (pos >= data_len) return GM_ERR_INVALID_ARG;
    unsigned char byte = cbor_data[pos++];
    if ((byte >> 5) != CBOR_MAJOR_MAP) return GM_ERR_INVALID_ARG;
    
    int map_items = byte & 0x1F;
    if (map_items > 23) return GM_ERR_INVALID_ARG; // We don't handle extended lengths
    
    // Default values
    if (out_confidence) *out_confidence = 1.0f;
    if (out_timestamp) *out_timestamp = 0;
    
    // Read map items
    for (int i = 0; i < map_items; i++) {
        // Read tag
        if (pos >= data_len) return GM_ERR_INVALID_ARG;
        byte = cbor_data[pos++];
        if ((byte >> 5) != CBOR_MAJOR_UINT) return GM_ERR_INVALID_ARG;
        
        int tag = byte & 0x1F;
        
        switch (tag) {
            case CBOR_TAG_TARGET_SHA: {
                // Read bytes header
                if (pos >= data_len) return GM_ERR_INVALID_ARG;
                byte = cbor_data[pos++];
                if ((byte >> 5) != CBOR_MAJOR_BYTES) return GM_ERR_INVALID_ARG;
                
                int len = byte & 0x1F;
                if (len == 24) {
                    // Extended length
                    if (pos >= data_len) return GM_ERR_INVALID_ARG;
                    len = cbor_data[pos++];
                }
                
                if (len != 20 && len != 32) return GM_ERR_INVALID_ARG; // SHA1 or SHA256
                
                if (pos + len > data_len) return GM_ERR_INVALID_ARG;
                
                // Convert bytes to hex string
                for (int j = 0; j < len; j++) {
                    sprintf(out_target_sha + j*2, "%02x", cbor_data[pos + j]);
                }
                out_target_sha[len * 2] = '\0';
                pos += len;
                break;
            }
            
            case CBOR_TAG_CONFIDENCE: {
                // Skip half-float parsing for now
                pos += 3;
                break;
            }
            
            case CBOR_TAG_TIMESTAMP: {
                // Skip timestamp parsing for now
                if (pos >= data_len) return GM_ERR_INVALID_ARG;
                byte = cbor_data[pos++];
                int extra_bytes = 0;
                if ((byte & 0x1F) == 24) extra_bytes = 1;
                else if ((byte & 0x1F) == 25) extra_bytes = 2;
                else if ((byte & 0x1F) == 26) extra_bytes = 4;
                else if ((byte & 0x1F) == 27) extra_bytes = 8;
                pos += extra_bytes;
                break;
            }
            
            default:
                // Skip unknown tags for forward compatibility
                // We need to skip the value but don't know its type
                // For now, return error - TODO: implement proper skipping
                return GM_ERR_INVALID_ARG;
        }
    }
    
    return GM_OK;
}
