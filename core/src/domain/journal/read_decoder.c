/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/journal/internal/read_decoder.h"

#include <string.h>

#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/security/memory.h"
#include "gitmind/security/string.h"

static gm_result_void_t make_buffer_error(const char *field) {
    return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL,
                                "journal edge conversion overflow (%s)", field));
}

static gm_result_void_t convert_legacy_to_attributed(const gm_edge_t *legacy,
                                                     gm_edge_attributed_t *attributed) {
    if (legacy == NULL || attributed == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "edge conversion requires outputs"));
    }
    gm_memset_safe(attributed, sizeof(*attributed), 0, sizeof(*attributed));
    if (gm_memcpy_span(attributed->src_sha, sizeof(attributed->src_sha),
                       legacy->src_sha, GM_SHA1_SIZE) != 0) {
        return make_buffer_error("src_sha");
    }
    if (gm_memcpy_span(attributed->tgt_sha, sizeof(attributed->tgt_sha),
                       legacy->tgt_sha, GM_SHA1_SIZE) != 0) {
        return make_buffer_error("tgt_sha");
    }
    attributed->src_oid = legacy->src_oid;
    attributed->tgt_oid = legacy->tgt_oid;
    attributed->rel_type = legacy->rel_type;
    attributed->confidence = legacy->confidence;
    attributed->timestamp = legacy->timestamp;
    if (gm_strcpy_safe(attributed->src_path, sizeof(attributed->src_path),
                       legacy->src_path) != 0) {
        return make_buffer_error("src_path");
    }
    if (gm_strcpy_safe(attributed->tgt_path, sizeof(attributed->tgt_path),
                       legacy->tgt_path) != 0) {
        return make_buffer_error("tgt_path");
    }
    if (gm_strcpy_safe(attributed->ulid, sizeof(attributed->ulid),
                       legacy->ulid) != 0) {
        return make_buffer_error("ulid");
    }
    attributed->attribution.source_type = GM_SOURCE_HUMAN;
    attributed->lane = GM_LANE_DEFAULT;
    return gm_ok_void();
}

static gm_result_void_t convert_attributed_to_basic(const gm_edge_attributed_t *aedge,
                                                    gm_edge_t *basic) {
    if (aedge == NULL || basic == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "edge conversion requires outputs"));
    }
    gm_memset_safe(basic, sizeof(*basic), 0, sizeof(*basic));
    if (gm_memcpy_span(basic->src_sha, sizeof(basic->src_sha),
                       aedge->src_sha, GM_SHA1_SIZE) != 0) {
        return make_buffer_error("src_sha");
    }
    if (gm_memcpy_span(basic->tgt_sha, sizeof(basic->tgt_sha),
                       aedge->tgt_sha, GM_SHA1_SIZE) != 0) {
        return make_buffer_error("tgt_sha");
    }
    basic->src_oid = aedge->src_oid;
    basic->tgt_oid = aedge->tgt_oid;
    basic->rel_type = aedge->rel_type;
    basic->confidence = aedge->confidence;
    basic->timestamp = aedge->timestamp;
    if (gm_strcpy_safe(basic->src_path, sizeof(basic->src_path),
                       aedge->src_path) != 0) {
        return make_buffer_error("src_path");
    }
    if (gm_strcpy_safe(basic->tgt_path, sizeof(basic->tgt_path),
                       aedge->tgt_path) != 0) {
        return make_buffer_error("tgt_path");
    }
    if (gm_strcpy_safe(basic->ulid, sizeof(basic->ulid),
                       aedge->ulid) != 0) {
        return make_buffer_error("ulid");
    }
    return gm_ok_void();
}

GM_NODISCARD gm_result_void_t gm_journal_decode_edge(const uint8_t *buf,
                                                     size_t len,
                                                     bool prefer_attributed,
                                                     gm_edge_t *out_basic,
                                                     gm_edge_attributed_t *out_attr,
                                                     size_t *consumed,
                                                     bool *got_attr) {
    if (buf == NULL || len == 0U) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, "decode edge requires buffers"));
    }
    if (consumed != NULL) {
        *consumed = 0U;
    }
    if (got_attr != NULL) {
        *got_attr = false;
    }

    if (prefer_attributed) {
        size_t c = 0; gm_edge_attributed_t a = {0};
        int rc = gm_edge_attributed_decode_cbor_ex(buf, len, &a, &c);
        if (rc == GM_OK && c > 0) {
            if (out_attr == NULL) {
                return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                            "attributed output missing"));
            }
            *out_attr = a;
            if (consumed != NULL) *consumed = c;
            if (got_attr != NULL) *got_attr = true;
            return gm_ok_void();
        }
        /* fallback to legacy */
        gm_edge_t e = {0}; c = 0;
        rc = gm_edge_decode_cbor_ex(buf, len, &e, &c);
        if (rc != GM_OK || c == 0) {
            return gm_err_void(GM_ERROR(GM_ERR_INVALID_FORMAT, "invalid edge"));
        }
        if (out_attr == NULL) {
            return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                        "attributed output missing"));
        }
        gm_result_void_t conv = convert_legacy_to_attributed(&e, out_attr);
        if (!conv.ok) return conv;
        if (consumed != NULL) *consumed = c;
        if (got_attr != NULL) *got_attr = true;
        return gm_ok_void();
    }

    /* prefer regular */
    size_t c = 0; gm_edge_t e = {0};
    int rc = gm_edge_decode_cbor_ex(buf, len, &e, &c);
    if (rc == GM_OK && c > 0) {
        if (out_basic == NULL) {
            return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                        "basic output missing"));
        }
        *out_basic = e;
        if (consumed != NULL) *consumed = c;
        if (got_attr != NULL) *got_attr = false;
        return gm_ok_void();
    }
    gm_edge_attributed_t a = {0}; c = 0;
    rc = gm_edge_attributed_decode_cbor_ex(buf, len, &a, &c);
    if (rc != GM_OK || c == 0) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_FORMAT, "invalid edge"));
    }
    if (out_basic == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "basic output missing"));
    }
    gm_result_void_t conv = convert_attributed_to_basic(&a, out_basic);
    if (!conv.ok) return conv;
    if (consumed != NULL) *consumed = c;
    if (got_attr != NULL) *got_attr = false;
    return gm_ok_void();
}
