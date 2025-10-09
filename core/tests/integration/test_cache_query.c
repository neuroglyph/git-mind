/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <git2.h>

#include "gitmind/cache.h"
#include <git2/sys/commit.h>
#include "gitmind/types/ulid.h"
#include "gitmind/error.h"
#include "gitmind/journal.h"
#include "gitmind/result.h"
#include "gitmind/edge.h"
#include "gitmind/types.h"
#include "gitmind/util/oid.h"
#include "gitmind/security/string.h"
#include "gitmind/util/memory.h"

#include "gitmind/adapters/fs/posix_temp_adapter.h"
#include "gitmind/adapters/git/libgit2_repository_port.h"
#include "core/tests/support/temp_repo_helpers.h"

static void ensure_branch_with_commit(git_repository *repo, const char *branch) {
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

    char refname[128];
    int wn = gm_snprintf(refname, sizeof refname, "refs/heads/%s", branch);
    assert(wn >= 0 && (size_t)wn < sizeof refname);

    /* Create a commit object and then a direct ref to it */
    rc = git_commit_create_from_ids(&commit_oid, repo, NULL, sig, sig, NULL,
                                    "init", &tree_oid, 0, NULL);
    git_signature_free(sig);
    assert(rc == 0);
    git_reference *ref = NULL;
    rc = git_reference_create(&ref, repo, refname, &commit_oid, 1, "init");
    if (ref) git_reference_free(ref);
    assert(rc == 0);

    rc = git_repository_set_head(repo, refname);
    assert(rc == 0);
}
static void set_user_config(git_repository *repo) {
    git_config *cfg = NULL;
    int rc = git_repository_config(&cfg, repo);
    assert(rc == 0);
    rc = git_config_set_string(cfg, "user.name", "Tester");
    assert(rc == 0);
    rc = git_config_set_string(cfg, "user.email", "tester@example.com");
    assert(rc == 0);
    git_config_free(cfg);
}

int main(void) {
    printf("test_cache_query... ");
    git_libgit2_init();

    gm_context_t ctx = {0};

    gm_result_void_t fs_result =
        gm_posix_fs_temp_port_create(&ctx.fs_temp_port, NULL,
                                     &ctx.fs_temp_port_dispose);
    assert(fs_result.ok);

    char repo_path[GM_PATH_MAX];
    gm_result_void_t temp_rc =
        gm_test_make_temp_repo_dir(&ctx.fs_temp_port, "cache-query-repo",
                                   repo_path, sizeof(repo_path));
    assert(temp_rc.ok);

    git_repository *repo = NULL;
    int rc = git_repository_init(&repo, repo_path, false);
    assert(rc == 0 && repo);
    set_user_config(repo);
    ensure_branch_with_commit(repo, "testq");

    gm_result_void_t repo_port_result =
        gm_libgit2_repository_port_create(&ctx.git_repo_port, NULL,
                                          &ctx.git_repo_port_dispose, repo);
    assert(repo_port_result.ok);
    assert(ctx.git_repo_port.vtbl != NULL);

    /* Create two edges A->B and A->C */
    gm_edge_t edges[2];
    memset(edges, 0, sizeof edges);
    uint8_t A[GM_OID_RAWSZ], B[GM_OID_RAWSZ], C[GM_OID_RAWSZ];
    memset(A, 0x11, sizeof A); memset(B, 0x22, sizeof B); memset(C, 0x33, sizeof C);
    assert(gm_oid_from_raw(&edges[0].src_oid, A, sizeof A) == GM_OK);
    assert(gm_oid_from_raw(&edges[0].tgt_oid, B, sizeof B) == GM_OK);
    edges[0].rel_type = GM_REL_IMPLEMENTS;
    edges[0].confidence = 0x3C00;
    assert(gm_strcpy_safe(edges[0].src_path, sizeof edges[0].src_path, "A") == GM_OK);
    assert(gm_strcpy_safe(edges[0].tgt_path, sizeof edges[0].tgt_path, "B") == GM_OK);
    gm_result_ulid_t ulr0 = gm_ulid_generate(edges[0].ulid);
    assert(ulr0.ok);
    assert(gm_oid_from_raw(&edges[1].src_oid, A, sizeof A) == GM_OK);
    assert(gm_oid_from_raw(&edges[1].tgt_oid, C, sizeof C) == GM_OK);
    edges[1].rel_type = GM_REL_IMPLEMENTS;
    edges[1].confidence = 0x3C00;
    assert(gm_strcpy_safe(edges[1].src_path, sizeof edges[1].src_path, "A") == GM_OK);
    assert(gm_strcpy_safe(edges[1].tgt_path, sizeof edges[1].tgt_path, "C") == GM_OK);
    gm_result_ulid_t ulr1 = gm_ulid_generate(edges[1].ulid);
    assert(ulr1.ok);

    rc = gm_journal_append(&ctx, edges, 2);
    assert(rc == GM_OK);

    /* Rebuild cache for branch 'master' (or default). Use explicit branch 'testq' */
    rc = gm_cache_rebuild(&ctx, "testq", true);
    assert(rc == GM_OK);

    gm_cache_result_t r1 = {0}, r2 = {0};
    gm_oid_t a_oid; assert(gm_oid_from_raw(&a_oid, A, sizeof A) == GM_OK);
    rc = gm_cache_query_fanout(&ctx, "testq", &a_oid, &r1);
    assert(rc == GM_OK);
    assert(r1.count >= 2);
    gm_cache_result_free(&r1);

    gm_oid_t b_oid; assert(gm_oid_from_raw(&b_oid, B, sizeof B) == GM_OK);
    rc = gm_cache_query_fanin(&ctx, "testq", &b_oid, &r2);
    assert(rc == GM_OK);
    assert(r2.count >= 1);
    gm_cache_result_free(&r2);

    git_repository *saved_repo = repo;
    if (ctx.git_repo_port_dispose != NULL) {
        ctx.git_repo_port_dispose(&ctx.git_repo_port);
    }
    assert(repo == saved_repo);
    git_repository_free(repo);
    repo = NULL;
    gm_result_void_t rm_rc =
        gm_test_cleanup_temp_repo_dir(&ctx.fs_temp_port, repo_path);
    if (!rm_rc.ok) {
        if (rm_rc.u.err != NULL) {
            gm_error_free(rm_rc.u.err);
        }
    }
    if (ctx.fs_temp_port_dispose != NULL) {
        ctx.fs_temp_port_dispose(&ctx.fs_temp_port);
    }
    git_libgit2_shutdown();
    printf("OK\n");
    return 0;
}
