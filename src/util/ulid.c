/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#define _POSIX_C_SOURCE 200809L

#include "gitmind.h"
#include "gitmind/constants_internal.h"
#include "gitmind/constants_cbor.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>

/* Crockford's Base32 alphabet */
static const char ENCODING[] = "0123456789ABCDEFGHJKMNPQRSTVWXYZ";

/* ULID constants */
#define TIME_LEN 10
#define RANDOM_LEN 16
#define ULID_LEN 26
#define BASE32_MASK 0x1F
#define BASE32_SHIFT 5

/* Get current time in milliseconds */
static uint64_t get_time_millis(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * MILLIS_PER_SECOND + ts.tv_nsec / NANOS_PER_MILLI;
}

/* Encode time component */
static void encode_time(uint64_t time, char *out) {
    for (int i = TIME_LEN - 1; i >= 0; i--) {
        out[i] = ENCODING[time & BASE32_MASK];
        time >>= BASE32_SHIFT;
    }
}

/* Encode random component */
static void encode_random(char *out) {
    for (int i = 0; i < RANDOM_LEN; i++) {
        out[i] = ENCODING[rand() & BASE32_MASK];
    }
}

/* Generate ULID */
int gm_ulid_generate(char *ulid) {
    if (!ulid) {
        return GM_INVALID_ARG;
    }
    
    /* Get current time */
    uint64_t time = get_time_millis();
    
    /* Encode time component */
    encode_time(time, ulid);
    
    /* Encode random component */
    encode_random(ulid + TIME_LEN);
    
    /* Null terminate */
    ulid[ULID_LEN] = '\0';
    
    return GM_OK;
}