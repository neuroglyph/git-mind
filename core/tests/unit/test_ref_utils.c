/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "gitmind/util/ref.h"
#include "gitmind/util/memory.h"
#include "gitmind/constants_internal.h"

static void test_build_ref_valid(void) {
    char buf[128];
    int rc = gm_build_ref(buf, sizeof buf, GITMIND_EDGES_REF_PREFIX, "main");
    assert(rc == 0);
    assert(strcmp(buf, "refs/gitmind/edges/main") == 0);

    rc = gm_build_ref(buf, sizeof buf, GITMIND_EDGES_REF_PREFIX, "feature/x");
    assert(rc == 0);
    assert(strcmp(buf, "refs/gitmind/edges/feature/x") == 0);
}

static void test_build_ref_reject_leading_refs(void) {
    char buf[64];
    int rc = gm_build_ref(buf, sizeof buf, GITMIND_EDGES_REF_PREFIX, "refs/heads/x");
    assert(rc != 0);
}

static void test_strcpy_safe_truncation(void) {
    char buf[5];
    int rc = gm_strcpy_safe(buf, sizeof buf, "abcdef");
    assert(rc == -1); /* truncated */
    assert(buf[4] == '\0');
    assert(strncmp(buf, "abcd", 4) == 0);
}

int main(void) {
    printf("test_ref_utils... ");
    test_build_ref_valid();
    test_build_ref_reject_leading_refs();
    test_strcpy_safe_truncation();
    printf("OK\n");
    return 0;
}
