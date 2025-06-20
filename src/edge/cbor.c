/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind.h"

#include "gitmind/cbor_common.h"
#include "gitmind/constants_cbor.h"

#include <string.h>

/* Use common CBOR functions from cbor_common.h */

/* cbor_write_* functions moved to cbor_common.c */

/* Encode edge to CBOR */
int gm_edge_encode_cbor(const gm_edge_t *edge, uint8_t *buffer, size_t *len) {
    if (!edge || !buffer || !len) {
        return GM_INVALID_ARG;
    }

    size_t offset = 0;

    /* Array header with 7 elements */
    buffer[offset++] = CBOR_TYPE_ARRAY | CBOR_ARRAY_SIZE_EDGE;

    /* 1. Source SHA */
    offset += gm_cbor_write_bytes(buffer + offset, edge->src_sha, GM_SHA1_SIZE);

    /* 2. Target SHA */
    offset += gm_cbor_write_bytes(buffer + offset, edge->tgt_sha, GM_SHA1_SIZE);

    /* 3. Relationship type */
    offset += gm_cbor_write_uint(buffer + offset, edge->rel_type);

    /* 4. Confidence */
    offset += gm_cbor_write_uint(buffer + offset, edge->confidence);

    /* 5. Timestamp */
    offset += gm_cbor_write_uint(buffer + offset, edge->timestamp);

    /* 6. Source path */
    offset += gm_cbor_write_text(buffer + offset, edge->src_path);

    /* 7. Target path */
    offset += gm_cbor_write_text(buffer + offset, edge->tgt_path);

    *len = offset;
    return GM_OK;
}

/* cbor_read_* functions moved to cbor_common.c */

/* Decode CBOR to edge */
int gm_edge_decode_cbor(const uint8_t *buffer, size_t len, gm_edge_t *edge) {
    if (!buffer || !edge || len == 0) {
        return GM_INVALID_ARG;
    }

    size_t offset = 0;

    /* Initialize edge structure */
    memset(edge, 0, sizeof(gm_edge_t));

    /* Need at least one byte for array header */
    if (len < 1) {
        return GM_INVALID_ARG;
    }

    /* Check array header */
    /* Check array header */
    if (buffer[offset] != (CBOR_TYPE_ARRAY | CBOR_ARRAY_SIZE_EDGE)) {
        return GM_INVALID_ARG;
    }
    offset++;

    /* 1. Source SHA */
    if (gm_cbor_read_bytes(buffer, &offset, edge->src_sha, GM_SHA1_SIZE) !=
        GM_OK) {
        return GM_INVALID_ARG;
    }

    /* 2. Target SHA */
    if (gm_cbor_read_bytes(buffer, &offset, edge->tgt_sha, GM_SHA1_SIZE) !=
        GM_OK) {
        return GM_INVALID_ARG;
    }

    /* 3. Relationship type */
    uint64_t rel_type;
    if (gm_cbor_read_uint(buffer, &offset, &rel_type) != GM_OK) {
        return GM_INVALID_ARG;
    }
    edge->rel_type = (uint16_t)rel_type;

    /* 4. Confidence */
    uint64_t confidence;
    if (gm_cbor_read_uint(buffer, &offset, &confidence) != GM_OK) {
        return GM_INVALID_ARG;
    }
    edge->confidence = (uint16_t)confidence;

    /* 5. Timestamp */
    if (gm_cbor_read_uint(buffer, &offset, &edge->timestamp) != GM_OK) {
        return GM_INVALID_ARG;
    }

    /* 6. Source path */
    if (gm_cbor_read_text(buffer, &offset, edge->src_path, GM_PATH_MAX) !=
        GM_OK) {
        return GM_INVALID_ARG;
    }

    /* 7. Target path */
    if (gm_cbor_read_text(buffer, &offset, edge->tgt_path, GM_PATH_MAX) !=
        GM_OK) {
        return GM_INVALID_ARG;
    }

    return GM_OK;
}