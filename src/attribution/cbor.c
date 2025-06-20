/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/attribution.h"
#include "gitmind/constants_cbor.h"
#include "gitmind/constants_internal.h"

#include <string.h>

/* Use centralized CBOR constants from constants_cbor.h */
/* Additional local constants */
#define CBOR_ADDITIONAL_1 0x01
#define CBOR_ADDITIONAL_2 0x02
#define CBOR_ADDITIONAL_4 0x04
#define CBOR_ADDITIONAL_8 0x08

/* Size constants for CBOR encoding */
#define CBOR_HEADER_SIZE 1
#define CBOR_UINT8_SIZE 2
#define CBOR_UINT16_SIZE 3
#define CBOR_UINT32_SIZE 5
#define CBOR_UINT64_SIZE 9
#define BYTES_IN_UINT16 2
#define BYTES_IN_UINT32 4
#define BYTES_IN_UINT64 8

/* Local aliases for clarity */
#define SHA_SIZE SHA_BYTES_SIZE
#define ULID_SIZE CBOR_ULID_SIZE
#define MAX_PATH_LEN 255
#define MAX_AUTHOR_LEN 63
#define MAX_SESSION_LEN 31

/* Helper functions for decoding */
static int decode_cbor_header(const uint8_t **p, const uint8_t *end) {
    if (*p >= end) {
        return -1;
    }

    if (**p != (CBOR_TYPE_ARRAY | CBOR_ARRAY_SIZE_ATTRIBUTED)) {
        /* Check for legacy format */
        if (**p == (CBOR_TYPE_ARRAY | CBOR_ARRAY_SIZE_LEGACY)) {
            /* TODO: Implement legacy decoder */
            return -1;
        }
        return -1;
    }
    (*p)++;
    return 0;
}

static int decode_cbor_sha(const uint8_t **p, const uint8_t *end,
                           uint8_t *sha) {
    if (*p + SHA_SIZE + CBOR_HEADER_SIZE > end ||
        **p != (CBOR_TYPE_BYTES | SHA_SIZE)) {
        return -1;
    }
    (*p)++;
    memcpy(sha, *p, SHA_SIZE);
    *p += SHA_SIZE;
    return 0;
}

static int decode_cbor_uint16(const uint8_t **p, const uint8_t *end,
                              uint16_t *value) {
    if (*p + CBOR_UINT16_SIZE > end ||
        **p != (CBOR_TYPE_UNSIGNED | CBOR_ADDITIONAL_2)) {
        return -1;
    }
    (*p)++;
    *value = ((uint16_t)(*p)[0] << SHIFT_8) | (*p)[1];
    *p += BYTES_IN_UINT16;
    return 0;
}

static int decode_cbor_uint32(const uint8_t **p, const uint8_t *end,
                              uint32_t *value) {
    if (*p + CBOR_UINT32_SIZE > end ||
        **p != (CBOR_TYPE_UNSIGNED | CBOR_ADDITIONAL_4)) {
        return -1;
    }
    (*p)++;
    *value = ((uint32_t)(*p)[0] << SHIFT_24) | ((uint32_t)(*p)[1] << SHIFT_16) |
             ((uint32_t)(*p)[2] << SHIFT_8) | (*p)[3];
    *p += BYTES_IN_UINT32;
    return 0;
}

static int decode_cbor_uint64(const uint8_t **p, const uint8_t *end,
                              uint64_t *value) {
    if (*p + CBOR_UINT64_SIZE > end ||
        **p != (CBOR_TYPE_UNSIGNED | CBOR_ADDITIONAL_8)) {
        return -1;
    }
    (*p)++;
    *value = 0;
    for (int i = 0; i < BYTES_IN_UINT64; i++) {
        *value = (*value << SHIFT_8) | (*p)[i];
    }
    *p += BYTES_IN_UINT64;
    return 0;
}

static int decode_cbor_uint8(const uint8_t **p, const uint8_t *end,
                             uint8_t *value) {
    if (*p + CBOR_UINT8_SIZE > end ||
        **p != (CBOR_TYPE_UNSIGNED | CBOR_ADDITIONAL_1)) {
        return -1;
    }
    (*p)++;
    *value = **p;
    (*p)++;
    return 0;
}

