/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <string.h>
#include "gitmind/attribution.h"

/* CBOR encoding tags */
#define CBOR_TYPE_ARRAY     0x80
#define CBOR_TYPE_BYTES     0x40
#define CBOR_TYPE_TEXT      0x60
#define CBOR_TYPE_UINT      0x00
#define CBOR_ADDITIONAL_1   0x01
#define CBOR_ADDITIONAL_2   0x02
#define CBOR_ADDITIONAL_4   0x04
#define CBOR_ADDITIONAL_8   0x08

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
    
    /* Array with 13 elements */
    if (remaining < 1) return -1;
    *p++ = CBOR_TYPE_ARRAY | 13;
    remaining--;
    
    /* 1. Source SHA (bytes) */
    if (remaining < 22) return -1;
    *p++ = CBOR_TYPE_BYTES | 20;
    memcpy(p, edge->src_sha, 20);
    p += 20;
    remaining -= 21;
    
    /* 2. Target SHA (bytes) */
    if (remaining < 22) return -1;
    *p++ = CBOR_TYPE_BYTES | 20;
    memcpy(p, edge->tgt_sha, 20);
    p += 20;
    remaining -= 21;
    
    /* 3. Relationship type (uint16) */
    if (remaining < 3) return -1;
    *p++ = CBOR_TYPE_UINT | CBOR_ADDITIONAL_2;
    *p++ = (edge->rel_type >> 8) & 0xFF;
    *p++ = edge->rel_type & 0xFF;
    remaining -= 3;
    
    /* 4. Confidence (uint16) */
    if (remaining < 3) return -1;
    *p++ = CBOR_TYPE_UINT | CBOR_ADDITIONAL_2;
    *p++ = (edge->confidence >> 8) & 0xFF;
    *p++ = edge->confidence & 0xFF;
    remaining -= 3;
    
    /* 5. Timestamp (uint64) */
    if (remaining < 9) return -1;
    *p++ = CBOR_TYPE_UINT | CBOR_ADDITIONAL_8;
    for (int i = 7; i >= 0; i--) {
        *p++ = (edge->timestamp >> (i * 8)) & 0xFF;
    }
    remaining -= 9;
    
    /* 6. Source path (text) */
    size_t src_len = strlen(edge->src_path);
    if (src_len > 255 || remaining < src_len + 2) return -1;
    *p++ = CBOR_TYPE_TEXT | (src_len & 0x1F);
    memcpy(p, edge->src_path, src_len);
    p += src_len;
    remaining -= (src_len + 1);
    
    /* 7. Target path (text) */
    size_t tgt_len = strlen(edge->tgt_path);
    if (tgt_len > 255 || remaining < tgt_len + 2) return -1;
    *p++ = CBOR_TYPE_TEXT | (tgt_len & 0x1F);
    memcpy(p, edge->tgt_path, tgt_len);
    p += tgt_len;
    remaining -= (tgt_len + 1);
    
    /* 8. ULID (text) */
    if (remaining < 28) return -1;
    *p++ = CBOR_TYPE_TEXT | 26;
    memcpy(p, edge->ulid, 26);
    p += 26;
    remaining -= 27;
    
    /* 9. Source type (uint8) */
    if (remaining < 2) return -1;
    *p++ = CBOR_TYPE_UINT | CBOR_ADDITIONAL_1;
    *p++ = edge->attribution.source_type;
    remaining -= 2;
    
    /* 10. Author (text) */
    size_t author_len = strlen(edge->attribution.author);
    if (author_len > 63 || remaining < author_len + 2) return -1;
    *p++ = CBOR_TYPE_TEXT | (author_len & 0x1F);
    memcpy(p, edge->attribution.author, author_len);
    p += author_len;
    remaining -= (author_len + 1);
    
    /* 11. Session ID (text) */
    size_t session_len = strlen(edge->attribution.session_id);
    if (session_len > 31 || remaining < session_len + 2) return -1;
    *p++ = CBOR_TYPE_TEXT | (session_len & 0x1F);
    memcpy(p, edge->attribution.session_id, session_len);
    p += session_len;
    remaining -= (session_len + 1);
    
    /* 12. Flags (uint32) */
    if (remaining < 5) return -1;
    *p++ = CBOR_TYPE_UINT | CBOR_ADDITIONAL_4;
    *p++ = (edge->attribution.flags >> 24) & 0xFF;
    *p++ = (edge->attribution.flags >> 16) & 0xFF;
    *p++ = (edge->attribution.flags >> 8) & 0xFF;
    *p++ = edge->attribution.flags & 0xFF;
    remaining -= 5;
    
    /* 13. Lane (uint8) */
    if (remaining < 2) return -1;
    *p++ = CBOR_TYPE_UINT | CBOR_ADDITIONAL_1;
    *p++ = edge->lane;
    
    *len = p - buffer;
    return 0;
}

/**
 * Decode CBOR to attributed edge (with consumed bytes)
 */
