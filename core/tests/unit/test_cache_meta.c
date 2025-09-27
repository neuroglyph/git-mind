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
    git_repository *repo = NULL;
    int rc = git_repository_init(&repo, "./.gm_cache_meta_tmp", true);
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
    git_libgit2_shutdown();
    printf("OK\n");
    return 0;
}
