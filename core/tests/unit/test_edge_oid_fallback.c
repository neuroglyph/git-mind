/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "gitmind/edge.h"
#include "gitmind/util/oid.h"
#include "gitmind/util/memory.h"
#include "gitmind/error.h"

int main(void) {
    printf("test_edge_oid_fallback... ");
    gm_edge_t a = {0};
    gm_edge_t b = {0};

    /* Set legacy SHA bytes identical; OIDs zero to force fallback */
    memset(a.src_sha, 0x11, GM_SHA1_SIZE);
    memset(a.tgt_sha, 0x22, GM_SHA1_SIZE);
    memset(b.src_sha, 0x11, GM_SHA1_SIZE);
    memset(b.tgt_sha, 0x22, GM_SHA1_SIZE);
    a.rel_type = b.rel_type = GM_REL_REFERENCES;
    assert(gm_strcpy_safe(a.src_path, sizeof a.src_path, "A") == GM_OK);
    assert(gm_strcpy_safe(a.tgt_path, sizeof a.tgt_path, "B") == GM_OK);
    assert(gm_strcpy_safe(b.src_path, sizeof b.src_path, "A") == GM_OK);
    assert(gm_strcpy_safe(b.tgt_path, sizeof b.tgt_path, "B") == GM_OK);

    /* With zero OIDs and identical legacy bytes, edges should be equal */
    assert(gm_edge_equal(&a, &b));

    /* Now set OIDs non-zero and different; equality must use OIDs */
    uint8_t raw1[GM_OID_RAWSZ]; memset(raw1, 0xAA, sizeof raw1);
    uint8_t raw2[GM_OID_RAWSZ]; memset(raw2, 0xBB, sizeof raw2);
    assert(gm_oid_from_raw(&a.src_oid, raw1, sizeof raw1) == GM_OK);
    assert(gm_oid_from_raw(&b.src_oid, raw2, sizeof raw2) == GM_OK);
    assert(gm_oid_from_raw(&a.tgt_oid, raw1, sizeof raw1) == GM_OK);
    assert(gm_oid_from_raw(&b.tgt_oid, raw2, sizeof raw2) == GM_OK);
    assert(!gm_edge_equal(&a, &b));

    printf("OK\n");
    return 0;
}
