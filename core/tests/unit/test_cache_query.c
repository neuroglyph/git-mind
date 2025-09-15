/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <git2.h>

#include "gitmind/cache.h"
#include "gitmind/journal.h"
#include "gitmind/edge.h"
#include "gitmind/types.h"

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

    git_repository *repo = NULL;
    int rc = git_repository_init(&repo, "./.gm_cache_query_tmp", true);
    assert(rc == 0 && repo);
    set_user_config(repo);

    gm_context_t ctx = {0};
    ctx.git_repo = repo;

    /* Create two edges A->B and A->C */
    gm_edge_t edges[2]; memset(edges, 0, sizeof edges);
    uint8_t A[GM_OID_RAWSZ], B[GM_OID_RAWSZ], C[GM_OID_RAWSZ];
    memset(A, 0x11, sizeof A); memset(B, 0x22, sizeof B); memset(C, 0x33, sizeof C);
    git_oid_fromraw(&edges[0].src_oid, A); git_oid_fromraw(&edges[0].tgt_oid, B);
    edges[0].rel_type = GM_REL_IMPLEMENTS; edges[0].confidence = 0x3C00; strcpy(edges[0].src_path, "A"); strcpy(edges[0].tgt_path, "B");
    (void)gm_ulid_generate(edges[0].ulid);
    git_oid_fromraw(&edges[1].src_oid, A); git_oid_fromraw(&edges[1].tgt_oid, C);
    edges[1].rel_type = GM_REL_IMPLEMENTS; edges[1].confidence = 0x3C00; strcpy(edges[1].src_path, "A"); strcpy(edges[1].tgt_path, "C");
    (void)gm_ulid_generate(edges[1].ulid);

    rc = gm_journal_append(&ctx, edges, 2);
    assert(rc == GM_OK);

    /* Rebuild cache for branch 'master' (or default). Use explicit branch 'testq' */
    rc = gm_cache_rebuild(&ctx, "testq", true);
    assert(rc == GM_OK);

    gm_cache_result_t r1 = {0}, r2 = {0};
    gm_oid_t a_oid; git_oid_fromraw(&a_oid, A);
    rc = gm_cache_query_fanout(&ctx, "testq", &a_oid, &r1);
    assert(rc == GM_OK);
    assert(r1.count >= 2);
    gm_cache_result_free(&r1);

    gm_oid_t b_oid; git_oid_fromraw(&b_oid, B);
    rc = gm_cache_query_fanin(&ctx, "testq", &b_oid, &r2);
    assert(rc == GM_OK);
    assert(r2.count >= 1);
    gm_cache_result_free(&r2);

    git_repository_free(repo);
    git_libgit2_shutdown();
    printf("OK\n");
    return 0;
}