static int decode_cbor_text(const uint8_t **p, const uint8_t *end, char *buffer,
                            size_t buffer_size) {
    if (*p >= end || (**p & CBOR_TYPE_MASK) != CBOR_TYPE_TEXT) {
        return -1;
    }
    size_t len = **p & CBOR_ADDITIONAL_INFO_MASK;
    (*p)++;
    if (*p + len > end || len >= buffer_size) {
        return -1;
    }
    memcpy(buffer, *p, len);
    buffer[len] = '\0';
    *p += len;
    return 0;
}

static int decode_cbor_ulid(const uint8_t **p, const uint8_t *end, char *ulid) {
    if (*p >= end || **p != (CBOR_TYPE_TEXT | ULID_SIZE)) {
        return -1;
    }
    (*p)++;
    if (*p + ULID_SIZE > end) {
        return -1;
    }
    memcpy(ulid, *p, ULID_SIZE);
    ulid[ULID_SIZE] = '\0';
    *p += ULID_SIZE;
    return 0;
}

static int decode_cbor_shas(const uint8_t **p, const uint8_t *end,
                            gm_edge_attributed_t *edge) {
    if (decode_cbor_sha(p, end, edge->src_sha) < 0) {
        return -1;
    }
    if (decode_cbor_sha(p, end, edge->tgt_sha) < 0) {
        return -1;
    }
    return 0;
}

static int decode_cbor_metadata(const uint8_t **p, const uint8_t *end,
                                gm_edge_attributed_t *edge) {
    if (decode_cbor_uint16(p, end, &edge->rel_type) < 0) {
        return -1;
    }
    if (decode_cbor_uint16(p, end, &edge->confidence) < 0) {
        return -1;
    }
    if (decode_cbor_uint64(p, end, &edge->timestamp) < 0) {
        return -1;
    }
    return 0;
}

static int decode_cbor_paths(const uint8_t **p, const uint8_t *end,
                             gm_edge_attributed_t *edge) {
    if (decode_cbor_text(p, end, edge->src_path, sizeof(edge->src_path)) < 0) {
        return -1;
    }
    if (decode_cbor_text(p, end, edge->tgt_path, sizeof(edge->tgt_path)) < 0) {
        return -1;
    }
    if (decode_cbor_ulid(p, end, edge->ulid) < 0) {
        return -1;
    }
    return 0;
}

static int decode_cbor_attribution(const uint8_t **p, const uint8_t *end,
                                   gm_edge_attributed_t *edge) {
    uint8_t source_type;
    if (decode_cbor_uint8(p, end, &source_type) < 0) {
        return -1;
    }
    edge->attribution.source_type = (gm_source_type_t)source_type;

    if (decode_cbor_text(p, end, edge->attribution.author,
                         sizeof(edge->attribution.author)) < 0) {
        return -1;
    }
    if (decode_cbor_text(p, end, edge->attribution.session_id,
                         sizeof(edge->attribution.session_id)) < 0) {
        return -1;
    }
    if (decode_cbor_uint32(p, end, &edge->attribution.flags) < 0) {
        return -1;
    }
    return 0;
}

static int decode_cbor_lane(const uint8_t **p, const uint8_t *end,
                            gm_edge_attributed_t *edge) {
    uint8_t lane;
    if (decode_cbor_uint8(p, end, &lane) < 0) {
        return -1;
    }
    edge->lane = (gm_lane_type_t)lane;
    return 0;
}

/* Helper functions for encoding */
static int encode_cbor_header(uint8_t **p, size_t *remaining) {
    if (*remaining < CBOR_HEADER_SIZE)
        return -1;
    **p = CBOR_TYPE_ARRAY | CBOR_ARRAY_SIZE_ATTRIBUTED;
    (*p)++;
    (*remaining) -= CBOR_HEADER_SIZE;
    return 0;
}

