/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

#include <git2.h>

#include "gitmind/error.h"
#include "gitmind/security/string.h"
#include "gitmind/types.h"
#include "gitmind/adapters/fs/posix_temp_adapter.h"
#include "gitmind/adapters/git/libgit2_repository_port.h"
#include "support/temp_repo_helpers.h"

static void write_file(const char *path, const char *content) {
    FILE *f = fopen(path, "wb");
    assert(f != NULL);
    size_t n = fwrite(content, 1, strlen(content), f);
    (void)n;
    fclose(f);
}

int main(void) {
    printf("test_repo_build_tree... ");
    git_libgit2_init();

    gm_fs_temp_port_t fs_port = {0};
    void (*fs_dispose)(gm_fs_temp_port_t *) = NULL;
    gm_result_void_t fs_result =
        gm_posix_fs_temp_port_create(&fs_port, NULL, &fs_dispose);
    assert(fs_result.ok);

    char repo_path[GM_PATH_MAX];
    gm_result_void_t repo_dir_result =
        gm_test_make_temp_repo_dir(&fs_port, "build-tree-repo", repo_path,
                                   sizeof(repo_path));
    assert(repo_dir_result.ok);

    char src_path[GM_PATH_MAX];
    gm_result_void_t src_dir_result =
        gm_test_make_temp_repo_dir(&fs_port, "build-tree-src", src_path,
                                   sizeof(src_path));
    assert(src_dir_result.ok);

    /* Create a bare repo */
    git_repository *repo = NULL;
    int rc = git_repository_init(&repo, repo_path, true);
    assert(rc == 0 && repo);

    /* Build a small source directory */
    rc = mkdir(src_path, 0775);
    assert(rc == 0 || errno == EEXIST);

    char root_path[GM_PATH_MAX];
    assert(gm_snprintf(root_path, sizeof(root_path), "%s/root.txt",
                       src_path) >= 0);
    write_file(root_path, "root\n");

    char dir_path[GM_PATH_MAX];
    assert(gm_snprintf(dir_path, sizeof(dir_path), "%s/dir", src_path) >= 0);
    rc = mkdir(dir_path, 0775);
    assert(rc == 0 || errno == EEXIST);

    char child_path[GM_PATH_MAX];
    assert(gm_snprintf(child_path, sizeof(child_path), "%s/dir/child.txt",
                       src_path) >= 0);
    write_file(child_path, "child\n");

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
        gm_git_repository_port_build_tree_from_directory(&port, src_path,
                                                         &tree_oid);
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

    if (dispose != NULL) {
        dispose(&port);
    }
    git_repository_free(repo);
    gm_result_void_t rm_repo_result =
        gm_fs_temp_port_remove_tree(&fs_port, repo_path);
    assert(rm_repo_result.ok);
    gm_result_void_t rm_src_result =
        gm_fs_temp_port_remove_tree(&fs_port, src_path);
    assert(rm_src_result.ok);
    if (fs_dispose != NULL) {
        fs_dispose(&fs_port);
    }
    git_libgit2_shutdown();
    printf("OK\n");
    return 0;
}
