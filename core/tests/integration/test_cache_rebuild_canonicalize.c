/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <git2.h>
#include <git2/sys/commit.h>
#include <stdio.h>
#include <string.h>

#include "gitmind/cache.h"
#include "gitmind/edge.h"
#include "gitmind/error.h"
#include "gitmind/journal.h"
#include "gitmind/result.h"
#include "gitmind/security/string.h"
#include "gitmind/types.h"
#include "gitmind/types/ulid.h"
#include "gitmind/util/memory.h"
#include "gitmind/util/oid.h"

#include "gitmind/adapters/fs/posix_temp_adapter.h"
#include "gitmind/adapters/git/libgit2_repository_port.h"

#include "core/tests/support/temp_repo_helpers.h"

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

static void ensure_branch_with_commit(git_repository *repo, const char *branch) {
    git_treebuilder *builder = NULL;
    git_oid tree_oid;
    git_signature *sig = NULL;
    git_reference *ref = NULL;

    assert(git_treebuilder_new(&builder, repo, NULL) == 0);
    assert(git_treebuilder_write(&tree_oid, builder) == 0);
    git_treebuilder_free(builder);

    assert(git_signature_now(&sig, "tester", "tester@example.com") == 0);

    git_oid commit_oid;
    assert(git_commit_create_from_ids(&commit_oid, repo, NULL, sig, sig, NULL,
                                      "init", &tree_oid, 0, NULL) == 0);
    git_signature_free(sig);

    char refname[128];
    int ref_rc = gm_snprintf(refname, sizeof(refname), "refs/heads/%s", branch);
    assert(ref_rc >= 0 && (size_t)ref_rc < sizeof(refname));

    assert(git_reference_create(&ref, repo, refname, &commit_oid, 1,
                                "init") == 0);
    git_reference_free(ref);
    assert(git_repository_set_head(repo, refname) == 0);
}

static void seed_edge(gm_edge_t *edge, const uint8_t src_fill,
                      const uint8_t tgt_fill, const char *src_path,
                      const char *tgt_path) {
    memset(edge, 0, sizeof(*edge));
    uint8_t src[GM_OID_RAWSZ];
    uint8_t tgt[GM_OID_RAWSZ];
    memset(src, src_fill, sizeof(src));
    memset(tgt, tgt_fill, sizeof(tgt));
    assert(gm_oid_from_raw(&edge->src_oid, src, sizeof(src)) == GM_OK);
    assert(gm_oid_from_raw(&edge->tgt_oid, tgt, sizeof(tgt)) == GM_OK);
    edge->rel_type = GM_REL_REFERENCES;
    edge->confidence = 0x3C00;
    assert(gm_strcpy_safe(edge->src_path, sizeof(edge->src_path), src_path) ==
           GM_OK);
    assert(gm_strcpy_safe(edge->tgt_path, sizeof(edge->tgt_path), tgt_path) ==
           GM_OK);
    gm_result_ulid_t ulid_rc = gm_ulid_generate(edge->ulid);
    assert(ulid_rc.ok);
}

int main(void) {
    printf("test_cache_rebuild_canonicalize... ");
    git_libgit2_init();

    gm_context_t ctx = {0};

    gm_result_void_t fs_result =
        gm_posix_fs_temp_port_create(&ctx.fs_temp_port, NULL,
                                     &ctx.fs_temp_port_dispose);
    assert(fs_result.ok);

    char repo_path[GM_PATH_MAX];
    gm_result_void_t repo_dir_rc = gm_test_make_temp_repo_dir(
        &ctx.fs_temp_port, "cache-rebuild-canon", repo_path, sizeof(repo_path));
    assert(repo_dir_rc.ok);

    git_repository *repo = NULL;
    assert(git_repository_init(&repo, repo_path, false) == 0);
    set_user_config(repo);
    ensure_branch_with_commit(repo, "canon");

    gm_result_void_t repo_port_rc = gm_libgit2_repository_port_create(
        &ctx.git_repo_port, NULL, &ctx.git_repo_port_dispose, repo);
    assert(repo_port_rc.ok);

    gm_edge_t edges[2];
    seed_edge(&edges[0], 0x01, 0xA1, "src_a", "tgt_a");
    seed_edge(&edges[1], 0x02, 0xB2, "src_b", "tgt_b");
    assert(gm_journal_append(&ctx, edges, 2) == GM_OK);

    /* Run cache rebuild twice; prior regression would double-free canonicalized
     * path buffers and crash. */
    assert(gm_cache_rebuild(&ctx, "canon", true) == GM_OK);
    assert(gm_cache_rebuild(&ctx, "canon", false) == GM_OK);

    if (ctx.git_repo_port_dispose != NULL) {
        ctx.git_repo_port_dispose(&ctx.git_repo_port);
    }
    git_repository_free(repo);
    repo = NULL;

    gm_result_void_t cleanup_rc =
        gm_test_cleanup_temp_repo_dir(&ctx.fs_temp_port, repo_path);
    if (!cleanup_rc.ok && cleanup_rc.u.err != NULL) {
        gm_error_free(cleanup_rc.u.err);
    }

    if (ctx.fs_temp_port_dispose != NULL) {
        ctx.fs_temp_port_dispose(&ctx.fs_temp_port);
    }
    git_libgit2_shutdown();
    printf("OK\n");
    return 0;
}
