/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sodium.h>

#include "gitmind/security/string.h"
#include "gitmind/edge.h"
#include "gitmind/types.h"

static void test_base64_roundtrip(void) {
    printf("test_base64_roundtrip... ");
    /* Sample CBOR-like bytes */
    uint8_t data[] = { 0xA1, 0x01, 0x61, 'x' }; /* {1: "x"} */
    const int variant = sodium_base64_VARIANT_ORIGINAL;
    size_t b64_len = sodium_base64_ENCODED_LEN(sizeof(data), variant);
    char *b64 = (char *)malloc(b64_len);
    assert(b64 != NULL);

    sodium_bin2base64(b64, b64_len, data, sizeof(data), variant);

    uint8_t decoded[16] = {0};
    size_t out_len = 0;
    int rc = sodium_base642bin(decoded, sizeof(decoded), b64, strlen(b64),
                               NULL, &out_len, NULL, variant);
    free(b64);
    assert(rc == 0);
    assert(out_len == sizeof(data));
    assert(memcmp(decoded, data, sizeof(data)) == 0);
    printf("OK\n");
}

static void test_gm_snprintf_truncation(void) {
    printf("test_gm_snprintf_truncation... ");
    char buf[5];
    int n = gm_snprintf(buf, sizeof buf, "%s", "abcdef");
    /* Would have written 6; buffer holds 4 + null */
    assert(n == 6);
    assert(strncmp(buf, "abcd", 4) == 0);
    assert(buf[4] == '\0');
    printf("OK\n");
}

static void test_edge_equal_oid_preferred(void) {
    printf("test_edge_equal_oid_preferred... ");
    gm_edge_t a = {0}, b = {0};
    a.rel_type = b.rel_type = GM_REL_IMPLEMENTS;
    /* Different legacy SHAs */
    memset(a.src_sha, 0x11, GM_SHA1_SIZE);
    memset(b.src_sha, 0x22, GM_SHA1_SIZE);
    memset(a.tgt_sha, 0x33, GM_SHA1_SIZE);
    memset(b.tgt_sha, 0x44, GM_SHA1_SIZE);
    /* Same OIDs */
    uint8_t same_raw[GM_OID_RAWSZ];
    memset(same_raw, 0xAA, sizeof same_raw);
    git_oid_fromraw(&a.src_oid, same_raw);
    git_oid_fromraw(&b.src_oid, same_raw);
    git_oid_fromraw(&a.tgt_oid, same_raw);
    git_oid_fromraw(&b.tgt_oid, same_raw);
    assert(gm_edge_equal(&a, &b));

    /* OIDs differ but legacy SHAs match => OID-first: not equal */
    gm_edge_t c = a, d = a;
    uint8_t other_raw[GM_OID_RAWSZ];
    memset(other_raw, 0xBB, sizeof other_raw);
    git_oid_fromraw(&c.src_oid, other_raw);
    git_oid_fromraw(&d.tgt_oid, other_raw);
    assert(!gm_edge_equal(&c, &d));

    /* Both differ (OIDs and legacy) => not equal */
    gm_edge_t e1 = a, e2 = b;
    git_oid_fromraw(&e2.src_oid, other_raw);
    git_oid_fromraw(&e2.tgt_oid, other_raw);
    assert(!gm_edge_equal(&e1, &e2));
    printf("OK\n");
}

int main(void) {
    if (sodium_init() < 0) {
        fprintf(stderr, "libsodium init failed\n");
        return 1;
    }
    printf("Running Journal/Safety Tests\n");
    printf("============================\n");
    test_base64_roundtrip();
    test_gm_snprintf_truncation();
    test_edge_equal_oid_preferred();
    printf("\nAll Journal/Safety Tests Passed! ✅\n");
    return 0;
}
