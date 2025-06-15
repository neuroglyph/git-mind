// SPDX-License-Identifier: Apache-2.0
// © 2025 J. Kirby Ross / Neuroglyph Collective

#include "gitmind.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

// SHA-1 implementation
// Based on FIPS 180-1

#define SHA1_BLOCK_SIZE 64

typedef struct {
    uint32_t state[5];
    uint64_t count;
    uint8_t buffer[64];
} SHA1_CTX;

// Rotate left
#define ROL(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

// SHA-1 functions
#define F(x, y, z) (((x) & (y)) | (~(x) & (z)))
#define G(x, y, z) ((x) ^ (y) ^ (z))
#define H(x, y, z) (((x) & (y)) | ((x) & (z)) | ((y) & (z)))

// Initialize SHA-1 context
static void sha1_init(SHA1_CTX* ctx) {
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
    ctx->state[4] = 0xC3D2E1F0;
    ctx->count = 0;
}

// Process a single block
static void sha1_transform(SHA1_CTX* ctx, const uint8_t* data) {
    uint32_t W[80];
    uint32_t a, b, c, d, e;
    uint32_t temp;
    int i;
    
    // Prepare message schedule
    for (i = 0; i < 16; i++) {
        W[i] = ((uint32_t)data[i * 4] << 24) |
               ((uint32_t)data[i * 4 + 1] << 16) |
               ((uint32_t)data[i * 4 + 2] << 8) |
               ((uint32_t)data[i * 4 + 3]);
    }
    
    for (i = 16; i < 80; i++) {
        W[i] = ROL(W[i-3] ^ W[i-8] ^ W[i-14] ^ W[i-16], 1);
    }
    
    // Initialize working variables
    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    
    // Main loop
    for (i = 0; i < 20; i++) {
        temp = ROL(a, 5) + F(b, c, d) + e + W[i] + 0x5A827999;
        e = d;
        d = c;
        c = ROL(b, 30);
        b = a;
        a = temp;
    }
    
    for (i = 20; i < 40; i++) {
        temp = ROL(a, 5) + G(b, c, d) + e + W[i] + 0x6ED9EBA1;
        e = d;
        d = c;
        c = ROL(b, 30);
        b = a;
        a = temp;
    }
    
    for (i = 40; i < 60; i++) {
        temp = ROL(a, 5) + H(b, c, d) + e + W[i] + 0x8F1BBCDC;
        e = d;
        d = c;
        c = ROL(b, 30);
        b = a;
        a = temp;
    }
    
    for (i = 60; i < 80; i++) {
        temp = ROL(a, 5) + G(b, c, d) + e + W[i] + 0xCA62C1D6;
        e = d;
        d = c;
        c = ROL(b, 30);
        b = a;
        a = temp;
    }
    
    // Update state
    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
}

// Update SHA-1 with data
static void sha1_update(SHA1_CTX* ctx, const uint8_t* data, size_t len) {
    size_t index = (size_t)(ctx->count & 63);
    ctx->count += len;
    
    // Process any pending bytes
    if (index) {
        size_t left = SHA1_BLOCK_SIZE - index;
        if (len < left) {
            memcpy(&ctx->buffer[index], data, len);
            return;
        }
        memcpy(&ctx->buffer[index], data, left);
        sha1_transform(ctx, ctx->buffer);
        data += left;
        len -= left;
    }
    
    // Process full blocks
    while (len >= SHA1_BLOCK_SIZE) {
        sha1_transform(ctx, data);
        data += SHA1_BLOCK_SIZE;
        len -= SHA1_BLOCK_SIZE;
    }
    
    // Save remaining bytes
    if (len) {
        memcpy(ctx->buffer, data, len);
    }
}

// Finalize SHA-1
static void sha1_final(SHA1_CTX* ctx, uint8_t* digest) {
    size_t index = (size_t)(ctx->count & 63);
    uint64_t bit_count = ctx->count * 8;
    
    // Pad message
    ctx->buffer[index++] = 0x80;
    
    if (index > 56) {
        memset(&ctx->buffer[index], 0, SHA1_BLOCK_SIZE - index);
        sha1_transform(ctx, ctx->buffer);
        index = 0;
    }
    
    memset(&ctx->buffer[index], 0, 56 - index);
    
    // Append bit count (big-endian)
    ctx->buffer[56] = (uint8_t)(bit_count >> 56);
    ctx->buffer[57] = (uint8_t)(bit_count >> 48);
    ctx->buffer[58] = (uint8_t)(bit_count >> 40);
    ctx->buffer[59] = (uint8_t)(bit_count >> 32);
    ctx->buffer[60] = (uint8_t)(bit_count >> 24);
    ctx->buffer[61] = (uint8_t)(bit_count >> 16);
    ctx->buffer[62] = (uint8_t)(bit_count >> 8);
    ctx->buffer[63] = (uint8_t)bit_count;
    
    sha1_transform(ctx, ctx->buffer);
    
    // Output digest (big-endian)
    for (int i = 0; i < 5; i++) {
        digest[i * 4] = (uint8_t)(ctx->state[i] >> 24);
        digest[i * 4 + 1] = (uint8_t)(ctx->state[i] >> 16);
        digest[i * 4 + 2] = (uint8_t)(ctx->state[i] >> 8);
        digest[i * 4 + 3] = (uint8_t)ctx->state[i];
    }
}

// Public function: compute SHA-1 of a string
int gm_sha1_string(const char* content, char* out_sha) {
    SHA1_CTX ctx;
    uint8_t digest[20];
    
    sha1_init(&ctx);
    sha1_update(&ctx, (const uint8_t*)content, strlen(content));
    sha1_final(&ctx, digest);
    
    // Convert to hex string
    for (int i = 0; i < 20; i++) {
        sprintf(out_sha + i * 2, "%02x", digest[i]);
    }
    out_sha[40] = '\0';
    
    return GM_OK;
}
