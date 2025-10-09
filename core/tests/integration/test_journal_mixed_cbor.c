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
#include "gitmind/edge_attributed.h"
#include "gitmind/types.h"
#include "gitmind/context.h"
#include "gitmind/result.h"
#include "gitmind/util/memory.h"
#include "gitmind/util/oid.h"
#include "gitmind/adapters/git/libgit2_repository_port.h"
#include "gitmind/adapters/fs/posix_temp_adapter.h"
#include "core/tests/support/temp_repo_helpers.h"

typedef struct {
    size_t count;
    gm_edge_t last;
} basic_ctx_t;

typedef struct {
    size_t count;
    gm_edge_attributed_t last;
} attr_ctx_t;

/**
 * Capture a basic edge into a basic_ctx_t by copying the provided edge and
 * incrementing the context's count.
 *
 * @param edge Pointer to the basic edge to capture.
 * @param ud   Pointer to a basic_ctx_t where the edge will be stored (last)
 *             and the count incremented.
 * @returns `GM_OK` on success, `GM_ERR_INVALID_ARGUMENT` if `edge` or `ud` is NULL,
 *          or another `GM_*` error code propagated from the copy operation.
 */
static int capture_basic_cb(const gm_edge_t *edge, void *ud) {
    if (edge == NULL || ud == NULL) {
        return GM_ERR_INVALID_ARGUMENT;
    }
    basic_ctx_t *ctx = (basic_ctx_t *)ud;
    int copy_rc = gm_memcpy_span(&ctx->last, sizeof(ctx->last), edge,
                                 sizeof(*edge));
    if (copy_rc != GM_OK) {
        return copy_rc;
    }
    ctx->count++;
    return GM_OK;
}

/**
 * Capture an attributed edge by copying it into the provided context and incrementing the count.
 *
 * @param edge Pointer to the attributed edge to capture; must not be NULL.
 * @param ud Pointer to an attr_ctx_t that will receive the captured edge and have its count incremented; must not be NULL.
 * @returns `GM_OK` on success, `GM_ERR_INVALID_ARGUMENT` if `edge` or `ud` is NULL, or an error code returned from the underlying copy operation. 
 */
static int capture_attr_cb(const gm_edge_attributed_t *edge, void *ud) {
    if (edge == NULL || ud == NULL) {
        return GM_ERR_INVALID_ARGUMENT;
    }
    attr_ctx_t *ctx = (attr_ctx_t *)ud;
    int copy_rc = gm_memcpy_span(&ctx->last, sizeof(ctx->last), edge,
                                 sizeof(*edge));
    if (copy_rc != GM_OK) {
        return copy_rc;
    }
    ctx->count++;
    return GM_OK;
}

/**
 * Run the mixed-CBOR journal integration test: create a temporary Git repository,
 * write a commit containing one basic and one attributed CBOR-encoded edge payload,
 * read them back, and verify their source/target OIDs match the generated values.
 *
 * @returns `0` on success, non-zero on failure.
 */
