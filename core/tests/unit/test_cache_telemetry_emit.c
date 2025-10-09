/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
extern int putenv(char *);
#endif

#include "gitmind/cache/internal/rebuild_service.h"
#include "gitmind/context.h"
#include "gitmind/error.h"
#include "gitmind/ports/git_repository_port.h"
#include "gitmind/ports/fs_temp_port.h"
#include "gitmind/security/memory.h"
#include "gitmind/util/memory.h"
#include "gitmind/security/string.h"
#include "core/tests/support/temp_repo_helpers.h"

#include "core/tests/fakes/fs/fake_fs_temp_port.h"
#include "core/tests/fakes/logging/fake_logger_port.h"
#include "core/tests/fakes/metrics/fake_metrics_port.h"

/* Minimal stub repository port for cache rebuild tests */
typedef struct {
    gm_git_repository_port_t port;
    char gitdir[GM_PATH_MAX];
    gm_oid_t next_tree;
    gm_oid_t next_commit;
} stub_repo_t;

static gm_result_void_t sr_repository_path(void *self,
                                           gm_git_repository_path_kind_t kind,
                                           char *out, size_t out_size) {
    (void)kind;
    stub_repo_t *sr = (stub_repo_t *)self;
    if (gm_strcpy_safe(out, out_size, sr->gitdir) != 0) {
        return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL, "stub repo path too long"));
    }
    return gm_ok_void();
}

static gm_result_void_t sr_build_tree(void *self, const char *dir, gm_oid_t *out_tree) {
    (void)self; (void)dir;
    if (out_tree) {
        gm_memset_safe(out_tree, sizeof(*out_tree), 0, sizeof(*out_tree));
        out_tree->id[0] = 0xAA;
    }
    return gm_ok_void();
}

static gm_result_void_t sr_reference_tip(void *self, const char *ref_name,
                                         gm_git_reference_tip_t *out_tip) {
    (void)self; (void)ref_name;
    if (out_tip) {
        gm_memset_safe(out_tip, sizeof(*out_tip), 0, sizeof(*out_tip));
        out_tip->has_target = false; /* simulate no prior commits */
    }
    return gm_ok_void();
}

static gm_result_void_t sr_commit_create(void *self, const gm_git_commit_spec_t *spec,
                                         gm_oid_t *out_commit_oid) {
    (void)self; (void)spec;
    if (out_commit_oid) {
        gm_memset_safe(out_commit_oid, sizeof(*out_commit_oid), 0, sizeof(*out_commit_oid));
        out_commit_oid->id[0] = 0xCC;
    }
    return gm_ok_void();
}

static gm_result_void_t sr_reference_update(void *self,
                                            const gm_git_reference_update_spec_t *spec) {
    (void)self; (void)spec;
    return gm_ok_void();
}

static gm_result_void_t sr_commit_tree_size(void *self, const gm_oid_t *commit_oid,
                                            uint64_t *out_size_bytes) {
    (void)self; (void)commit_oid;
    if (out_size_bytes) *out_size_bytes = 1234;
    return gm_ok_void();
}

static gm_result_void_t sr_walk_commits(void *self, const char *ref_name,
                                        gm_git_commit_visit_cb cb, void *ud) {
    (void)self; (void)ref_name; (void)cb; (void)ud;
    /* No commits to visit; succeed */
    return gm_ok_void();
}

static const gm_git_repository_port_vtbl_t SR_VTBL = {
    .repository_path = sr_repository_path,
    .head_branch = NULL,
    .build_tree_from_directory = sr_build_tree,
    .reference_tip = sr_reference_tip,
    .reference_glob_latest = NULL,
    .commit_read_blob = NULL,
    .commit_read_message = NULL,
    .commit_message_dispose = NULL,
    .walk_commits = sr_walk_commits,
    .commit_tree_size = sr_commit_tree_size,
    .commit_create = sr_commit_create,
    .reference_update = sr_reference_update,
    .resolve_blob_at_head = NULL,
    .resolve_blob_at_commit = NULL,
    .commit_parent_count = NULL,
};

static void set_env(const char *k, const char *v) {
    char buf[256];
    if (v == NULL) v = "";
    int n = gm_snprintf(buf, sizeof(buf), "%s=%s", k, v);
    assert(n >= 0 && (size_t)n < sizeof(buf));
    char *heap = (char *)malloc((size_t)n + 1);
    assert(heap != NULL);
    assert(gm_memcpy_span(heap, (size_t)n + 1, buf, (size_t)n + 1) == GM_OK);
    assert(putenv(heap) == 0);
}

