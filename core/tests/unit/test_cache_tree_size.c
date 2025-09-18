/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <git2.h>

#include "gitmind/error.h"

int gm_cache_calculate_size(git_repository *repo, const git_oid *tree_oid,
                            uint64_t *size_bytes);

static size_t object_size(git_repository *repo, const git_oid *oid) {
    git_odb *odb = NULL;
    git_odb_object *obj = NULL;
    int rc = git_repository_odb(&odb, repo);
    assert(rc == 0);
    rc = git_odb_read(&obj, odb, oid);
    git_odb_free(odb);
    assert(rc == 0 && obj != NULL);
    size_t size = git_odb_object_size(obj);
    git_odb_object_free(obj);
    return size;
}

int main(void) {
    printf("test_cache_tree_size... ");
    git_libgit2_init();

    git_repository *repo = NULL;
    int rc = git_repository_init(&repo, "./.gm_cache_tree_tmp", true);
    assert(rc == 0 && repo);

    const char *root_content = "root";
    const char *nested_content = "nested-data";

    git_oid root_blob_oid;
    rc = git_blob_create_frombuffer(&root_blob_oid, repo,
                                    root_content, strlen(root_content));
    assert(rc == 0);

    git_oid nested_blob_oid;
    rc = git_blob_create_frombuffer(&nested_blob_oid, repo,
                                    nested_content, strlen(nested_content));
    assert(rc == 0);

    /* Build nested tree */
    git_treebuilder *tb = NULL;
    git_oid nested_tree_oid;
    rc = git_treebuilder_new(&tb, repo, NULL);
    assert(rc == 0);
    rc = git_treebuilder_insert(NULL, tb, "nested.txt", &nested_blob_oid,
                                GIT_FILEMODE_BLOB);
    assert(rc == 0);
    rc = git_treebuilder_write(&nested_tree_oid, tb);
    git_treebuilder_free(tb);
    assert(rc == 0);

    /* Build root tree */
    rc = git_treebuilder_new(&tb, repo, NULL);
    assert(rc == 0);
    rc = git_treebuilder_insert(NULL, tb, "root.txt", &root_blob_oid,
                                GIT_FILEMODE_BLOB);
    assert(rc == 0);
    rc = git_treebuilder_insert(NULL, tb, "nested", &nested_tree_oid,
                                GIT_FILEMODE_TREE);
    assert(rc == 0);
    git_oid root_tree_oid;
    rc = git_treebuilder_write(&root_tree_oid, tb);
    git_treebuilder_free(tb);
    assert(rc == 0);

    uint64_t total = 0;
    rc = gm_cache_calculate_size(repo, &root_tree_oid, &total);
    assert(rc == GM_OK);

    size_t expected = 0;
    expected += object_size(repo, &root_tree_oid);
    expected += object_size(repo, &nested_tree_oid);
    expected += object_size(repo, &root_blob_oid);
    expected += object_size(repo, &nested_blob_oid);

    assert(total == (uint64_t)expected);

    git_repository_free(repo);
    git_libgit2_shutdown();
    printf("OK\n");
    return 0;
}
