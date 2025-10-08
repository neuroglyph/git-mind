/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <git2.h>
#include <git2/sys/commit.h>

#include "gitmind/cache.h"
#include "gitmind/context.h"
#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/adapters/git/libgit2_repository_port.h"
#include "gitmind/adapters/fs/posix_temp_adapter.h"
#include "core/tests/support/temp_repo_helpers.h"

static void make_temp_ref(git_repository *repo, const char *legacy_refname) {
    git_treebuilder *tb = NULL;
    git_oid tree_oid, commit_oid;
    git_signature *sig = NULL;
    git_reference *legacy = NULL;
    int rc;

    rc = git_treebuilder_new(&tb, repo, NULL);
    assert(rc == 0);
    rc = git_treebuilder_write(&tree_oid, tb);
    git_treebuilder_free(tb);
    assert(rc == 0);

    rc = git_signature_now(&sig, "tester", "tester@example.com");
    assert(rc == 0);

    /* Create a commit object directly (no ref update) */
    rc = git_commit_create_from_ids(&commit_oid, repo, NULL, sig, sig, NULL,
                                    "cache", &tree_oid, 0, NULL);
    assert(rc == 0);

    /* Create the legacy ref pointing to that commit */
    rc = git_reference_create(&legacy, repo, legacy_refname, &commit_oid, 1,
                              "legacy cache ref");
    assert(rc == 0);
    git_reference_free(legacy);
    git_signature_free(sig);
}

int main(void) {
    printf("test_cache_meta_fallback... ");
    git_libgit2_init();

    /* Create temp repo */
    gm_fs_temp_port_t fs_port = {0};
    gm_posix_fs_state_t *fs_state = NULL;
    void (*fs_dispose)(gm_fs_temp_port_t *) = NULL;
    gm_result_void_t fs_rc =
        gm_posix_fs_temp_port_create(&fs_port, &fs_state, &fs_dispose);
    assert(fs_rc.ok);

    char repo_path[GM_PATH_MAX];
    gm_result_void_t temp_rc =
        gm_test_make_temp_repo_dir(&fs_port, "cache-meta-repo", repo_path,
                                   sizeof(repo_path));
    assert(temp_rc.ok);

    git_repository *repo = NULL;
    int rc = git_repository_init(&repo, repo_path, true);
    assert(rc == 0 && repo != NULL);

    /* Create legacy timestamped ref under refs/gitmind/cache/test/12345 */
    make_temp_ref(repo, "refs/gitmind/cache/test/12345");

    gm_context_t ctx = {0};

    gm_result_void_t repo_port_result =
        gm_libgit2_repository_port_create(&ctx.git_repo_port, NULL,
                                          &ctx.git_repo_port_dispose, repo);
    assert(repo_port_result.ok);
    gm_cache_meta_t meta;
    rc = gm_cache_load_meta(&ctx, "test", &meta);
    assert(rc == GM_OK);
    assert(strcmp(meta.branch, "test") == 0);

    if (ctx.git_repo_port_dispose != NULL) {
        ctx.git_repo_port_dispose(&ctx.git_repo_port);
    }
    git_repository_free(repo);
    gm_result_void_t rm_rc = gm_fs_temp_port_remove_tree(&fs_port, repo_path);
    assert(rm_rc.ok);
    if (fs_dispose != NULL) {
        fs_dispose(&fs_port);
    }
    git_libgit2_shutdown();
    printf("OK\n");
    return 0;
}
