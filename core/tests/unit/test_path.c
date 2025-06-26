/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/result.h"
#include "gitmind/types/path.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Test path creation */
static void test_path_new(void) {
    /* Basic path */
    gm_result_path result = gm_path_new("/usr/local/bin");
    assert(GM_IS_OK(result));

    gm_path_t path = GM_UNWRAP(result);
    assert(strcmp(gm_path_str(&path), "/usr/local/bin") == 0);
    assert(path.separator == '/');
    assert(path.is_absolute == true);
    assert(path.is_validated == true);

    gm_path_free(&path);

    /* Relative path */
    result = gm_path_new("src/main.c");
    assert(GM_IS_OK(result));

    path = GM_UNWRAP(result);
    assert(strcmp(gm_path_str(&path), "src/main.c") == 0);
    assert(path.is_absolute == false);

    gm_path_free(&path);

    /* Empty path */
    result = gm_path_new("");
    assert(GM_IS_OK(result));

    path = GM_UNWRAP(result);
    assert(gm_path_is_empty(&path));

    gm_path_free(&path);

    /* NULL path (treated as empty) */
    result = gm_path_new(NULL);
    assert(GM_IS_OK(result));

    path = GM_UNWRAP(result);
    assert(gm_path_is_empty(&path));

    gm_path_free(&path);

    printf("✓ test_path_new\n");
}

/* Test path joining */
static void test_path_join(void) {
    gm_result_path base_result = gm_path_new("/home/user");
    gm_result_path rel_result = gm_path_new("documents/file.txt");

    assert(GM_IS_OK(base_result) && GM_IS_OK(rel_result));

    gm_path_t base = GM_UNWRAP(base_result);
    gm_path_t rel = GM_UNWRAP(rel_result);

    /* Join paths */
    gm_result_path joined_result = gm_path_join(&base, &rel);
    assert(GM_IS_OK(joined_result));

    gm_path_t joined = GM_UNWRAP(joined_result);
    assert(strcmp(gm_path_str(&joined), "/home/user/documents/file.txt") == 0);

    gm_path_free(&base);
    gm_path_free(&rel);
    gm_path_free(&joined);

    /* Join with trailing separator */
    base_result = gm_path_new("/home/user/");
    rel_result = gm_path_new("documents");
    assert(GM_IS_OK(base_result) && GM_IS_OK(rel_result));

    base = GM_UNWRAP(base_result);
    rel = GM_UNWRAP(rel_result);

    joined_result = gm_path_join(&base, &rel);
    assert(GM_IS_OK(joined_result));

    joined = GM_UNWRAP(joined_result);
    assert(strcmp(gm_path_str(&joined), "/home/user/documents") == 0);

    gm_path_free(&base);
    gm_path_free(&rel);
    gm_path_free(&joined);

    /* Join with absolute relative path (returns relative) */
    base_result = gm_path_new("/home/user");
    rel_result = gm_path_new("/etc/config");
    assert(GM_IS_OK(base_result) && GM_IS_OK(rel_result));

    base = GM_UNWRAP(base_result);
    rel = GM_UNWRAP(rel_result);

    joined_result = gm_path_join(&base, &rel);
    assert(GM_IS_OK(joined_result));

    joined = GM_UNWRAP(joined_result);
    assert(strcmp(gm_path_str(&joined), "/etc/config") == 0);

    gm_path_free(&base);
    gm_path_free(&rel);
    gm_path_free(&joined);

    printf("✓ test_path_join\n");
}

/* Test dirname extraction */
static void test_path_dirname(void) {
    /* Normal path */
    gm_result_path result = gm_path_new("/home/user/file.txt");
    assert(GM_IS_OK(result));

    gm_path_t path = GM_UNWRAP(result);

    gm_result_path dir_result = gm_path_dirname(&path);
    assert(GM_IS_OK(dir_result));

    gm_path_t dir = GM_UNWRAP(dir_result);
    assert(strcmp(gm_path_str(&dir), "/home/user") == 0);

    gm_path_free(&path);
    gm_path_free(&dir);

    /* Path without directory */
    result = gm_path_new("file.txt");
    assert(GM_IS_OK(result));

    path = GM_UNWRAP(result);
    dir_result = gm_path_dirname(&path);
    assert(GM_IS_OK(dir_result));

    dir = GM_UNWRAP(dir_result);
    assert(strcmp(gm_path_str(&dir), ".") == 0);

    gm_path_free(&path);
    gm_path_free(&dir);

    /* Root path */
    result = gm_path_new("/");
    assert(GM_IS_OK(result));

    path = GM_UNWRAP(result);
    dir_result = gm_path_dirname(&path);
    assert(GM_IS_OK(dir_result));

    dir = GM_UNWRAP(dir_result);
    assert(strcmp(gm_path_str(&dir), "/") == 0);

    gm_path_free(&path);
    gm_path_free(&dir);

    printf("✓ test_path_dirname\n");
}

