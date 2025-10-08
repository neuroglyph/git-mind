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

    /* Create a bare repo */
    git_repository *repo = NULL;
    int rc = git_repository_init(&repo, "./.gm_build_tree_repo", true);
    assert(rc == 0 && repo);

    /* Build a small source directory */
    const char *src = "./.gm_build_tree_src";
    rc = mkdir(src, 0775);
    assert(rc == 0 || errno == EEXIST);
    write_file("./.gm_build_tree_src/root.txt", "root\n");
    rc = mkdir("./.gm_build_tree_src/dir", 0775);
    assert(rc == 0 || errno == EEXIST);
    write_file("./.gm_build_tree_src/dir/child.txt", "child\n");

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
        gm_git_repository_port_build_tree_from_directory(&port, src, &tree_oid);
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
    git_libgit2_shutdown();
    printf("OK\n");
    return 0;
}
