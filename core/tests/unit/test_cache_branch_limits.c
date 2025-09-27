/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <git2.h>
#include <git2/sys/commit.h>

#include "gitmind/cache.h"
#include "gitmind/context.h"
#include "gitmind/edge.h"
#include "gitmind/error.h"
#include "gitmind/journal.h"
#include "gitmind/result.h"
#include "gitmind/types.h"
#include "gitmind/types/ulid.h"

#include "gitmind/adapters/fs/posix_temp_adapter.h"
#include "gitmind/adapters/git/libgit2_repository_port.h"

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
    git_treebuilder *tb = NULL;
    git_oid tree_oid, commit_oid;
    git_signature *sig = NULL;
    git_reference *ref = NULL;
    char refname[256];
    int rc;

    rc = git_treebuilder_new(&tb, repo, NULL);
    assert(rc == 0);
    rc = git_treebuilder_write(&tree_oid, tb);
    git_treebuilder_free(tb);
    assert(rc == 0);

    rc = git_signature_now(&sig, "tester", "tester@example.com");
    assert(rc == 0);

    snprintf(refname, sizeof refname, "refs/heads/%s", branch);
    rc = git_commit_create_from_ids(&commit_oid, repo, NULL, sig, sig, NULL,
                                    "init", &tree_oid, 0, NULL);
    git_signature_free(sig);
    assert(rc == 0);

    rc = git_reference_create(&ref, repo, refname, &commit_oid, 1, "init");
    if (ref != NULL) {
        git_reference_free(ref);
    }
    assert(rc == 0);

    rc = git_repository_set_head(repo, refname);
    assert(rc == 0);
}

static void append_dummy_edge(gm_context_t *ctx) {
    gm_edge_t edge;
    memset(&edge, 0, sizeof edge);

    uint8_t src_raw[GM_OID_RAWSZ];
    uint8_t tgt_raw[GM_OID_RAWSZ];
    memset(src_raw, 0x11, sizeof src_raw);
    memset(tgt_raw, 0x22, sizeof tgt_raw);

    git_oid_fromraw(&edge.src_oid, src_raw);
    git_oid_fromraw(&edge.tgt_oid, tgt_raw);

    edge.rel_type = GM_REL_IMPLEMENTS;
    edge.confidence = 0x3C00;
    strcpy(edge.src_path, "A");
    strcpy(edge.tgt_path, "B");
    (void)gm_ulid_generate(edge.ulid);

    int rc = gm_journal_append(ctx, &edge, 1);
    assert(rc == GM_OK);
}

int main(void) {
    printf("test_cache_branch_limits... ");
    git_libgit2_init();

    git_repository *repo = NULL;
    int rc = git_repository_init(&repo, "./.gm_cache_branch_tmp", true);
    assert(rc == 0 && repo);
    set_user_config(repo);

    char valid_branch[GM_CACHE_BRANCH_NAME_SIZE];
    memset(valid_branch, 'a', sizeof valid_branch);
    valid_branch[GM_CACHE_BRANCH_NAME_SIZE - 1] = '\0';
    ensure_branch_with_commit(repo, valid_branch);

    gm_context_t ctx = {0};

    gm_result_void_t repo_port_result =
        gm_libgit2_repository_port_create(&ctx.git_repo_port, NULL,
                                          &ctx.git_repo_port_dispose, repo);
    assert(repo_port_result.ok);

    gm_result_void_t fs_result =
        gm_posix_fs_temp_port_create(&ctx.fs_temp_port, NULL,
                                     &ctx.fs_temp_port_dispose);
    assert(fs_result.ok);

    append_dummy_edge(&ctx);
    rc = gm_cache_rebuild(&ctx, valid_branch, true);
    assert(rc == GM_OK);

    char invalid_branch[GM_CACHE_BRANCH_NAME_SIZE + 1];
    memset(invalid_branch, 'b', sizeof invalid_branch);
    invalid_branch[GM_CACHE_BRANCH_NAME_SIZE] = '\0';
    ensure_branch_with_commit(repo, invalid_branch);
    append_dummy_edge(&ctx);

    rc = gm_cache_rebuild(&ctx, invalid_branch, true);
    assert(rc == GM_ERR_INVALID_ARGUMENT);

    if (ctx.fs_temp_port_dispose != NULL) {
        ctx.fs_temp_port_dispose(&ctx.fs_temp_port);
    }
    if (ctx.git_repo_port_dispose != NULL) {
        ctx.git_repo_port_dispose(&ctx.git_repo_port);
    }
    git_libgit2_shutdown();
    printf("OK\n");
    return 0;
}