int gm_edge_attributed_decode_cbor_ex(const uint8_t *buffer, size_t len, 
                                     gm_edge_attributed_t *edge, size_t *consumed) {
    if (!buffer || !edge || len < 1) {
        return -1;
    }
    
    const uint8_t *p = buffer;
    const uint8_t *end = buffer + len;
    
    /* Ensure we have enough data */
    if (p >= end) {
        return -1;
    }
    
    /* Check array header */
    if (*p != (CBOR_TYPE_ARRAY | 13)) {
        /* Try legacy format (8 elements) */
        if (*p == (CBOR_TYPE_ARRAY | 8)) {
            /* Decode legacy format and set defaults */
            /* TODO: Implement legacy decoder */
            return -1;
        }
        return -1;
    }
    p++;
    
    /* 1. Source SHA (bytes) */
    if (p + 21 > end || *p != (CBOR_TYPE_BYTES | 20)) {
        return -1;
    }
    p++;
    memcpy(edge->src_sha, p, 20);
    p += 20;
    
    /* 2. Target SHA (bytes) */
    if (p + 21 > end || *p != (CBOR_TYPE_BYTES | 20)) {
        return -1;
    }
    p++;
    memcpy(edge->tgt_sha, p, 20);
    p += 20;
    
    /* 3. Relationship type (uint16) */
    if (p + 3 > end || *p != (CBOR_TYPE_UINT | CBOR_ADDITIONAL_2)) {
        return -1;
    }
    p++;
    edge->rel_type = ((uint16_t)p[0] << 8) | p[1];
    p += 2;
    
    /* 4. Confidence (uint16) */
    if (p + 3 > end || *p != (CBOR_TYPE_UINT | CBOR_ADDITIONAL_2)) {
        return -1;
    }
    p++;
    edge->confidence = ((uint16_t)p[0] << 8) | p[1];
    p += 2;
    
    /* 5. Timestamp (uint64) */
    if (p + 9 > end || *p != (CBOR_TYPE_UINT | CBOR_ADDITIONAL_8)) {
        return -1;
    }
    p++;
    edge->timestamp = 0;
    for (int i = 0; i < 8; i++) {
        edge->timestamp = (edge->timestamp << 8) | p[i];
    }
    p += 8;
    
    /* 6. Source path (text) */
    if (p >= end || (*p & 0xE0) != CBOR_TYPE_TEXT) {
        return -1;
    }
    size_t src_len = *p & 0x1F;
    p++;
    if (p + src_len > end || src_len >= sizeof(edge->src_path)) {
        return -1;
    }
    memcpy(edge->src_path, p, src_len);
    edge->src_path[src_len] = '\0';
    p += src_len;
    
    /* 7. Target path (text) */
    if (p >= end || (*p & 0xE0) != CBOR_TYPE_TEXT) {
        return -1;
    }
    size_t tgt_len = *p & 0x1F;
    p++;
    if (p + tgt_len > end || tgt_len >= sizeof(edge->tgt_path)) {
        return -1;
    }
    memcpy(edge->tgt_path, p, tgt_len);
    edge->tgt_path[tgt_len] = '\0';
    p += tgt_len;
    
    /* 8. ULID (text) */
    if (p >= end || *p != (CBOR_TYPE_TEXT | 26)) {
        return -1;
    }
    p++;
    if (p + 26 > end) {
        return -1;
    }
    memcpy(edge->ulid, p, 26);
    edge->ulid[26] = '\0';
    p += 26;
    
    /* 9. Source type (uint8) */
    if (p + 2 > end || *p != (CBOR_TYPE_UINT | CBOR_ADDITIONAL_1)) {
        return -1;
    }
    p++;
    edge->attribution.source_type = *p;
    p++;
    
    /* 10. Author (text) */
    if (p >= end || (*p & 0xE0) != CBOR_TYPE_TEXT) {
        return -1;
    }
    size_t author_len = *p & 0x1F;
    p++;
    if (p + author_len > end || author_len >= sizeof(edge->attribution.author)) {
        return -1;
    }
    memcpy(edge->attribution.author, p, author_len);
    edge->attribution.author[author_len] = '\0';
    p += author_len;
    
    /* 11. Session ID (text) */
    if (p >= end || (*p & 0xE0) != CBOR_TYPE_TEXT) {
        return -1;
    }
    size_t session_len = *p & 0x1F;
    p++;
    if (p + session_len > end || session_len >= sizeof(edge->attribution.session_id)) {
        return -1;
    }
    memcpy(edge->attribution.session_id, p, session_len);
    edge->attribution.session_id[session_len] = '\0';
    p += session_len;
    
    /* 12. Flags (uint32) */
    if (p + 5 > end || *p != (CBOR_TYPE_UINT | CBOR_ADDITIONAL_4)) {
        return -1;
    }
    p++;
    edge->attribution.flags = ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | 
                             ((uint32_t)p[2] << 8) | p[3];
    p += 4;
    
    /* 13. Lane (uint8) */
    if (p + 2 > end || *p != (CBOR_TYPE_UINT | CBOR_ADDITIONAL_1)) {
        return -1;
    }
    p++;
    edge->lane = *p;
    p++;
    
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