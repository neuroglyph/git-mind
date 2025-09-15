/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "gitmind/types.h"

/* Simple LCG for deterministic pseudo-random bytes */
static uint32_t lcg_state = 0x12345678u;
static uint32_t lcg_next(void) {
    lcg_state = 1664525u * lcg_state + 1013904223u;
    return lcg_state;
}

static void random_oid(gm_oid_t *oid) {
    uint8_t raw[GM_OID_RAWSZ];
    for (int i = 0; i < GM_OID_RAWSZ; i++) raw[i] = (uint8_t)(lcg_next() & 0xFF);
    git_oid_fromraw(oid, raw);
}

int main(void) {
    printf("test_cache_shard_distribution... ");
    enum { N = 10000 };
    uint32_t buckets[256];
    memset(buckets, 0, sizeof buckets);

    for (int i = 0; i < N; i++) {
        gm_oid_t oid = {0};
        random_oid(&oid);
        const uint8_t *raw = git_oid_raw(&oid);
        buckets[raw[0]]++;
    }

    /* Check very weak uniformity: each bucket should be within [0, N/8] for this sample. */
    /* This is a sanity smoke-test, not a statistical proof. */
    uint32_t max_bucket = 0;
    for (int i = 0; i < 256; i++) {
        if (buckets[i] > max_bucket) max_bucket = buckets[i];
    }
    assert(max_bucket < (uint32_t)(N / 8));
    printf("OK\n");
    return 0;
}
