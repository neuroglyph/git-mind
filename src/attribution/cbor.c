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
 * Decode CBOR to attributed edge
 */
int gm_edge_attributed_decode_cbor(const uint8_t *buffer, size_t len, 
                                  gm_edge_attributed_t *edge) {
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
    
    /* TODO: Implement full decoder for all 13 fields */
    /* For now, this is a stub that validates the format */
    (void)end;  /* Mark as used to avoid warning */
    
    return 0;
}