/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

/*
 * DFA-based UTF-8 decoder/validator
 *
 * Based on Björn Höhrmann's DFA decoder
 * (http://bjoern.hoehrmann.de/utf-8/decoder/dfa/) Original implementation is in
 * the public domain.
 *
 * This validator rejects:
 * - Overlong encodings
 * - UTF-16 surrogates (U+D800-U+DFFF)
 * - Codepoints > U+10FFFF
 * - Invalid byte sequences
 */

#include "gitmind/utf8/validate.h"

#include <stdint.h>

/* DFA states */
#define UTF8_ACCEPT 0
#define UTF8_REJECT 12

/* Force inline for performance */
#ifdef _MSC_VER
#define GM_ALWAYS_INLINE __forceinline
#else
#define GM_ALWAYS_INLINE __attribute__((always_inline)) inline
#endif

/*
 * UTF-8 DFA lookup table
 *
 * First 256 entries: maps bytes to character classes
 * Remaining entries: state transitions
 */
static const uint8_t UTF8D[] = {
    /* Byte classification (0-255) */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, /* 00-1F: ASCII control */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, /* 20-3F: ASCII printable */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, /* 40-5F: ASCII printable */
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0, /* 60-7F: ASCII printable */
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    1,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9,
    9, /* 80-9F: Continuation bytes */
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7,
    7, /* A0-BF: Continuation bytes */
    8,
    8,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2,
    2, /* C0-DF: 2-byte sequences */
    10,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    3,
    4,
    3,
    3,
    11,
    6,
    6,
    6,
    5,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8,
    8, /* E0-FF: 3-4 byte sequences */

    /* State transition table */
    0,
    12,
    24,
    36,
    60,
    96,
    84,
    12,
    12,
    12,
    48,
    72,
    12,
    12,
    12,
    12,
    12,
    12,
    12,
    12,
    12,
    12,
    12,
    12,
    12,
    0,
    12,
    12,
    12,
    12,
    12,
    0,
    12,
    0,
    12,
    12,
    12,
    24,
    12,
    12,
    12,
    12,
    12,
    24,
    12,
    24,
    12,
    12,
    12,
    12,
    12,
    12,
    12,
    12,
    12,
    24,
    12,
    12,
    12,
    12,
    12,
    24,
    12,
    12,
    12,
    12,
    12,
    12,
    12,
    24,
    12,
    12,
    12,
    12,
    12,
    12,
    12,
    12,
    12,
    36,
    12,
    36,
    12,
    12,
    12,
    36,
    12,
    12,
    12,
    12,
    12,
    36,
    12,
    36,
    12,
    12,
    12,
    36,
    12,
    12,
    12,
    12,
    12,
    12,
    12,
    12,
    12,
    12,
};

/**
 * @brief Decode one UTF-8 byte
 *
 * Updates state and codepoint based on input byte.
 *
 * @param state Current state (modified)
 * @param codep Current codepoint (modified)
 * @param byte Input byte
 * @return New state (UTF8_ACCEPT, UTF8_REJECT, or intermediate)
 */
static GM_ALWAYS_INLINE uint32_t decode(uint32_t *state, uint32_t *codep,
                                        uint8_t byte) {
    uint32_t type = UTF8D[byte];

    *codep = (*state != UTF8_ACCEPT) ? (byte & 0x3fu) | (*codep << 6)
                                     : (0xff >> type) & (byte);

    *state = UTF8D[256 + *state + type];
    return *state;
}

/**
 * @brief Map DFA state to error code
 */
static gm_utf8_error_t state_to_error(uint32_t state, uint32_t codep) {
    if (state == UTF8_ACCEPT) {
        return GM_UTF8_OK;
    }

    /* Check for specific error conditions */
    if (state == UTF8_REJECT) {
        /* Check for surrogate */
        if (codep >= 0xD800 && codep <= 0xDFFF) {
            return GM_UTF8_ERR_SURROGATE;
        }
        /* Check for out of range */
        if (codep > 0x10FFFF) {
            return GM_UTF8_ERR_OUT_OF_RANGE;
        }
        /* Default to invalid start */
        return GM_UTF8_ERR_INVALID_START;
    }

    /* State != ACCEPT and != REJECT means truncated */
    return GM_UTF8_ERR_TRUNCATED;
}

gm_utf8_error_t gm_utf8_validate(const char *buf, size_t len) {
    uint32_t state = UTF8_ACCEPT;
    uint32_t codep = 0;

    for (size_t i = 0; i < len; i++) {
        uint32_t prev_state = state;
        decode(&state, &codep, (uint8_t)buf[i]);

        if (state == UTF8_REJECT) {
            /* Fast fail on first error */
            return state_to_error(state, codep);
        }

        /* Check for completed codepoint */
        if (prev_state != UTF8_ACCEPT && state == UTF8_ACCEPT) {
            /* We just completed a codepoint, check if it's valid */
            if (codep >= 0xD800 && codep <= 0xDFFF) {
                return GM_UTF8_ERR_SURROGATE;
            }
            if (codep > 0x10FFFF) {
                return GM_UTF8_ERR_OUT_OF_RANGE;
            }
        }
    }

    return state_to_error(state, codep);
}

void gm_utf8_state_init(gm_utf8_state_t *s) {
    s->state = UTF8_ACCEPT;
    s->codep = 0;
}

gm_utf8_error_t gm_utf8_validate_chunk(gm_utf8_state_t *s, const char *buf,
                                       size_t len) {
    for (size_t i = 0; i < len; i++) {
        uint32_t prev_state = s->state;
        decode(&s->state, &s->codep, (uint8_t)buf[i]);

        if (s->state == UTF8_REJECT) {
            /* Fast fail on first error */
            return state_to_error(s->state, s->codep);
        }

        /* Check for completed codepoint */
        if (prev_state != UTF8_ACCEPT && s->state == UTF8_ACCEPT) {
            /* We just completed a codepoint, check if it's valid */
            if (s->codep >= 0xD800 && s->codep <= 0xDFFF) {
                return GM_UTF8_ERR_SURROGATE;
            }
            if (s->codep > 0x10FFFF) {
                return GM_UTF8_ERR_OUT_OF_RANGE;
            }
        }
    }

    /* Chunk is valid so far */
    return GM_UTF8_OK;
}

bool gm_utf8_state_is_complete(const gm_utf8_state_t *s) {
    return s->state == UTF8_ACCEPT;
}