/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "gitmind/cache/internal/oid_prefix.h"
#include "gitmind/error.h"
#include "gitmind/util/oid.h"

int main(void) {
    printf("test_cache_oid_prefix... ");
    gm_oid_t oid;
    assert(gm_oid_from_hex(&oid, "0123456789abcdef0123456789abcdef01234567") == GM_OK);

    char out[GM_CACHE_MAX_SHARD_PATH];
    assert(gm_cache_oid_prefix(&oid, 4, out, sizeof(out)) == GM_OK);
    assert(strcmp(out, "0") == 0);

    assert(gm_cache_oid_prefix(&oid, 8, out, sizeof(out)) == GM_OK);
    assert(strcmp(out, "01") == 0);

    assert(gm_cache_oid_prefix(&oid, 12, out, sizeof(out)) == GM_OK);
    assert(strcmp(out, "012") == 0);

    assert(gm_cache_oid_prefix(&oid, 20, out, sizeof(out)) == GM_OK);
    assert(strcmp(out, "01234") == 0);

    /* Rounding for non-multiples of 4 bits */
    assert(gm_cache_oid_prefix(&oid, 5, out, sizeof(out)) == GM_OK);
    assert(strcmp(out, "01") == 0);
    assert(gm_cache_oid_prefix(&oid, 7, out, sizeof(out)) == GM_OK);
    assert(strcmp(out, "01") == 0);
    assert(gm_cache_oid_prefix(&oid, 9, out, sizeof(out)) == GM_OK);
    assert(strcmp(out, "012") == 0);

    /* Zero bits yields empty string */
    assert(gm_cache_oid_prefix(&oid, 0, out, sizeof(out)) == GM_OK);
    assert(strcmp(out, "") == 0);

    /* Error handling paths */
    assert(gm_cache_oid_prefix(NULL, 4, out, sizeof(out)) == GM_OK);
    assert(strcmp(out, "") == 0);
    assert(gm_cache_oid_prefix(&oid, 4, NULL, sizeof(out)) != GM_OK);
    assert(gm_cache_oid_prefix(&oid, 4, out, 0) != GM_OK);

    char small[2];
    assert(gm_cache_oid_prefix(&oid, 16, small, sizeof(small)) == GM_OK);
    assert(strlen(small) == sizeof(small) - 1);

    /* Bits beyond limit clamp at GM_CACHE_MAX_SHARD_PATH-1 */
    assert(gm_cache_oid_prefix(&oid, 1024, out, sizeof(out)) == GM_OK);
    assert(strlen(out) <= GM_CACHE_MAX_SHARD_PATH - 1);

    printf("OK\n");
    return 0;
}
