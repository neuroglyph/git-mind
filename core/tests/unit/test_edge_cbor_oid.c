/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "gitmind/edge.h"
#include "gitmind/types.h"
#include "gitmind/util/oid.h"
#include "gitmind/util/memory.h"
#include "gitmind/error.h"

/* Simple test for OID-first CBOR encode/decode */
int main(void) {
    printf("test_edge_cbor_oid... ");

    gm_edge_t e = {0};
    /* Set legacy SHAs to different values to ensure OID wins */
    memset(e.src_sha, 0x11, GM_SHA1_SIZE);
    memset(e.tgt_sha, 0x22, GM_SHA1_SIZE);
    e.rel_type = GM_REL_IMPLEMENTS;
    e.confidence = 0x3C00;
    e.timestamp = 42;
    strcpy(e.src_path, "src/A.c");
    strcpy(e.tgt_path, "src/B.c");
    strcpy(e.ulid, "01ARZ3NDEKTSV4RRFFQ69G5FAV");

    /* Set preferred OIDs */
    uint8_t src_raw[GM_OID_RAWSZ]; memset(src_raw, 0xAA, sizeof src_raw);
    uint8_t tgt_raw[GM_OID_RAWSZ]; memset(tgt_raw, 0xBB, sizeof tgt_raw);
    assert(gm_oid_from_raw(&e.src_oid, src_raw, sizeof src_raw) == GM_OK);
    assert(gm_oid_from_raw(&e.tgt_oid, tgt_raw, sizeof tgt_raw) == GM_OK);
    assert(gm_strcpy_safe(e.src_path, sizeof e.src_path, "src/A.c") == GM_OK);
    assert(gm_strcpy_safe(e.tgt_path, sizeof e.tgt_path, "src/B.c") == GM_OK);
    assert(gm_strcpy_safe(e.ulid, sizeof e.ulid,
                          "01ARZ3NDEKTSV4RRFFQ69G5FAV") == GM_OK);

    uint8_t buf[1024];
    size_t len = sizeof buf;
    gm_result_void_t enc = gm_edge_encode_cbor(&e, buf, &len);
    assert(enc.ok);
    assert(len > 0);

    gm_result_edge_t dec = gm_edge_decode_cbor(buf, len);
    assert(dec.ok);
    gm_edge_t d = dec.u.val;

    /* OIDs must carry through */
    assert(gm_oid_equal(&d.src_oid, &e.src_oid));
    assert(gm_oid_equal(&d.tgt_oid, &e.tgt_oid));

    /* Equality must use OID-first */
    assert(gm_edge_equal(&e, &d));

    printf("OK\n");
    return 0;
}
