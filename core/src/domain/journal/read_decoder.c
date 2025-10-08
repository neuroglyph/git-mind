/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/journal/internal/read_decoder.h"

#include <string.h>

#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/security/memory.h"
#include "gitmind/security/string.h"

static void convert_legacy_to_attributed(const gm_edge_t *legacy,
                                         gm_edge_attributed_t *attributed) {
    for (size_t i = 0; i < GM_SHA1_SIZE; i++) {
        attributed->src_sha[i] = legacy->src_sha[i];
        attributed->tgt_sha[i] = legacy->tgt_sha[i];
    }
    attributed->src_oid = legacy->src_oid;
    attributed->tgt_oid = legacy->tgt_oid;
    attributed->rel_type = legacy->rel_type;
    attributed->confidence = legacy->confidence;
    attributed->timestamp = legacy->timestamp;

    size_t src_len = strlen(legacy->src_path);
    if (src_len >= sizeof(attributed->src_path)) src_len = sizeof(attributed->src_path) - 1U;
    for (size_t i = 0; i < src_len; i++) { attributed->src_path[i] = legacy->src_path[i]; }
    attributed->src_path[src_len] = '\0';

    size_t tgt_len = strlen(legacy->tgt_path);
    if (tgt_len >= sizeof(attributed->tgt_path)) tgt_len = sizeof(attributed->tgt_path) - 1U;
    for (size_t i = 0; i < tgt_len; i++) { attributed->tgt_path[i] = legacy->tgt_path[i]; }
    attributed->tgt_path[tgt_len] = '\0';

    size_t ulid_len = strlen(legacy->ulid);
    if (ulid_len >= sizeof(attributed->ulid)) ulid_len = sizeof(attributed->ulid) - 1U;
    for (size_t i = 0; i < ulid_len; i++) { attributed->ulid[i] = legacy->ulid[i]; }
    attributed->ulid[ulid_len] = '\0';

    attributed->attribution.source_type = GM_SOURCE_HUMAN;
    attributed->attribution.author[0] = '\0';
    attributed->attribution.session_id[0] = '\0';
    attributed->attribution.flags = 0;
    attributed->lane = GM_LANE_DEFAULT;
}

static void convert_attributed_to_basic(const gm_edge_attributed_t *aedge,
                                        gm_edge_t *basic) {
    gm_memset_safe(basic, sizeof(*basic), 0, sizeof(*basic));
    (void)gm_memcpy_span(basic->src_sha, GM_SHA1_SIZE, aedge->src_sha, GM_SHA1_SIZE);
    (void)gm_memcpy_span(basic->tgt_sha, GM_SHA1_SIZE, aedge->tgt_sha, GM_SHA1_SIZE);
    basic->src_oid = aedge->src_oid;
    basic->tgt_oid = aedge->tgt_oid;
    basic->rel_type = aedge->rel_type;
    basic->confidence = aedge->confidence;
    basic->timestamp = aedge->timestamp;
    (void)gm_strcpy_safe(basic->src_path, GM_PATH_MAX, aedge->src_path);
    (void)gm_strcpy_safe(basic->tgt_path, GM_PATH_MAX, aedge->tgt_path);
    (void)gm_strcpy_safe(basic->ulid, GM_ULID_SIZE + 1, aedge->ulid);
}

GM_NODISCARD gm_result_void_t gm_journal_decode_edge(const uint8_t *buf,
                                                     size_t len,
                                                     bool prefer_attributed,
                                                     gm_edge_t *out_basic,
                                                     gm_edge_attributed_t *out_attr,
                                                     size_t *consumed,
                                                     bool *got_attr) {
    if (buf == NULL || len == 0U || consumed == NULL || got_attr == NULL ||
        out_basic == NULL || out_attr == NULL) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, "decode edge requires buffers"));
    }

    *got_attr = false;
    *consumed = 0U;

    if (prefer_attributed) {
        size_t c = 0; gm_edge_attributed_t a = {0};
        int rc = gm_edge_attributed_decode_cbor_ex(buf, len, &a, &c);
        if (rc == GM_OK && c > 0) {
            *out_attr = a; *consumed = c; *got_attr = true; return gm_ok_void();
        }
        /* fallback to legacy */
        gm_edge_t e = {0}; c = 0;
        rc = gm_edge_decode_cbor_ex(buf, len, &e, &c);
        if (rc != GM_OK || c == 0) {
            return gm_err_void(GM_ERROR(GM_ERR_INVALID_FORMAT, "invalid edge"));
        }
        convert_legacy_to_attributed(&e, out_attr);
        *consumed = c; *got_attr = true; return gm_ok_void();
    }

    /* prefer regular */
    size_t c = 0; gm_edge_t e = {0};
    int rc = gm_edge_decode_cbor_ex(buf, len, &e, &c);
    if (rc == GM_OK && c > 0) {
        *out_basic = e; *consumed = c; *got_attr = false; return gm_ok_void();
    }
    gm_edge_attributed_t a = {0}; c = 0;
    rc = gm_edge_attributed_decode_cbor_ex(buf, len, &a, &c);
    if (rc != GM_OK || c == 0) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_FORMAT, "invalid edge"));
    }
    convert_attributed_to_basic(&a, out_basic);
    *consumed = c; *got_attr = false; return gm_ok_void();
}

