/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "gitmind/context.h"
#include "gitmind/edge.h"
#include "gitmind/journal.h"
#include "gitmind/util/oid.h"
#include "gitmind/util/memory.h"
#include "gitmind/types/ulid.h"

#include "core/tests/fakes/diagnostics/fake_diagnostics_port.h"

/* Minimal stub repo that returns NFF once, then OK on reference_update. */
typedef struct {
    gm_git_repository_port_t port;
    int update_calls;
} nff_repo_t;

static gm_result_void_t nff_update(void *self, const gm_git_reference_update_spec_t *spec) {
    (void)spec;
    nff_repo_t *st = (nff_repo_t *)self;
    st->update_calls++;
    if (st->update_calls == 1) {
        return gm_err_void(GM_ERROR(GM_ERR_ALREADY_EXISTS, "nff"));
    }
    return gm_ok_void();
}

static gm_result_void_t nff_walk(void *self, const char *ref_name,
                                 gm_git_commit_visit_cb cb, void *ud) {
    (void)self; (void)ref_name; (void)cb; (void)ud; return gm_ok_void();
}

static gm_result_void_t nff_commit_create(void *self, const gm_git_commit_spec_t *spec,
                                          gm_oid_t *out) {
    (void)self; (void)spec;
    memset(out, 0, sizeof *out);
    out->id[0] = 1;
    return gm_ok_void();
}

static gm_result_void_t nff_head_branch(void *self, char *out, size_t n) {
    (void)self; const char *name = "main";
    size_t i=0; while (name[i] && i+1<n) { out[i]=name[i]; ++i; } out[i]='\0';
    return gm_ok_void();
}

static const gm_git_repository_port_vtbl_t NFF_VTBL = {
    .head_branch = nff_head_branch,
    .walk_commits = nff_walk,
    .commit_create = nff_commit_create,
    .reference_update = nff_update,
};

int main(void) {
    printf("test_journal_nff_retry... ");
    gm_context_t ctx = {0};

    /* Wire stub repo */
    nff_repo_t rep = {0};
    rep.port.vtbl = &NFF_VTBL;
    rep.port.self = &rep;
    ctx.git_repo_port = rep.port;

    /* Wire diagnostics fake */
    gm_fake_diag_state_t *dst = NULL;
    assert(gm_fake_diag_port_init(&ctx.diag_port, &dst).ok);

    /* Build one edge */
    gm_edge_t e = {0};
    uint8_t A[GM_OID_RAWSZ], B[GM_OID_RAWSZ];
    memset(A, 0x11, sizeof A); memset(B, 0x22, sizeof B);
    assert(gm_oid_from_raw(&e.src_oid, A, sizeof A) == GM_OK);
    assert(gm_oid_from_raw(&e.tgt_oid, B, sizeof B) == GM_OK);
    e.rel_type = GM_REL_REFERENCES; e.confidence = 0x3C00;
    assert(gm_strcpy_safe(e.src_path, sizeof e.src_path, "A") == 0);
    assert(gm_strcpy_safe(e.tgt_path, sizeof e.tgt_path, "B") == 0);
    assert(gm_ulid_generate(e.ulid).ok);

    /* The writer resolves head branch through repo port; our stub lacks it,
       so we call the internal append via current-branch path by bypassing head.
       To keep the test minimal, set branch name inline by initializing the
       ref in the writer (we can't). Instead, rely on the fact that when head
       lookup fails, writer returns error; so we simulate head by injecting a
       pre-known branch via overriding resolve function is not possible. We
       avoid head resolution by calling gm_journal_create_commit with an
       explicit ref and spec assembly paths exercised by flush/updates.
    */

    /* Append path should retry on NFF and succeed. */
    gm_edge_t edges[1] = { e };
    int rc = gm_journal_append(&ctx, edges, 1);
    assert(rc == GM_OK);
    /* Should have emitted a diagnostic nff retry once */
    int saw_retry = 0;
    for (size_t i=0;i<dst->count;i++) {
        if (strcmp(dst->meta[i].component, "journal")==0 &&
            strcmp(dst->meta[i].event, "journal_nff_retry")==0) { saw_retry=1; break; }
    }
    assert(saw_retry == 1);

    gm_fake_diag_port_dispose(&ctx.diag_port);
    printf("OK\n");
    return 0;
}