static int encode_cbor_sha(uint8_t **p, size_t *remaining, const uint8_t *sha) {
    if (*remaining < SHA_SIZE + CBOR_HEADER_SIZE)
        return -1;
    **p = CBOR_TYPE_BYTES | SHA_SIZE;
    (*p)++;
    memcpy(*p, sha, SHA_SIZE);
    *p += SHA_SIZE;
    *remaining -= (SHA_SIZE + CBOR_HEADER_SIZE);
    return 0;
}

static int encode_cbor_uint16(uint8_t **p, size_t *remaining, uint16_t value) {
    if (*remaining < CBOR_UINT16_SIZE)
        return -1;
    **p = CBOR_TYPE_UNSIGNED | CBOR_ADDITIONAL_2;
    (*p)++;
    **p = (value >> SHIFT_8) & BYTE_MASK;
    (*p)++;
    **p = value & BYTE_MASK;
    (*p)++;
    *remaining -= CBOR_UINT16_SIZE;
    return 0;
}

static int encode_cbor_uint32(uint8_t **p, size_t *remaining, uint32_t value) {
    if (*remaining < CBOR_UINT32_SIZE)
        return -1;
    **p = CBOR_TYPE_UNSIGNED | CBOR_ADDITIONAL_4;
    (*p)++;
    **p = (value >> SHIFT_24) & BYTE_MASK;
    (*p)++;
    **p = (value >> SHIFT_16) & BYTE_MASK;
    (*p)++;
    **p = (value >> SHIFT_8) & BYTE_MASK;
    (*p)++;
    **p = value & BYTE_MASK;
    (*p)++;
    *remaining -= CBOR_UINT32_SIZE;
    return 0;
}

static int encode_cbor_uint64(uint8_t **p, size_t *remaining, uint64_t value) {
    if (*remaining < CBOR_UINT64_SIZE)
        return -1;
    **p = CBOR_TYPE_UNSIGNED | CBOR_ADDITIONAL_8;
    (*p)++;
    for (int i = BYTES_IN_UINT64 - 1; i >= 0; i--) {
        **p = (value >> (i * BYTE_SIZE)) & BYTE_MASK;
        (*p)++;
    }
    *remaining -= CBOR_UINT64_SIZE;
    return 0;
}

static int encode_cbor_uint8(uint8_t **p, size_t *remaining, uint8_t value) {
    if (*remaining < CBOR_UINT8_SIZE)
        return -1;
    **p = CBOR_TYPE_UNSIGNED | CBOR_ADDITIONAL_1;
    (*p)++;
    **p = value;
    (*p)++;
    *remaining -= CBOR_UINT8_SIZE;
    return 0;
}

static int encode_cbor_text(uint8_t **p, size_t *remaining, const char *text,
                            size_t max_len) {
    size_t len = strlen(text);
    if (len > max_len || *remaining < len + CBOR_HEADER_SIZE + 1)
        return -1;
    **p = CBOR_TYPE_TEXT | (len & CBOR_ADDITIONAL_INFO_MASK);
    (*p)++;
    memcpy(*p, text, len);
    *p += len;
    *remaining -= (len + CBOR_HEADER_SIZE);
    return 0;
}

static int encode_cbor_ulid(uint8_t **p, size_t *remaining, const char *ulid) {
    if (*remaining < ULID_SIZE + CBOR_HEADER_SIZE)
        return -1;
    **p = CBOR_TYPE_TEXT | ULID_SIZE;
    (*p)++;
    memcpy(*p, ulid, ULID_SIZE);
    *p += ULID_SIZE;
    *remaining -= (ULID_SIZE + CBOR_HEADER_SIZE);
    return 0;
}

static int encode_cbor_shas(uint8_t **p, size_t *remaining,
                            const gm_edge_attributed_t *edge) {
    if (encode_cbor_sha(p, remaining, edge->src_sha) < 0) {
        return -1;
    }
    if (encode_cbor_sha(p, remaining, edge->tgt_sha) < 0) {
        return -1;
    }
    return 0;
}

