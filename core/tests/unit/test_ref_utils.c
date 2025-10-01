/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "gitmind/util/ref.h"
#include "gitmind/util/memory.h"
#include "gitmind/constants_internal.h"
#include "gitmind/error.h"

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
    assert(rc == GM_ERR_INVALID_ARGUMENT);
    assert(buf[0] == '\0');
}

static void test_strcpy_safe_truncation(void) {
    char buf[5];
    int rc = gm_strcpy_safe(buf, sizeof buf, "abcdef");
    assert(rc == -1); /* truncated */
    assert(buf[4] == '\0');
    assert(strncmp(buf, "abcd", 4) == 0);
}

static void test_build_ref_invalid_inputs(void) {
    struct invalid_case {
        const char *branch;
    } cases[] = {
        {""},              /* empty */
        {"/leading"},      /* leading slash */
        {"trailing/"},     /* trailing slash */
        {"feat~ure"},      /* forbidden character */
        {"feat^ure"},
        {"feat:ure"},
        {"feat?ure"},
        {"feat[ure"},
        {"feat*ure"},
        {"feat\\ure"},
        {"double..dot"},
        {"brace@{test"},
    };

    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        char buf[64] = {0};
        int rc = gm_build_ref(buf, sizeof buf, GITMIND_EDGES_REF_PREFIX,
                              cases[i].branch);
        assert(rc == GM_ERR_INVALID_ARGUMENT);
        assert(buf[0] == '\0');
    }
}

static void test_build_ref_null_and_buffer(void) {
    char buf[64];
    int rc = gm_build_ref(NULL, sizeof buf, GITMIND_EDGES_REF_PREFIX, "main");
    assert(rc == GM_ERR_INVALID_ARGUMENT);

    rc = gm_build_ref(buf, 0, GITMIND_EDGES_REF_PREFIX, "main");
    assert(rc == GM_ERR_INVALID_ARGUMENT);
}

static void test_build_ref_buffer_too_small(void) {
    char buf[8];
    int rc = gm_build_ref(buf, sizeof buf, GITMIND_EDGES_REF_PREFIX, "main");
    assert(rc == GM_ERR_BUFFER_TOO_SMALL);
    assert(buf[0] == '\0');
}

int main(void) {
    printf("test_ref_utils... ");
    test_build_ref_valid();
    test_build_ref_reject_leading_refs();
    test_strcpy_safe_truncation();
    test_build_ref_invalid_inputs();
    test_build_ref_null_and_buffer();
    test_build_ref_buffer_too_small();
    printf("OK\n");
    return 0;
}
