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

    printf("OK\n");
    return 0;
}
