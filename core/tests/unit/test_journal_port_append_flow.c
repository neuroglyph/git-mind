/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "gitmind/context.h"
#include "gitmind/edge.h"
#include "gitmind/journal.h"
#include "gitmind/ports/journal_command_port.h"
#include "gitmind/util/oid.h"
#include "gitmind/types/ulid.h"

#include "core/tests/fakes/git/fake_git_repository_port.h"
#include "core/tests/fakes/fs/fake_fs_temp_port.h"
#include "core/tests/fakes/logging/fake_logger_port.h"
#include "core/tests/fakes/metrics/fake_metrics_port.h"
#include "core/tests/fakes/diagnostics/fake_diagnostics_port.h"
#include "gitmind/util/memory.h"

static int count_cb(const gm_edge_t *e, void *ud) {
    (void)e; size_t *n = (size_t *)ud; (*n)++; return GM_OK;
}

int main(void) {
    printf("test_journal_port_append_flow... ");
    gm_context_t ctx = {0};

    /* Repo fake */
    gm_fake_git_repository_port_t frepo;
    assert(gm_fake_git_repository_port_init(&frepo, "/fake/repo/.git", "/fake/repo").ok);
    assert(gm_fake_git_repository_port_set_head_branch(&frepo, "main").ok);
    ctx.git_repo_port = frepo.port;

    /* FS fake (for optional tag building) */
    gm_fake_fs_temp_port_t ffs; assert(gm_fake_fs_temp_port_init(&ffs, "/fake/tmp", "/fake/state").ok);
    ctx.fs_temp_port = ffs.port;

    /* Logger, metrics, diagnostics */
    gm_logger_port_t lport; gm_fake_logger_state_t *lstate = NULL; assert(gm_fake_logger_port_init(&lport, &lstate).ok);
    ctx.logger_port = lport;
    gm_metrics_port_t mport; gm_fake_metrics_state_t *mstate = NULL; assert(gm_fake_metrics_port_init(&mport, &mstate).ok);
    ctx.metrics_port = mport;
    gm_diagnostics_port_t dport; gm_fake_diag_state_t *dstate = NULL; assert(gm_fake_diag_port_init(&dport, &dstate).ok);
    ctx.diag_port = dport;

    /* Inbound journal port */
    gm_cmd_journal_port_t jport = {0};
    assert(gm_cmd_journal_port_init(&jport, &ctx).ok);

    /* Build two edges */
    gm_edge_t edges[2]; memset(edges, 0, sizeof edges);
    uint8_t A[GM_OID_RAWSZ], B[GM_OID_RAWSZ], C[GM_OID_RAWSZ];
    memset(A, 0x11, sizeof A); memset(B, 0x22, sizeof B); memset(C, 0x33, sizeof C);
    assert(gm_oid_from_raw(&edges[0].src_oid, A, sizeof A) == GM_OK);
    assert(gm_oid_from_raw(&edges[0].tgt_oid, B, sizeof B) == GM_OK);
    edges[0].rel_type = GM_REL_IMPLEMENTS; edges[0].confidence = 0x3C00;
    assert(gm_strcpy_safe(edges[0].src_path, sizeof edges[0].src_path, "A") == 0);
    assert(gm_strcpy_safe(edges[0].tgt_path, sizeof edges[0].tgt_path, "B") == 0);
    assert(gm_ulid_generate(edges[0].ulid).ok);
    assert(gm_oid_from_raw(&edges[1].src_oid, A, sizeof A) == GM_OK);
    assert(gm_oid_from_raw(&edges[1].tgt_oid, C, sizeof C) == GM_OK);
    edges[1].rel_type = GM_REL_DEPENDS_ON; edges[1].confidence = 0x3C00;
    assert(gm_strcpy_safe(edges[1].src_path, sizeof edges[1].src_path, "A") == 0);
    assert(gm_strcpy_safe(edges[1].tgt_path, sizeof edges[1].tgt_path, "C") == 0);
    assert(gm_ulid_generate(edges[1].ulid).ok);

    /* Append via inbound port */
    assert(jport.vtbl->append(&jport, edges, 2).ok);

    /* Verify repo update side-effects */
    const char *last_ref = gm_fake_git_repository_port_last_update_ref(&frepo);
    assert(last_ref != NULL && strstr(last_ref, "refs/gitmind/edges/") == last_ref);
    const char *last_msg = gm_fake_git_repository_port_last_commit_message(&frepo);
    assert(last_msg != NULL && last_msg[0] != '\0');

    /* Verify logs/metrics */
    int saw_append_start = 0, saw_append_ok = 0, saw_read_start = 0, saw_read_ok = 0;
    for (size_t i=0;i<lstate->count;i++) {
        const char *msg = lstate->message[i];
        if (strstr(msg, "journal_append_start")) saw_append_start=1;
        if (strstr(msg, "journal_append_ok")) saw_append_ok=1;
    }
    assert(saw_append_start && saw_append_ok);

    int idx_timing_append = -1, idx_counter_append = -1;
    for (size_t i=0;i<mstate->timing_count;i++) if (strcmp(mstate->timings[i].name, "journal.append.duration_ms")==0) idx_timing_append=(int)i;
    for (size_t i=0;i<mstate->counter_count;i++) if (strcmp(mstate->counters[i].name, "journal.append.edges_total")==0) idx_counter_append=(int)i;
    assert(idx_timing_append >= 0 && idx_counter_append >= 0 && mstate->counters[idx_counter_append].value >= 2);

    /* Read back via high-level API */
    size_t count = 0; assert(gm_journal_read(&ctx, NULL, count_cb, &count) == GM_OK);
    assert(count >= 2);
    for (size_t i=0;i<lstate->count;i++) {
        const char *msg = lstate->message[i];
        if (strstr(msg, "journal_read_start")) saw_read_start=1;
        if (strstr(msg, "journal_read_ok")) saw_read_ok=1;
    }
    assert(saw_read_start && saw_read_ok);
    int idx_timing_read = -1, idx_counter_read = -1;
    for (size_t i=0;i<mstate->timing_count;i++) if (strcmp(mstate->timings[i].name, "journal.read.duration_ms")==0) idx_timing_read=(int)i;
    for (size_t i=0;i<mstate->counter_count;i++) if (strcmp(mstate->counters[i].name, "journal.read.edges_total")==0) idx_counter_read=(int)i;
    assert(idx_timing_read >= 0 && idx_counter_read >= 0 && mstate->counters[idx_counter_read].value >= 2);

    /* Diagnostics should be empty on success */
    assert(dstate->count == 0);

    gm_cmd_journal_port_dispose(&jport);
    gm_fake_diag_port_dispose(&ctx.diag_port);
    gm_fake_metrics_port_dispose(&ctx.metrics_port);
    gm_fake_logger_port_dispose(&ctx.logger_port);
    gm_fake_fs_temp_port_dispose(&ffs);
    gm_fake_git_repository_port_dispose(&frepo);
    printf("OK\n");
    return 0;
}