static int encode_cbor_metadata(uint8_t **p, size_t *remaining,
                                const gm_edge_attributed_t *edge) {
    if (encode_cbor_uint16(p, remaining, edge->rel_type) < 0) {
        return -1;
    }
    if (encode_cbor_uint16(p, remaining, edge->confidence) < 0) {
        return -1;
    }
    if (encode_cbor_uint64(p, remaining, edge->timestamp) < 0) {
        return -1;
    }
    return 0;
}

static int encode_cbor_paths(uint8_t **p, size_t *remaining,
                             const gm_edge_attributed_t *edge) {
    if (encode_cbor_text(p, remaining, edge->src_path, MAX_PATH_LEN) < 0) {
        return -1;
    }
    if (encode_cbor_text(p, remaining, edge->tgt_path, MAX_PATH_LEN) < 0) {
        return -1;
    }
    if (encode_cbor_ulid(p, remaining, edge->ulid) < 0) {
        return -1;
    }
    return 0;
}

static int encode_cbor_attribution(uint8_t **p, size_t *remaining,
                                   const gm_edge_attributed_t *edge) {
    if (encode_cbor_uint8(p, remaining, edge->attribution.source_type) < 0) {
        return -1;
    }
    if (encode_cbor_text(p, remaining, edge->attribution.author,
                         MAX_AUTHOR_LEN) < 0) {
        return -1;
    }
    if (encode_cbor_text(p, remaining, edge->attribution.session_id,
                         MAX_SESSION_LEN) < 0) {
        return -1;
    }
    if (encode_cbor_uint32(p, remaining, edge->attribution.flags) < 0) {
        return -1;
    }
    return 0;
}

static int encode_cbor_lane(uint8_t **p, size_t *remaining,
                            const gm_edge_attributed_t *edge) {
    return encode_cbor_uint8(p, remaining, edge->lane);
}

/**
 * Encode attributed edge to CBOR
 *
 * Format: [src_sha, tgt_sha, rel_type, confidence, timestamp,
 *          src_path, tgt_path, ulid,
 *          source_type, author, session_id, flags, lane]
 */
int gm_edge_attributed_encode_cbor(const gm_edge_attributed_t *edge,
                                   uint8_t *buffer, size_t *len) {
    if (!edge || !buffer || !len) {
        return -1;
    }

    uint8_t *p = buffer;
    size_t remaining = *len;

    /* Encode all fields */
    if (encode_cbor_header(&p, &remaining) < 0)
        return -1;
    if (encode_cbor_shas(&p, &remaining, edge) < 0)
        return -1;
    if (encode_cbor_metadata(&p, &remaining, edge) < 0)
        return -1;
    if (encode_cbor_paths(&p, &remaining, edge) < 0)
        return -1;
    if (encode_cbor_attribution(&p, &remaining, edge) < 0)
        return -1;
    if (encode_cbor_lane(&p, &remaining, edge) < 0)
        return -1;

    *len = p - buffer;
    return 0;
}

/**
 * Decode CBOR to attributed edge (with consumed bytes)
 */
int gm_edge_attributed_decode_cbor_ex(const uint8_t *buffer, size_t len,
                                      gm_edge_attributed_t *edge,
                                      size_t *consumed) {
    if (!buffer || !edge || len < 1) {
        return -1;
    }

    const uint8_t *p = buffer;
    const uint8_t *end = buffer + len;

    /* Decode all fields */
    if (decode_cbor_header(&p, end) < 0)
        return -1;
    if (decode_cbor_shas(&p, end, edge) < 0)
        return -1;
    if (decode_cbor_metadata(&p, end, edge) < 0)
        return -1;
    if (decode_cbor_paths(&p, end, edge) < 0)
        return -1;
    if (decode_cbor_attribution(&p, end, edge) < 0)
        return -1;
    if (decode_cbor_lane(&p, end, edge) < 0)
        return -1;

    /* Set consumed bytes if requested */
    if (consumed) {
        *consumed = p - buffer;
    }

    return 0;
}

/**
 * Decode CBOR to attributed edge (simple wrapper)
 */
int gm_edge_attributed_decode_cbor(const uint8_t *buffer, size_t len,
                                   gm_edge_attributed_t *edge) {
    return gm_edge_attributed_decode_cbor_ex(buffer, len, edge, NULL);
}