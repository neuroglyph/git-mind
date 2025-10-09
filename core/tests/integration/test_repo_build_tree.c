/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#include <git2.h>

#include "gitmind/error.h"
#include "gitmind/adapters/git/libgit2_repository_port.h"
#include "gitmind/security/string.h"
#include "gitmind/adapters/fs/posix_temp_adapter.h"
#include "core/tests/support/temp_repo_helpers.h"

static void write_file(const char *path, const char *content) {
    FILE *f = fopen(path, "wb");
    assert(f != NULL);
    size_t len = strlen(content);
    size_t written = fwrite(content, 1, len, f);
    assert(written == len);
    fclose(f);
}

int main(void) {
    printf("test_repo_build_tree... ");
    git_libgit2_init();

    gm_fs_temp_port_t fs_port = {0};
    gm_posix_fs_state_t *fs_state = NULL;
    void (*fs_dispose)(gm_fs_temp_port_t *) = NULL;
    gm_result_void_t fs_rc =
        gm_posix_fs_temp_port_create(&fs_port, &fs_state, &fs_dispose);
    assert(fs_rc.ok);

    char repo_path[GM_PATH_MAX];
    gm_result_void_t repo_dir_rc = gm_test_make_temp_repo_dir(&fs_port,
                                                              "build-tree-repo",
                                                              repo_path,
                                                              sizeof(repo_path));
    assert(repo_dir_rc.ok);

    char src_dir[GM_PATH_MAX];
    gm_result_void_t src_dir_rc = gm_test_make_temp_repo_dir(&fs_port,
                                                             "build-tree-src",
                                                             src_dir,
                                                             sizeof(src_dir));
    assert(src_dir_rc.ok);

    /* Create a bare repo */
    git_repository *repo = NULL;
    int rc = git_repository_init(&repo, repo_path, true);
    assert(rc == 0 && repo);

    /* Build a small source directory */
    rc = mkdir(src_dir, 0775);
    assert(rc == 0 || errno == EEXIST);
    char root_file[GM_PATH_MAX];
    int root_fmt = gm_snprintf(root_file, sizeof(root_file), "%s/root.txt", src_dir);
    assert(root_fmt >= 0 && (size_t)root_fmt < sizeof(root_file));
    write_file(root_file, "root\n");
    char nested_dir[GM_PATH_MAX];
    int dir_fmt = gm_snprintf(nested_dir, sizeof(nested_dir), "%s/dir", src_dir);
    assert(dir_fmt >= 0 && (size_t)dir_fmt < sizeof(nested_dir));
    rc = mkdir(nested_dir, 0775);
    assert(rc == 0 || errno == EEXIST);
    char child_file[GM_PATH_MAX];
    int child_fmt = gm_snprintf(child_file, sizeof(child_file), "%s/dir/child.txt", src_dir);
    assert(child_fmt >= 0 && (size_t)child_fmt < sizeof(child_file));
    write_file(child_file, "child\n");

    /* Wire the repository port */
    gm_git_repository_port_t port = {0};
    gm_libgit2_repository_port_state_t *state = NULL;
    void (*dispose)(gm_git_repository_port_t *) = NULL;
    gm_result_void_t port_result =
        gm_libgit2_repository_port_create(&port, &state, &dispose, repo);
    assert(port_result.ok);

    /* Invoke tree build */
    gm_oid_t tree_oid;
    gm_result_void_t tree_result =
        gm_git_repository_port_build_tree_from_directory(&port, src_dir, &tree_oid);
    assert(tree_result.ok);

    /* Verify object exists and is a tree */
    git_odb *odb = NULL;
    git_odb_object *obj = NULL;
    rc = git_repository_odb(&odb, repo);
    assert(rc == 0);
    rc = git_odb_read(&obj, odb, (const git_oid *)&tree_oid);
    git_odb_free(odb);
    assert(rc == 0 && obj != NULL);
    assert(git_odb_object_type(obj) == GIT_OBJECT_TREE);
    git_odb_object_free(obj);

    git_tree *tree = NULL;
    rc = git_tree_lookup(&tree, repo, (const git_oid *)&tree_oid);
    assert(rc == 0 && tree != NULL);
    const git_tree_entry *root_entry = git_tree_entry_byname(tree, "root.txt");
    assert(root_entry != NULL);
    assert(git_tree_entry_type(root_entry) == GIT_OBJECT_BLOB);
    const git_tree_entry *dir_entry = git_tree_entry_byname(tree, "dir");
    assert(dir_entry != NULL);
    assert(git_tree_entry_type(dir_entry) == GIT_OBJECT_TREE);
    git_tree_free(tree);

    if (dispose != NULL) {
        dispose(&port);
    }
    git_repository_free(repo);
    gm_result_void_t rm_repo = gm_fs_temp_port_remove_tree(&fs_port, repo_path);
    assert(rm_repo.ok);
    gm_result_void_t rm_src = gm_fs_temp_port_remove_tree(&fs_port, src_dir);
    assert(rm_src.ok);
    if (fs_dispose != NULL) {
        fs_dispose(&fs_port);
    }
    git_libgit2_shutdown();
    printf("OK\n");
    return 0;
}
