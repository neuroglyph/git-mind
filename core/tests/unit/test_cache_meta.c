/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <git2.h>

#include "gitmind/cache.h"
#include "gitmind/context.h"
#include "gitmind/error.h"

static void make_temp_ref(git_repository *repo, const char *refname) {
    git_treebuilder *tb = NULL;
    git_oid tree_oid, commit_oid;
    git_signature *sig = NULL;
    int rc;

    rc = git_treebuilder_new(&tb, repo, NULL);
    assert(rc == 0);
    rc = git_treebuilder_write(&tree_oid, tb);
    git_treebuilder_free(tb);
    assert(rc == 0);

    rc = git_signature_now(&sig, "tester", "tester@example.com");
    assert(rc == 0);
    rc = git_commit_create_v(&commit_oid, repo, refname, sig, sig, NULL,
                             "cache", (git_tree*)NULL, 0);
    /* Older libgit2 requires tree; re-create with tree lookup */
    if (rc < 0) {
        git_tree *tree = NULL;
        rc = git_tree_lookup(&tree, repo, &tree_oid);
        assert(rc == 0);
        rc = git_commit_create(&commit_oid, repo, refname, sig, sig, NULL,
                               "cache", tree, 0, NULL);
        git_tree_free(tree);
        assert(rc == 0);
    }
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
    ctx.git_repo = repo;
    gm_cache_meta_t meta;
    rc = gm_cache_load_meta(&ctx, "test", &meta);
    assert(rc == GM_OK);
    assert(strcmp(meta.branch, "test") == 0);

    git_repository_free(repo);
    git_libgit2_shutdown();
    printf("OK\n");
    return 0;
}
