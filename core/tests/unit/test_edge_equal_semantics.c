/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "gitmind/edge.h"
#include "gitmind/types.h"

static void test_equal_with_oids(void) {
    printf("test_equal_with_oids... ");
    gm_edge_t a = {0}, b = {0};
    a.rel_type = b.rel_type = GM_REL_DEPENDS_ON;
    /* Different SHAs but identical OIDs -> equal */
    memset(a.src_sha, 0x11, GM_SHA1_SIZE);
    memset(b.src_sha, 0x22, GM_SHA1_SIZE);
    uint8_t raw[GM_OID_RAWSZ]; memset(raw, 0xAA, sizeof raw);
    git_oid_fromraw(&a.src_oid, raw);
    git_oid_fromraw(&b.src_oid, raw);
    memset(a.tgt_sha, 0x33, GM_SHA1_SIZE);
    memset(b.tgt_sha, 0x44, GM_SHA1_SIZE);
    git_oid_fromraw(&a.tgt_oid, raw);
    git_oid_fromraw(&b.tgt_oid, raw);
    assert(gm_edge_equal(&a, &b));
    printf("OK\n");
}

static void test_fallback_to_sha(void) {
    printf("test_fallback_to_sha... ");
    gm_edge_t a = {0}, b = {0};
    a.rel_type = b.rel_type = GM_REL_DEPENDS_ON;
    /* No OIDs: compare SHAs */
    memset(a.src_sha, 0x55, GM_SHA1_SIZE);
    memset(b.src_sha, 0x55, GM_SHA1_SIZE);
    memset(a.tgt_sha, 0x66, GM_SHA1_SIZE);
    memset(b.tgt_sha, 0x66, GM_SHA1_SIZE);
    assert(gm_edge_equal(&a, &b));
    b.tgt_sha[0] ^= 0xFF;
    assert(!gm_edge_equal(&a, &b));
    printf("OK\n");
}

int main(void) {
    printf("Running Edge Equality Semantics Tests\n");
    printf("====================================\n");
    test_equal_with_oids();
    test_fallback_to_sha();
    printf("\nAll Edge Equality Semantics Tests Passed! ✅\n");
    return 0;
}

