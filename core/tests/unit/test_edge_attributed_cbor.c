/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "gitmind/edge_attributed.h"
#include "gitmind/cbor/cbor.h"
#include "gitmind/cbor/keys.h"
#include "gitmind/types.h"
#include "gitmind/util/oid.h"
#include "gitmind/error.h"

static void test_roundtrip_full(void) {
    printf("test_edge_attributed_roundtrip_full... ");
    gm_edge_attributed_t e = {0};
    memset(e.src_sha, 0x01, GM_SHA1_SIZE);
    memset(e.tgt_sha, 0x02, GM_SHA1_SIZE);
    e.rel_type = GM_REL_REFERENCES;
    e.confidence = 0x2000;
    e.timestamp = 1234;
    strcpy(e.src_path, "docs/A.md");
    strcpy(e.tgt_path, "src/A.c");
    strcpy(e.ulid, "01ARZ3NDEKTSV4RRFFQ69G5FAV");
    uint8_t rawA[GM_OID_RAWSZ]; memset(rawA, 0xAA, sizeof rawA);
    uint8_t rawB[GM_OID_RAWSZ]; memset(rawB, 0xBB, sizeof rawB);
    assert(gm_oid_from_raw(&e.src_oid, rawA, sizeof rawA) == GM_OK);
    assert(gm_oid_from_raw(&e.tgt_oid, rawB, sizeof rawB) == GM_OK);
    e.attribution.source_type = GM_SOURCE_AI_CLAUDE;
    strcpy(e.attribution.author, "claude@local");
    strcpy(e.attribution.session_id, "sess-1");
    e.attribution.flags = 0x5;
    e.lane = GM_LANE_ANALYSIS;

    uint8_t buf[2048]; size_t len = sizeof buf;
    gm_result_void_t enc = gm_edge_attributed_encode_cbor(&e, buf, &len);
    assert(enc.ok);
    gm_result_edge_attributed_t dec = gm_edge_attributed_decode_cbor(buf, len);
    assert(dec.ok);
    gm_edge_attributed_t d = dec.u.val;

    assert(gm_oid_equal(&e.src_oid, &d.src_oid));
    assert(gm_oid_equal(&e.tgt_oid, &d.tgt_oid));
    assert(d.rel_type == e.rel_type);
    assert(d.confidence == e.confidence);
    assert(strcmp(d.src_path, e.src_path) == 0);
    assert(strcmp(d.tgt_path, e.tgt_path) == 0);
    assert(strcmp(d.ulid, e.ulid) == 0);
    assert(d.attribution.source_type == e.attribution.source_type);
    assert(strcmp(d.attribution.author, e.attribution.author) == 0);
    assert(strcmp(d.attribution.session_id, e.attribution.session_id) == 0);
    assert(d.attribution.flags == e.attribution.flags);
    assert(d.lane == e.lane);
    printf("OK\n");
}