int main(void) {
    printf("test_journal_mixed_cbor... ");
    git_libgit2_init();

    gm_fs_temp_port_t fs_port = {0};
    gm_posix_fs_state_t *fs_state = NULL;
    void (*fs_dispose)(gm_fs_temp_port_t *) = NULL;
    gm_result_void_t fs_rc =
        gm_posix_fs_temp_port_create(&fs_port, &fs_state, &fs_dispose);
    assert(fs_rc.ok);

    /* Create temp repo */
    char repo_path[GM_PATH_MAX];
    gm_result_void_t repo_dir_rc =
        gm_test_make_temp_repo_dir(&fs_port, "journal-mixed-repo",
                                   repo_path, sizeof(repo_path));
    assert(repo_dir_rc.ok);

    git_repository *repo = NULL;
    int rc = git_repository_init(&repo, repo_path, true);
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
    uint8_t rawA[GM_OID_RAWSZ]; memset(rawA, 0xCC, sizeof rawA);
    uint8_t rawB[GM_OID_RAWSZ]; memset(rawB, 0xDD, sizeof rawB);

    gm_edge_t e = {0};
    assert(gm_memcpy_span(e.src_sha, sizeof e.src_sha, rawA, sizeof rawA) == GM_OK);
    assert(gm_memcpy_span(e.tgt_sha, sizeof e.tgt_sha, rawB, sizeof rawB) == GM_OK);
    assert(gm_strcpy_safe(e.src_path, sizeof e.src_path, "src/A.c") == GM_OK);
    assert(gm_strcpy_safe(e.tgt_path, sizeof e.tgt_path, "src/B.c") == GM_OK);
    e.rel_type = GM_REL_IMPLEMENTS; e.confidence = 0x3C00; e.timestamp = 7;
    gm_result_ulid_t u = gm_ulid_generate(e.ulid); assert(u.ok);
    uint8_t buf1[512]; size_t len1 = sizeof buf1; assert(gm_edge_encode_cbor(&e, buf1, &len1).ok);

    gm_edge_attributed_t ae = {0};
    assert(gm_memcpy_span(ae.src_sha, sizeof ae.src_sha, rawA, sizeof rawA) == GM_OK);
    assert(gm_memcpy_span(ae.tgt_sha, sizeof ae.tgt_sha, rawB, sizeof rawB) == GM_OK);
    assert(gm_strcpy_safe(ae.src_path, sizeof ae.src_path,
                          "docs/A.md") == GM_OK);
    assert(gm_strcpy_safe(ae.tgt_path, sizeof ae.tgt_path,
                          "src/C.c") == GM_OK);
    ae.rel_type = GM_REL_REFERENCES; ae.confidence = 0x1C00; ae.timestamp = 8;
    ae.attribution.source_type = GM_SOURCE_AI_CLAUDE;
    assert(gm_strcpy_safe(ae.attribution.author, sizeof ae.attribution.author,
                          "claude@local") == GM_OK);
    assert(gm_strcpy_safe(ae.attribution.session_id,
                          sizeof ae.attribution.session_id, "s1") == GM_OK);
    ae.lane = GM_LANE_ANALYSIS;
    u = gm_ulid_generate(ae.ulid); assert(u.ok);
    uint8_t buf2[512]; size_t len2 = sizeof buf2; assert(gm_edge_attributed_encode_cbor(&ae, buf2, &len2).ok);

    /* Concatenate */
    uint8_t payload[1024];
    assert(len1 + len2 <= sizeof payload);
    assert(gm_memcpy_span(payload, sizeof payload, buf1, len1) == GM_OK);
    assert(gm_memcpy_span(payload + len1, sizeof payload - len1, buf2, len2) == GM_OK);

    gm_context_t ctx = {0};
    gm_result_void_t repo_port_result =
        gm_libgit2_repository_port_create(&ctx.git_repo_port, NULL,
                                          &ctx.git_repo_port_dispose, repo);
    assert(repo_port_result.ok);
    rc = gm_journal_create_commit(&ctx, "refs/gitmind/edges/test", payload, len1 + len2);
    assert(rc == GM_OK);

    basic_ctx_t cbasic = {0};
    attr_ctx_t cattr = {0};
    gm_oid_t expected_src; assert(gm_oid_from_raw(&expected_src, rawA, sizeof rawA) == GM_OK);
    gm_oid_t expected_tgt; assert(gm_oid_from_raw(&expected_tgt, rawB, sizeof rawB) == GM_OK);

    rc = gm_journal_read(&ctx, "test", capture_basic_cb, &cbasic);
    assert(rc == GM_OK && cbasic.count >= 1);
    assert(!gm_oid_is_zero(&cbasic.last.src_oid));
    assert(!gm_oid_is_zero(&cbasic.last.tgt_oid));
    assert(gm_oid_equal(&cbasic.last.src_oid, &expected_src));
    assert(gm_oid_equal(&cbasic.last.tgt_oid, &expected_tgt));

    rc = gm_journal_read_attributed(&ctx, "test", capture_attr_cb, &cattr);
    assert(rc == GM_OK && cattr.count >= 1);
    assert(!gm_oid_is_zero(&cattr.last.src_oid));
    assert(!gm_oid_is_zero(&cattr.last.tgt_oid));
    assert(gm_oid_equal(&cattr.last.src_oid, &expected_src));
    assert(gm_oid_equal(&cattr.last.tgt_oid, &expected_tgt));

    if (ctx.git_repo_port_dispose != NULL) {
        ctx.git_repo_port_dispose(&ctx.git_repo_port);
    }
    git_repository_free(repo);
    git_libgit2_shutdown();
    gm_result_void_t rm_rc = gm_fs_temp_port_remove_tree(&fs_port, repo_path);
    assert(rm_rc.ok);
    if (fs_dispose != NULL) {
        fs_dispose(&fs_port);
    }
    printf("OK\n");
    return 0;
}
