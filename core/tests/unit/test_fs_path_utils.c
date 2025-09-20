/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "gitmind/fs/path_utils.h"
#include "gitmind/result.h"
#include "gitmind/types.h"

static void test_logical_normalization(void) {
    char out[GM_PATH_MAX];

    gm_result_void_t res =
        gm_fs_path_normalize_logical("/a//b///c", out, sizeof(out));
    assert(res.ok);
    assert(strcmp(out, "/a/b/c") == 0);

    res = gm_fs_path_normalize_logical("/a/./b", out, sizeof(out));
    assert(res.ok);
    assert(strcmp(out, "/a/b") == 0);

    res = gm_fs_path_normalize_logical("/a/b/../c", out, sizeof(out));
    assert(res.ok);
    assert(strcmp(out, "/a/c") == 0);

    res = gm_fs_path_normalize_logical("/", out, sizeof(out));
    assert(res.ok);
    assert(strcmp(out, "/") == 0);

    res = gm_fs_path_normalize_logical("", out, sizeof(out));
    assert(res.ok);
    assert(strcmp(out, ".") == 0);

    res = gm_fs_path_normalize_logical("relative/./path", out, sizeof(out));
    assert(res.ok);
    assert(strcmp(out, "relative/path") == 0);
}

static void test_dirname_basename(void) {
    char out[GM_PATH_MAX];

    gm_result_void_t res = gm_fs_path_dirname("/a/b/c", out, sizeof(out));
    assert(res.ok);
    assert(strcmp(out, "/a/b") == 0);

    res = gm_fs_path_dirname("/", out, sizeof(out));
    assert(res.ok);
    assert(strcmp(out, "/") == 0);

    res = gm_fs_path_dirname("single", out, sizeof(out));
    assert(res.ok);
    assert(strcmp(out, ".") == 0);

    char base[GM_PATH_MAX];
    assert(gm_fs_path_dirname("/base", base, sizeof(base)).ok);
    size_t len = strlen(base);
    res = gm_fs_path_basename_append(base, sizeof(base), &len, "leaf");
    assert(res.ok);
    assert(strcmp(base, "/leaf") == 0);

    len = strlen(base);
    res = gm_fs_path_basename_append(base, sizeof(base), &len, "segment");
    assert(res.ok);
    assert(strcmp(base, "/leaf/segment") == 0);
}

int main(void) {
    printf("test_fs_path_utils... ");
    test_logical_normalization();
    test_dirname_basename();
    printf("OK\n");
    return 0;
}
