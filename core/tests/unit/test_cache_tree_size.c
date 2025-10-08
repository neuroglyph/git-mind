/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <git2.h>

#include "gitmind/error.h"
#include "gitmind/security/string.h"
#include "gitmind/types.h"
#include "gitmind/adapters/fs/posix_temp_adapter.h"
#include "gitmind/adapters/git/libgit2_repository_port.h"
#include "support/temp_repo_helpers.h"

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

    gm_fs_temp_port_t fs_port = {0};
    void (*fs_dispose)(gm_fs_temp_port_t *) = NULL;
    gm_result_void_t fs_result =
        gm_posix_fs_temp_port_create(&fs_port, NULL, &fs_dispose);
    assert(fs_result.ok);

    char repo_path[GM_PATH_MAX];
    gm_result_void_t tmp_dir_result =
        gm_test_make_temp_repo_dir(&fs_port, "cache-tree-repo", repo_path,
                                   sizeof(repo_path));
    assert(tmp_dir_result.ok);

    git_repository *repo = NULL;
    int rc = git_repository_init(&repo, repo_path, true);
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

    git_tree *root_tree = NULL;
    rc = git_tree_lookup(&root_tree, repo, &root_tree_oid);
    assert(rc == 0 && root_tree != NULL);

    git_signature *sig = NULL;
    rc = git_signature_now(&sig, "tester", "tester@example.com");
    assert(rc == 0);

    git_oid commit_oid;
    rc = git_commit_create(&commit_oid, repo, NULL, sig, sig, NULL,
                           "tree-size", root_tree, 0, NULL);
    git_signature_free(sig);
    git_tree_free(root_tree);
    assert(rc == 0);

    gm_git_repository_port_t port = {0};
    gm_libgit2_repository_port_state_t *state = NULL;
    void (*dispose)(gm_git_repository_port_t *) = NULL;
    gm_result_void_t port_result =
        gm_libgit2_repository_port_create(&port, &state, &dispose, repo);
    assert(port_result.ok);

    uint64_t total = 0;
    gm_result_void_t size_result = gm_git_repository_port_commit_tree_size(
        &port, &commit_oid, &total);
    assert(size_result.ok);

    size_t expected = 0;
    expected += object_size(repo, &root_tree_oid);
    expected += object_size(repo, &nested_tree_oid);
    expected += object_size(repo, &root_blob_oid);
    expected += object_size(repo, &nested_blob_oid);

    assert(total == (uint64_t)expected);

    if (dispose != NULL) {
        dispose(&port);
    }
    git_repository_free(repo);
    gm_result_void_t rm_result =
        gm_fs_temp_port_remove_tree(&fs_port, repo_path);
    assert(rm_result.ok);
    if (fs_dispose != NULL) {
        fs_dispose(&fs_port);
    }
    git_libgit2_shutdown();
    printf("OK\n");
    return 0;
}