/* Test basename extraction */
static void test_path_basename(void) {
    /* Normal path */
    gm_result_path result = gm_path_new("/home/user/file.txt");
    assert(GM_IS_OK(result));

    gm_path_t path = GM_UNWRAP(result);

    gm_result_path base_result = gm_path_basename(&path);
    assert(GM_IS_OK(base_result));

    gm_path_t base = GM_UNWRAP(base_result);
    assert(strcmp(gm_path_str(&base), "file.txt") == 0);

    gm_path_free(&path);
    gm_path_free(&base);

    /* Path without directory */
    result = gm_path_new("file.txt");
    assert(GM_IS_OK(result));

    path = GM_UNWRAP(result);
    base_result = gm_path_basename(&path);
    assert(GM_IS_OK(base_result));

    base = GM_UNWRAP(base_result);
    assert(strcmp(gm_path_str(&base), "file.txt") == 0);

    gm_path_free(&path);
    gm_path_free(&base);

    /* Directory path */
    result = gm_path_new("/home/user/");
    assert(GM_IS_OK(result));

    path = GM_UNWRAP(result);
    base_result = gm_path_basename(&path);
    assert(GM_IS_OK(base_result));

    base = GM_UNWRAP(base_result);
    assert(strcmp(gm_path_str(&base), "") == 0);

    gm_path_free(&path);
    gm_path_free(&base);

    printf("✓ test_path_basename\n");
}

/* Test path safety checks */
static void test_path_safety(void) {
    /* Safe paths */
    gm_result_path result = gm_path_new("/home/user/docs");
    assert(GM_IS_OK(result));
    gm_path_t path = GM_UNWRAP(result);
    assert(gm_path_is_safe(&path));
    gm_path_free(&path);

    result = gm_path_new("src/main.c");
    assert(GM_IS_OK(result));
    path = GM_UNWRAP(result);
    assert(gm_path_is_safe(&path));
    gm_path_free(&path);

    /* Unsafe paths with .. */
    result = gm_path_new("../etc/passwd");
    assert(GM_IS_OK(result));
    path = GM_UNWRAP(result);
    assert(!gm_path_is_safe(&path));
    gm_path_free(&path);

    result = gm_path_new("/home/user/../../../etc");
    assert(GM_IS_OK(result));
    path = GM_UNWRAP(result);
    assert(!gm_path_is_safe(&path));
    gm_path_free(&path);

    printf("✓ test_path_safety\n");
}

/* Test path extensions */
static void test_path_extension(void) {
    gm_result_path result = gm_path_new("document.pdf");
    assert(GM_IS_OK(result));

    gm_path_t path = GM_UNWRAP(result);
    assert(gm_path_has_extension(&path, ".pdf"));
    assert(!gm_path_has_extension(&path, ".txt"));
    assert(!gm_path_has_extension(&path, ".pd"));

    gm_path_free(&path);

    /* Path without extension */
    result = gm_path_new("README");
    assert(GM_IS_OK(result));

    path = GM_UNWRAP(result);
    assert(!gm_path_has_extension(&path, ".txt"));

    gm_path_free(&path);

    /* Hidden file */
    result = gm_path_new(".gitignore");
    assert(GM_IS_OK(result));

    path = GM_UNWRAP(result);
    assert(!gm_path_has_extension(&path, ".gitignore"));

    gm_path_free(&path);

    printf("✓ test_path_extension\n");
}

/* Test path comparison */
static void test_path_compare(void) {
    gm_result_path r1 = gm_path_new("/home/user");
    gm_result_path r2 = gm_path_new("/home/user");
    gm_result_path r3 = gm_path_new("/home/other");

    assert(GM_IS_OK(r1) && GM_IS_OK(r2) && GM_IS_OK(r3));

    gm_path_t p1 = GM_UNWRAP(r1);
    gm_path_t p2 = GM_UNWRAP(r2);
    gm_path_t p3 = GM_UNWRAP(r3);

    assert(gm_path_equals(&p1, &p2));
    assert(!gm_path_equals(&p1, &p3));

    /* Prefix check */
    gm_result_path r4 = gm_path_new("/home/user/documents");
    assert(GM_IS_OK(r4));
    gm_path_t p4 = GM_UNWRAP(r4);

    assert(gm_path_starts_with(&p4, &p1));
    assert(!gm_path_starts_with(&p1, &p4));

    /* Child check */
    assert(gm_path_is_child_of(&p4, &p1));
    assert(!gm_path_is_child_of(&p1, &p4));
    assert(!gm_path_is_child_of(&p1, &p3));

    gm_path_free(&p1);
    gm_path_free(&p2);
    gm_path_free(&p3);
    gm_path_free(&p4);

    printf("✓ test_path_compare\n");
}

/* Test canonicalization */
static void test_path_canonicalize(void) {
    gm_result_path result = gm_path_new("/home/user/./documents");
    assert(GM_IS_OK(result));

    gm_path_t path = GM_UNWRAP(result);
    assert(path.state == GM_PATH_STATE_RAW);

    gm_result_path canon_result = gm_path_canonicalize(&path);
    assert(GM_IS_OK(canon_result));

    gm_path_t canon = GM_UNWRAP(canon_result);
    assert(canon.state == GM_PATH_STATE_CANONICAL);

    gm_path_free(&path);
    gm_path_free(&canon);

    printf("✓ test_path_canonicalize\n");
}

int main(void) {
    printf("Running path type tests...\n\n");

    test_path_new();
    test_path_join();
    test_path_dirname();
    test_path_basename();
    test_path_safety();
    test_path_extension();
    test_path_compare();
    test_path_canonicalize();

    printf("\n✅ All path tests passed!\n");
    return 0;
}