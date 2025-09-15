/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "gitmind/edge.h"

int main(void) {
    printf("test_edge_oid_fallback... ");
    gm_edge_t a = {0};
    gm_edge_t b = {0};

    /* Set legacy SHA bytes different; OIDs zero to force fallback */
    memset(a.src_sha, 0x11, GM_SHA1_SIZE);
    memset(a.tgt_sha, 0x22, GM_SHA1_SIZE);
    memset(b.src_sha, 0x11, GM_SHA1_SIZE);
    memset(b.tgt_sha, 0x22, GM_SHA1_SIZE);
    a.rel_type = b.rel_type = GM_REL_REFERENCES;
    strcpy(a.src_path, "A"); strcpy(a.tgt_path, "B");
    strcpy(b.src_path, "A"); strcpy(b.tgt_path, "B");

    /* With zero OIDs and identical legacy bytes, edges should be equal */
    assert(gm_edge_equal(&a, &b));

    /* Now set OIDs non-zero and different; equality must use OIDs */
    uint8_t raw1[GM_OID_RAWSZ]; memset(raw1, 0xAA, sizeof raw1);
    uint8_t raw2[GM_OID_RAWSZ]; memset(raw2, 0xBB, sizeof raw2);
    git_oid_fromraw(&a.src_oid, raw1); git_oid_fromraw(&b.src_oid, raw2);
    git_oid_fromraw(&a.tgt_oid, raw1); git_oid_fromraw(&b.tgt_oid, raw2);
    assert(!gm_edge_equal(&a, &b));

    printf("OK\n");
    return 0;
}

