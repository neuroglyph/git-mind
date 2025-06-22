/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind.h"
#include "gitmind/cbor_common.h"
#include "gitmind/constants_cbor.h"
#include "../util/gm_mem.h"

#include <string.h>

/*
 * Extended CBOR decoder with consumed bytes tracking
 * Follows strict SRP, DI principles with pedantic quality
 */

/* Validate CBOR array header (SRP: validate header only) */
/* NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - buffer_len vs expected_size */
static int validate_array_header(const uint8_t *buffer, size_t buffer_len,
                                size_t expected_size) {
    if (buffer_len < 1) {
        return GM_INVALID_ARG;
    }

    uint8_t header = buffer[0];
    if ((header & CBOR_TYPE_MASK) != CBOR_TYPE_ARRAY ||
        (header & CBOR_ADDITIONAL_INFO_MASK) != expected_size) {
        return GM_INVALID_ARG;
    }

    return GM_OK;
}

/* Decode single SHA field (SRP: decode one SHA) */
static int decode_sha_field(const uint8_t *buffer, size_t *offset,
                           uint8_t *sha) {
    return gm_cbor_read_bytes(buffer, offset, sha, GM_SHA1_SIZE);
}

/* Decode relationship type (SRP: decode one field) */
static int decode_rel_type(const uint8_t *buffer, size_t *offset,
                          uint16_t *rel_type) {
    uint64_t temp;
    if (gm_cbor_read_uint(buffer, offset, &temp) != GM_OK) {
        return GM_INVALID_ARG;
    }
    *rel_type = (uint16_t)temp;
    return GM_OK;
}

/* Decode confidence (SRP: decode one field) */
static int decode_confidence(const uint8_t *buffer, size_t *offset,
                            uint16_t *confidence) {
    uint64_t temp;
    if (gm_cbor_read_uint(buffer, offset, &temp) != GM_OK) {
        return GM_INVALID_ARG;
    }
    *confidence = (uint16_t)temp;
    return GM_OK;
}

/* Decode timestamp (SRP: decode one field) */
static int decode_timestamp(const uint8_t *buffer, size_t *offset,
                           uint64_t *timestamp) {
    return gm_cbor_read_uint(buffer, offset, timestamp);
}

/* Decode single path (SRP: decode one path) */
static int decode_path(const uint8_t *buffer, size_t *offset,
                      char *path) {
    return gm_cbor_read_text(buffer, offset, path, GM_PATH_MAX);
}

/* Initialize edge structure (SRP: initialization only) */
static void init_edge(gm_edge_t *edge) {
    gm_memset(edge, 0, sizeof(gm_edge_t));
}

/* Helper: Decode all SHAs */
static int decode_all_shas(const uint8_t *buffer, size_t *offset, 
                          gm_edge_t *edge) {
    int result = decode_sha_field(buffer, offset, edge->src_sha);
    if (result != GM_OK) {
        return result;
    }
    return decode_sha_field(buffer, offset, edge->tgt_sha);
}

/* Helper: Decode all metadata */
static int decode_all_metadata(const uint8_t *buffer, size_t *offset,
                              gm_edge_t *edge) {
    int result = decode_rel_type(buffer, offset, &edge->rel_type);
    if (result != GM_OK) {
        return result;
    }
    
    result = decode_confidence(buffer, offset, &edge->confidence);
    if (result != GM_OK) {
        return result;
    }
    
    return decode_timestamp(buffer, offset, &edge->timestamp);
}

/* Helper: Decode all paths */
static int decode_all_paths(const uint8_t *buffer, size_t *offset,
                           gm_edge_t *edge) {
    int result = decode_path(buffer, offset, edge->src_path);
    if (result != GM_OK) {
        return result;
    }
    return decode_path(buffer, offset, edge->tgt_path);
}

/* Main decoder with consumed bytes tracking */
int gm_edge_decode_cbor_ex(const uint8_t *buffer, size_t len, gm_edge_t *edge,
                          size_t *consumed) {
    if (!buffer || !edge || !consumed || len == 0) {
        return GM_INVALID_ARG;
    }

    size_t offset = 0;
    init_edge(edge);

    /* Validate and decode */
    int result = validate_array_header(buffer, len, CBOR_ARRAY_SIZE_EDGE);
    if (result != GM_OK) {
        return result;
    }
    offset++;

    if ((result = decode_all_shas(buffer, &offset, edge)) != GM_OK ||
        (result = decode_all_metadata(buffer, &offset, edge)) != GM_OK ||
        (result = decode_all_paths(buffer, &offset, edge)) != GM_OK) {
        return result;
    }

    *consumed = offset;
    return GM_OK;
}