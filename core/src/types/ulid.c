/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp,readability-identifier-naming) - POSIX feature test macro */
#define _POSIX_C_SOURCE 199309L
#include "gitmind/types/ulid.h"

#include <stdint.h>
#include <string.h>
#include <time.h>

#include "gitmind/crypto/backend.h"
#include "gitmind/crypto/random.h"
#include "gitmind/error.h"
#include "gitmind/result.h"

/* Crockford's Base32 alphabet (excludes I, L, O, U to avoid confusion) */
static const char ENCODING[32] = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";

/* ULID structure constants */
#define TIME_COMPONENT_LENGTH 10
#define RANDOM_COMPONENT_LENGTH 16
#define BASE32_MASK 0x1FU
#define BASE32_SHIFT 5
#define BITS_PER_CHARACTER 5
#define MILLISECONDS_PER_SECOND 1000
#define NANOSECONDS_PER_MILLISECOND 1000000
#define BITS_PER_BYTE 8
#define RANDOM_BYTES_COUNT 10
#define MAX_TIMESTAMP_VALUE 16

/* Decoding table for validation (-1 for invalid characters) */
static const int DECODING[256] = {
    /* 0-31: Control characters */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    /* 32-47: Space and punctuation */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    /* 48-57: '0'-'9' */
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, -1, -1, -1, -1, -1, -1,
    /* 64-79: '@', 'A'-'O' (excluding I, L, O) */
    -1, 10, 11, 12, 13, 14, 15, 16, 17, -1, 18, 19, -1, 20, 21, -1,
    /* 80-95: 'P'-'_' */
    22, 23, 24, 25, 26, -1, 27, 28, 29, 30, 31, -1, -1, -1, -1, -1,
    /* 96-111: '`', 'a'-'o' (lowercase) */
    -1, 10, 11, 12, 13, 14, 15, 16, 17, -1, 18, 19, -1, 20, 21, -1,
    /* 112-127: 'p'-DEL */
    22, 23, 24, 25, 26, -1, 27, 28, 29, 30, 31, -1, -1, -1, -1, -1,
    /* 128-255: Extended ASCII */
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

/* Get current time in milliseconds */
static uint64_t get_current_time_ms(void) {
    struct timespec timestamp;
    /* NOLINTNEXTLINE(misc-include-cleaner) - CLOCK_REALTIME from time.h with _POSIX_C_SOURCE */
    if (clock_gettime(CLOCK_REALTIME, &timestamp) != 0) {
        /* Fallback to time() if clock_gettime fails */
        return (uint64_t)time(NULL) * MILLISECONDS_PER_SECOND;
    }
    return ((uint64_t)timestamp.tv_sec * MILLISECONDS_PER_SECOND) +
           ((uint64_t)timestamp.tv_nsec / NANOSECONDS_PER_MILLISECOND);
}

/* Encode time component (48 bits) into 10 base32 characters */
static void encode_time(uint64_t time_ms, char *output) {
    /* Encode from right to left (least significant first) */
    for (int index = TIME_COMPONENT_LENGTH - 1; index >= 0; index--) {
        output[index] = ENCODING[time_ms & BASE32_MASK];
        time_ms >>= BASE32_SHIFT;
    }
}

/* Encode random component (80 bits) into 16 base32 characters */
static gm_result_void_t encode_random(char *output) {
    uint8_t random_bytes[RANDOM_BYTES_COUNT]; /* 80 bits = 10 bytes */
    
    /* Get random bytes using default backend */
    const gm_crypto_backend_t *backend = gm_crypto_backend_libsodium();
    gm_result_crypto_context_t ctx_result = gm_crypto_context_create(backend);
    if (!ctx_result.ok) {
        return (gm_result_void_t){.ok = false, .u.err = ctx_result.u.err};
    }
    
    gm_result_void_t result = gm_random_bytes_with_context(&ctx_result.u.val, random_bytes, sizeof(random_bytes));
    if (!result.ok) {
        return result;
    }
    
    /* Convert 80 bits to base32 
     * We need to handle bit shifting across byte boundaries */
    uint32_t bits_available = 0;
    uint32_t bit_buffer = 0;
    size_t byte_index = 0;
    
    for (int char_index = 0; char_index < RANDOM_COMPONENT_LENGTH; char_index++) {
        /* Ensure we have at least 5 bits */
        while (bits_available < BITS_PER_CHARACTER && byte_index < sizeof(random_bytes)) {
            bit_buffer = (bit_buffer << BITS_PER_BYTE) | random_bytes[byte_index++];
            bits_available += BITS_PER_BYTE;
        }
        
        /* Extract 5 bits */
        bits_available -= BITS_PER_CHARACTER;
        output[char_index] = ENCODING[(bit_buffer >> bits_available) & BASE32_MASK];
    }
    
    return gm_ok_void();
}

/* Public API implementation */

gm_result_ulid_t gm_ulid_generate(char *buffer) {
    if (!buffer) {
        gm_error_t *err = GM_ERROR(GM_ERR_INVALID_ARGUMENT, "Buffer cannot be NULL");
        return (gm_result_ulid_t){.ok = false, .u.err = err};
    }
    
    uint64_t timestamp_ms = get_current_time_ms();
    return gm_ulid_generate_with_timestamp(buffer, timestamp_ms);
}

gm_result_ulid_t gm_ulid_generate_with_timestamp(char *buffer,
                                                  uint64_t timestamp_ms) {
    if (!buffer) {
        gm_error_t *err = GM_ERROR(GM_ERR_INVALID_ARGUMENT, "Buffer cannot be NULL");
        return (gm_result_ulid_t){.ok = false, .u.err = err};
    }
    
    /* Encode time component */
    encode_time(timestamp_ms, buffer);
    
    /* Encode random component */
    gm_result_void_t random_result = encode_random(buffer + TIME_COMPONENT_LENGTH);
    if (!random_result.ok) {
        return (gm_result_ulid_t){.ok = false, .u.err = random_result.u.err};
    }
    
    /* Null terminate */
    buffer[GM_ULID_SIZE] = '\0';
    
    return (gm_result_ulid_t){.ok = true, .u.val = buffer};
}

bool gm_ulid_is_valid(const char *ulid) {
    if (!ulid) {
        return false;
    }
    
    /* Check length */
    size_t length = 0;
    while (ulid[length] != '\0' && length <= GM_ULID_SIZE) {
        length++;
    }
    if (length != GM_ULID_SIZE) {
        return false;
    }
    
    /* Validate each character */
    for (size_t index = 0; index < GM_ULID_SIZE; index++) {
        unsigned char character = (unsigned char)ulid[index];
        if (DECODING[character] < 0) {
            return false;
        }
    }
    
    /* Check timestamp doesn't overflow (first 10 chars = 50 bits, max 48 bits) */
    unsigned char first_char = (unsigned char)ulid[0];
    int first_value = DECODING[first_char];
    /* Top 2 bits must be 0 for 48-bit value */
    return first_value < MAX_TIMESTAMP_VALUE;
}

gm_result_void_t gm_ulid_get_timestamp(const char *ulid,
                                       uint64_t *timestamp_ms) {
    if (!ulid || !timestamp_ms) {
        gm_error_t *err = GM_ERROR(GM_ERR_INVALID_ARGUMENT, "Parameters cannot be NULL");
        return (gm_result_void_t){.ok = false, .u.err = err};
    }
    
    if (!gm_ulid_is_valid(ulid)) {
        gm_error_t *err = GM_ERROR(GM_ERR_INVALID_ARGUMENT, "Invalid ULID format");
        return (gm_result_void_t){.ok = false, .u.err = err};
    }
    
    /* Decode time component */
    uint64_t time_value = 0;
    for (int index = 0; index < TIME_COMPONENT_LENGTH; index++) {
        unsigned char character = (unsigned char)ulid[index];
        int value = DECODING[character];
        time_value = (time_value << BASE32_SHIFT) | (uint64_t)value;
    }
    
    *timestamp_ms = time_value;
    return gm_ok_void();
}

int gm_ulid_compare(const char *ulid1, const char *ulid2) {
    if (!ulid1 && !ulid2) {
        return 0;
    }
    if (!ulid1) {
        return -1;
    }
    if (!ulid2) {
        return 1;
    }
    
    /* ULIDs are lexicographically sortable */
    return strcmp(ulid1, ulid2);
}