static void test_legacy_backfill(void) {
    printf("test_edge_attributed_legacy_backfill... ");
    /* Build CBOR with only legacy fields + minimal attribution */
    uint8_t buf[1024]; size_t off = 0; (void)buf;
    /* fields: src_sha,tgt_sha,rel,conf,ts,src_path,tgt_path,ulid, src_type,author,session,flags,lane => 13 */
    buf[off++] = (uint8_t)(0xA0 | 13);
    gm_result_size_t r;
    /* src_sha */
    r = gm_cbor_write_uint(GM_CBOR_KEY_SRC_SHA, buf + off, sizeof buf - off); assert(r.ok); off += r.u.val;
    uint8_t src_sha[GM_SHA1_SIZE]; memset(src_sha, 0x0A, sizeof src_sha);
    r = gm_cbor_write_bytes(buf + off, sizeof buf - off, src_sha, GM_SHA1_SIZE); assert(r.ok); off += r.u.val;
    /* tgt_sha */
    r = gm_cbor_write_uint(GM_CBOR_KEY_TGT_SHA, buf + off, sizeof buf - off); assert(r.ok); off += r.u.val;
    uint8_t tgt_sha[GM_SHA1_SIZE]; memset(tgt_sha, 0x0B, sizeof tgt_sha);
    r = gm_cbor_write_bytes(buf + off, sizeof buf - off, tgt_sha, GM_SHA1_SIZE); assert(r.ok); off += r.u.val;
    /* rel */
    r = gm_cbor_write_uint(GM_CBOR_KEY_REL_TYPE, buf + off, sizeof buf - off); assert(r.ok); off += r.u.val;
    r = gm_cbor_write_uint(GM_REL_IMPLEMENTS, buf + off, sizeof buf - off); assert(r.ok); off += r.u.val;
    /* conf */
    r = gm_cbor_write_uint(GM_CBOR_KEY_CONFIDENCE, buf + off, sizeof buf - off); assert(r.ok); off += r.u.val;
    r = gm_cbor_write_uint(0x3C00, buf + off, sizeof buf - off); assert(r.ok); off += r.u.val;
    /* ts */
    r = gm_cbor_write_uint(GM_CBOR_KEY_TIMESTAMP, buf + off, sizeof buf - off); assert(r.ok); off += r.u.val;
    r = gm_cbor_write_uint(9999, buf + off, sizeof buf - off); assert(r.ok); off += r.u.val;
    /* src_path */
    r = gm_cbor_write_uint(GM_CBOR_KEY_SRC_PATH, buf + off, sizeof buf - off); assert(r.ok); off += r.u.val;
    r = gm_cbor_write_text(buf + off, sizeof buf - off, "x.c"); assert(r.ok); off += r.u.val;
    /* tgt_path */
    r = gm_cbor_write_uint(GM_CBOR_KEY_TGT_PATH, buf + off, sizeof buf - off); assert(r.ok); off += r.u.val;
    r = gm_cbor_write_text(buf + off, sizeof buf - off, "y.c"); assert(r.ok); off += r.u.val;
    /* ulid */
    r = gm_cbor_write_uint(GM_CBOR_KEY_ULID, buf + off, sizeof buf - off); assert(r.ok); off += r.u.val;
    r = gm_cbor_write_text(buf + off, sizeof buf - off, "01ARZ3NDEKTSV4RRFFQ69G5FAV"); assert(r.ok); off += r.u.val;
    /* source_type */
    r = gm_cbor_write_uint(GM_CBOR_KEY_SOURCE_TYPE, buf + off, sizeof buf - off); assert(r.ok); off += r.u.val;
    r = gm_cbor_write_uint(GM_SOURCE_HUMAN, buf + off, sizeof buf - off); assert(r.ok); off += r.u.val;
    /* author */
    r = gm_cbor_write_uint(GM_CBOR_KEY_AUTHOR, buf + off, sizeof buf - off); assert(r.ok); off += r.u.val;
    r = gm_cbor_write_text(buf + off, sizeof buf - off, "me@example.com"); assert(r.ok); off += r.u.val;
    /* session */
    r = gm_cbor_write_uint(GM_CBOR_KEY_SESSION, buf + off, sizeof buf - off); assert(r.ok); off += r.u.val;
    r = gm_cbor_write_text(buf + off, sizeof buf - off, ""); assert(r.ok); off += r.u.val;
    /* flags */
    r = gm_cbor_write_uint(GM_CBOR_KEY_FLAGS, buf + off, sizeof buf - off); assert(r.ok); off += r.u.val;
    r = gm_cbor_write_uint(0, buf + off, sizeof buf - off); assert(r.ok); off += r.u.val;
    /* lane */
    r = gm_cbor_write_uint(GM_CBOR_KEY_LANE, buf + off, sizeof buf - off); assert(r.ok); off += r.u.val;
    r = gm_cbor_write_uint(GM_LANE_DEFAULT, buf + off, sizeof buf - off); assert(r.ok); off += r.u.val;

    gm_result_edge_attributed_t dec = gm_edge_attributed_decode_cbor(buf, off);
    assert(dec.ok);
    gm_edge_attributed_t d = dec.u.val;
    assert(!gm_oid_is_zero(&d.src_oid)); /* backfilled from SHA */
    assert(!gm_oid_is_zero(&d.tgt_oid));
    printf("OK\n");
}

int main(void) {
    printf("Running Attributed Edge CBOR Tests\n");
    printf("==================================\n");
    test_roundtrip_full();
    test_legacy_backfill();
    printf("\nAll Attributed Edge CBOR Tests Passed! ✅\n");
    return 0;
}