static void setup_context(gm_context_t *ctx,
                          gm_fake_fs_temp_port_t *fs,
                          gm_logger_port_t *logger,
                          gm_metrics_port_t *metrics) {
    memset(ctx, 0, sizeof(*ctx));

    /* FS: fake with known roots so canonicalize works */
    assert(gm_fake_fs_temp_port_init(fs, "/fake/tmp", "/fake/state").ok);
    ctx->fs_temp_port = fs->port;
    ctx->fs_temp_port_dispose = NULL;

    /* Repo: stub */
    static stub_repo_t sr;
    memset(&sr, 0, sizeof(sr));
    int dir_copy = gm_strcpy_safe(sr.gitdir, sizeof(sr.gitdir), "/fake/state");
    assert(dir_copy == GM_OK);
    sr.port.vtbl = &SR_VTBL;
    sr.port.self = &sr;
    ctx->git_repo_port = sr.port;

    /* Logger & metrics */
    ctx->logger_port = *logger;
    ctx->metrics_port = *metrics;
}

static void test_metrics_disabled_logs_only(void) {
    printf("test_cache_telemetry_emit.disabled... ");
    set_env("GITMIND_METRICS_ENABLED", "0");
    set_env("GITMIND_METRICS_REPO_TAG", "hash");
    gm_fake_fs_temp_port_t fs;
    gm_logger_port_t log_port; gm_fake_logger_state_t *log_state = NULL;
    gm_metrics_port_t met_port; gm_fake_metrics_state_t *met_state = NULL;
    assert(gm_fake_logger_port_init(&log_port, &log_state).ok);
    assert(gm_fake_metrics_port_init(&met_port, &met_state).ok);

    gm_context_t ctx;
    setup_context(&ctx, &fs, &log_port, &met_port);

    int rc = gm_cache_rebuild_execute(&ctx, "main", true);
    assert(rc == GM_OK);

    /* No metrics emitted */
    assert(met_state->counter_count == 0);
    assert(met_state->gauge_count == 0);
    assert(met_state->timing_count == 0);

    /* Logs emitted: start + ok (2 entries) */
    assert(log_state->count >= 2);
    assert(strstr(log_state->message[0], "rebuild_start") != NULL);
    assert(strstr(log_state->message[log_state->count - 1], "rebuild_ok") != NULL);

    gm_fake_fs_temp_port_dispose(&fs);
    gm_fake_logger_port_dispose(&log_port);
    gm_fake_metrics_port_dispose(&met_port);
    printf("OK\n");
}

static void test_metrics_enabled_with_tags(void) {
    printf("test_cache_telemetry_emit.enabled... ");
    set_env("GITMIND_METRICS_ENABLED", "1");
    set_env("GITMIND_METRICS_BRANCH_TAG", "1");
    set_env("GITMIND_METRICS_MODE_TAG", "1");
    set_env("GITMIND_METRICS_REPO_TAG", "hash");
    set_env("GITMIND_METRICS_EXTRA_TAGS", "team=dev,role=ops");

    gm_fake_fs_temp_port_t fs;
    gm_logger_port_t log_port; gm_fake_logger_state_t *log_state = NULL;
    gm_metrics_port_t met_port; gm_fake_metrics_state_t *met_state = NULL;
    assert(gm_fake_logger_port_init(&log_port, &log_state).ok);
    assert(gm_fake_metrics_port_init(&met_port, &met_state).ok);

    gm_context_t ctx;
    setup_context(&ctx, &fs, &log_port, &met_port);

    int rc = gm_cache_rebuild_execute(&ctx, "dev", true);
    assert(rc == GM_OK);

    /* Metrics emitted (at least one timing entry). Additional counters from
     * journal.read.* may be present due to internal reads during rebuild. */
    assert(met_state->timing_count >= 1);
    /* Ensure cache edges counter exists (allow others as well). */
    size_t found_cache_edges = 0;
    for (size_t i = 0; i < met_state->counter_count; ++i) {
        if (strcmp(met_state->counters[i].name, "cache.edges_processed_total") == 0) {
            found_cache_edges = 1; break;
        }
    }
    assert(found_cache_edges == 1);
    /* Gauge should be present (tree size). */
    assert(met_state->gauge_count >= 1);

    /* Check tags content on cache.rebuild.duration_ms specifically */
    const char *tags = NULL;
    for (size_t i = 0; i < met_state->timing_count; ++i) {
        if (strcmp(met_state->timings[i].name, "cache.rebuild.duration_ms") == 0) {
            tags = met_state->timings[i].tags; break;
        }
    }
    assert(tags != NULL);
    assert(strstr(tags, "branch=dev") != NULL);
    assert(strstr(tags, "mode=full") != NULL);
    assert(strstr(tags, "repo=") != NULL);
    assert(strstr(tags, "team=dev") != NULL);
    assert(strstr(tags, "role=ops") != NULL);

    /* Start and OK logs exist */
    assert(log_state->count >= 2);
    assert(strstr(log_state->message[0], "rebuild_start") != NULL);
    assert(strstr(log_state->message[log_state->count - 1], "rebuild_ok") != NULL);

    gm_fake_fs_temp_port_dispose(&fs);
    gm_fake_logger_port_dispose(&log_port);
    gm_fake_metrics_port_dispose(&met_port);
    printf("OK\n");
}

int main(void) {
    test_metrics_disabled_logs_only();
    test_metrics_enabled_with_tags();
    return 0;
}
