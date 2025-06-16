// SPDX-License-Identifier: Apache-2.0
// © 2025 J. Kirby Ross / Neuroglyph Collective

#define _POSIX_C_SOURCE 200112L
#include "gitmind.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdint.h>

// Crockford's Base32 alphabet (excluding I, L, O to avoid confusion)
static const char base32_alphabet[] = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";

// Generate random bytes
static int random_bytes(unsigned char* buf, size_t len) {
    FILE* fp = fopen("/dev/urandom", "r");
    if (fp) {
        size_t bytes_read = fread(buf, 1, len, fp);
        fclose(fp);
        if (bytes_read != len) {
            gm_set_error("Failed to read enough random bytes");
            return GM_ERR_IO;
        }
        return GM_OK;
    } else {
        // Fallback to less secure random
        static int seeded = 0;
        if (!seeded) {
            srand((unsigned int)time(NULL) ^ (unsigned int)getpid());
            seeded = 1;
        }
        for (size_t i = 0; i < len; i++) {
            buf[i] = (unsigned char)(rand() & 0xFF);
        }
        return GM_OK;
    }
}

// Encode bytes to base32
static void base32_encode(const unsigned char* in, size_t in_len, char* out) {
    int bits = 0;
    int value = 0;
    size_t out_idx = 0;
    
    for (size_t i = 0; i < in_len; i++) {
        value = (value << 8) | in[i];
        bits += 8;
        
        while (bits >= 5) {
            out[out_idx++] = base32_alphabet[(value >> (bits - 5)) & 0x1F];
            bits -= 5;
        }
    }
    
    if (bits > 0) {
        out[out_idx++] = base32_alphabet[(value << (5 - bits)) & 0x1F];
    }
    
    out[out_idx] = '\0';
}

// Generate ULID
int gm_ulid_generate(char* out_ulid) {
    if (!out_ulid) {
        gm_set_error("Invalid argument");
        return GM_ERR_INVALID_ARG;
    }
    
    // ULID = 48 bits timestamp + 80 bits randomness = 128 bits total
    unsigned char ulid_bytes[16];
    
    // Get current time in milliseconds
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    uint64_t timestamp_ms = (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    
    // Store timestamp in first 6 bytes (48 bits), big-endian
    ulid_bytes[0] = (timestamp_ms >> 40) & 0xFF;
    ulid_bytes[1] = (timestamp_ms >> 32) & 0xFF;
    ulid_bytes[2] = (timestamp_ms >> 24) & 0xFF;
    ulid_bytes[3] = (timestamp_ms >> 16) & 0xFF;
    ulid_bytes[4] = (timestamp_ms >> 8) & 0xFF;
    ulid_bytes[5] = timestamp_ms & 0xFF;
    
    // Generate 10 random bytes (80 bits)
    int ret = random_bytes(&ulid_bytes[6], 10);
    if (ret != GM_OK) return ret;
    
    // Encode to base32 (26 characters for 128 bits)
    base32_encode(ulid_bytes, 16, out_ulid);
    
    return GM_OK;
}

// Extract timestamp from ULID (for decay/GC)
int gm_ulid_timestamp(const char* ulid, time_t* out_timestamp) {
    if (!ulid || !out_timestamp) {
        gm_set_error("Invalid arguments");
        return GM_ERR_INVALID_ARG;
    }
    
    // Decode first 10 characters (50 bits, but we only need 48)
    uint64_t timestamp_ms = 0;
    
    for (int i = 0; i < 10; i++) {
        char c = ulid[i];
        int value = -1;
        
        // Find character in alphabet
        for (int j = 0; j < 32; j++) {
            if (base32_alphabet[j] == c) {
                value = j;
                break;
            }
        }
        
        if (value < 0) {
            gm_set_error("Invalid ULID format");
            return GM_ERR_INVALID_ARG;
        }
        
        timestamp_ms = (timestamp_ms << 5) | value;
    }
    
    // Right shift to get 48-bit timestamp
    timestamp_ms >>= 2;
    
    // Convert to seconds
    *out_timestamp = (time_t)(timestamp_ms / 1000);
    
    return GM_OK;
}
