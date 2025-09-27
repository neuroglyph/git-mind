/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <git2.h>

#include "gitmind/journal.h"
#include "gitmind/error.h"
#include "gitmind/edge.h"
#include "gitmind/types/ulid.h"
#include "gitmind/error.h"
#include "gitmind/edge_attributed.h"
#include "gitmind/types.h"
#include "gitmind/context.h"
#include "gitmind/result.h"
#include "gitmind/adapters/git/libgit2_repository_port.h"

typedef struct {
    size_t count;
} count_ctx_t;

static int count_basic_cb(const gm_edge_t *edge, void *ud) {
    (void)edge; count_ctx_t *c = (count_ctx_t *)ud; c->count++; return 0;
}
static int count_attr_cb(const gm_edge_attributed_t *edge, void *ud) {
    (void)edge; count_ctx_t *c = (count_ctx_t *)ud; c->count++; return 0;
}

int main(void) {
    printf("test_journal_mixed_cbor... ");
    git_libgit2_init();

    /* Create temp repo */
    git_repository *repo = NULL;
    int rc = git_repository_init(&repo, "./.gm_journal_tmp", true);
    assert(rc == 0 && repo);

    /* Ensure empty tree object exists */
    git_treebuilder *tb = NULL; git_oid empty_tree_oid;
    rc = git_treebuilder_new(&tb, repo, NULL); assert(rc == 0);
    rc = git_treebuilder_write(&empty_tree_oid, tb); git_treebuilder_free(tb);
    assert(rc == 0);

    /* Set user config for signature default */
    git_config *cfg = NULL; rc = git_repository_config(&cfg, repo); assert(rc == 0);
    rc = git_config_set_string(cfg, "user.name", "Tester"); assert(rc == 0);
    rc = git_config_set_string(cfg, "user.email", "tester@example.com"); assert(rc == 0);
    git_config_free(cfg);

    /* Build mixed CBOR payload: one basic, one attributed */
    gm_edge_t e = {0};
    strcpy(e.src_path, "src/A.c"); strcpy(e.tgt_path, "src/B.c");
    e.rel_type = GM_REL_IMPLEMENTS; e.confidence = 0x3C00; e.timestamp = 7;
    uint8_t rawA[GM_OID_RAWSZ]; memset(rawA, 0xCC, sizeof rawA);
    uint8_t rawB[GM_OID_RAWSZ]; memset(rawB, 0xDD, sizeof rawB);
    git_oid_fromraw(&e.src_oid, rawA); git_oid_fromraw(&e.tgt_oid, rawB);
    gm_result_ulid_t u = gm_ulid_generate(e.ulid); assert(u.ok);
    uint8_t buf1[512]; size_t len1 = sizeof buf1; assert(gm_edge_encode_cbor(&e, buf1, &len1).ok);

    gm_edge_attributed_t ae = {0};
    strcpy(ae.src_path, "docs/A.md"); strcpy(ae.tgt_path, "src/C.c");
    ae.rel_type = GM_REL_REFERENCES; ae.confidence = 0x1C00; ae.timestamp = 8;
    git_oid_fromraw(&ae.src_oid, rawA); git_oid_fromraw(&ae.tgt_oid, rawB);
    ae.attribution.source_type = GM_SOURCE_AI_CLAUDE;
    strcpy(ae.attribution.author, "claude@local");
    strcpy(ae.attribution.session_id, "s1");
    ae.lane = GM_LANE_ANALYSIS;
    u = gm_ulid_generate(ae.ulid); assert(u.ok);
    uint8_t buf2[512]; size_t len2 = sizeof buf2; assert(gm_edge_attributed_encode_cbor(&ae, buf2, &len2).ok);

    /* Concatenate */
    uint8_t payload[1024]; memcpy(payload, buf1, len1); memcpy(payload + len1, buf2, len2);

    gm_context_t ctx = {0};
    gm_result_void_t repo_port_result =
        gm_libgit2_repository_port_create(&ctx.git_repo_port, NULL,
                                          &ctx.git_repo_port_dispose, repo);
    assert(repo_port_result.ok);
    rc = gm_journal_create_commit(&ctx, "refs/gitmind/edges/test", payload, len1 + len2);
    assert(rc == GM_OK);

    count_ctx_t cbasic = {0}, cattr = {0};
    rc = gm_journal_read(&ctx, "test", count_basic_cb, &cbasic);
    assert(rc == GM_OK && cbasic.count >= 1);
    rc = gm_journal_read_attributed(&ctx, "test", count_attr_cb, &cattr);
    assert(rc == GM_OK && cattr.count >= 1);

    if (ctx.git_repo_port_dispose != NULL) {
        ctx.git_repo_port_dispose(&ctx.git_repo_port);
    } else {
        git_repository_free(repo);
    }
    git_libgit2_shutdown();
    printf("OK\n");
    return 0;
}
