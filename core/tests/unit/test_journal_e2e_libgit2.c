/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <errno.h>
#include <git2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#include "gitmind/context.h"
#include "gitmind/error.h"
#include "gitmind/edge.h"
#include "gitmind/journal.h"
#include "gitmind/ports/journal_command_port.h"
#include "gitmind/security/string.h"

#include "gitmind/adapters/fs/posix_temp_adapter.h"
#include "gitmind/adapters/git/libgit2_repository_port.h"
#include "core/tests/support/temp_repo_helpers.h"

#include "core/tests/fakes/logging/fake_logger_port.h"
#include "core/tests/fakes/metrics/fake_metrics_port.h"

static void write_file(const char *path, const char *content) {
    FILE *f = fopen(path, "wb");
    assert(f != NULL);
    size_t n = fwrite(content, 1, strlen(content), f);
    assert(n == strlen(content));
    fclose(f);
}

static int init_repo_with_main(const char *repo_path, git_repository **out_repo) {
    git_repository_init_options opts;
    git_repository_init_options_init(&opts, GIT_REPOSITORY_INIT_OPTIONS_VERSION);
    opts.flags = GIT_REPOSITORY_INIT_MKPATH; /* create path */
    opts.initial_head = "main";
    int rc = git_repository_init_ext(out_repo, repo_path, &opts);
    if (rc != 0) return rc;

    /* Create two working files */
    char a_path[GM_PATH_MAX];
    char b_path[GM_PATH_MAX];
    int a_fmt = gm_snprintf(a_path, sizeof(a_path), "%s/%s", repo_path, "a.txt");
    int b_fmt = gm_snprintf(b_path, sizeof(b_path), "%s/%s", repo_path, "b.txt");
    assert(a_fmt >= 0 && (size_t)a_fmt < sizeof(a_path));
    assert(b_fmt >= 0 && (size_t)b_fmt < sizeof(b_path));
    write_file(a_path, "A\n");
    write_file(b_path, "B\n");

    git_index *idx = NULL;
    rc = git_repository_index(&idx, *out_repo);
    if (rc != 0) return rc;
    rc = git_index_add_bypath(idx, "a.txt");
    if (rc == 0) rc = git_index_add_bypath(idx, "b.txt");
    if (rc == 0) rc = git_index_write(idx);

    git_oid tree_oid;
    if (rc == 0) rc = git_index_write_tree(&tree_oid, idx);
    git_index_free(idx);
    if (rc != 0) return rc;

    git_tree *tree = NULL;
    rc = git_tree_lookup(&tree, *out_repo, &tree_oid);
    if (rc != 0) return rc;

    git_signature *sig = NULL;
    rc = git_signature_now(&sig, "Tester", "tester@example.com");
    if (rc != 0) {
        git_tree_free(tree);
        return rc;
    }

    git_oid commit_oid;
    rc = git_commit_create(&commit_oid, *out_repo, "refs/heads/main", sig, sig,
                           "UTF-8", "init", tree, 0, NULL);
    git_signature_free(sig);
    git_tree_free(tree);
    return rc;
}

typedef struct { size_t count; } read_counter_t;

static int read_count_cb(const gm_edge_t *edge, void *ud) {
    (void)edge;
    read_counter_t *c = (read_counter_t *)ud;
    c->count++;
    return GM_OK;
}

static int metrics_find_timing(const gm_fake_metrics_state_t *st, const char *name) {
    for (size_t i = 0; i < st->timing_count; ++i) {
        if (strcmp(st->timings[i].name, name) == 0) return (int)i;
    }
    return -1;
}
static int metrics_find_counter(const gm_fake_metrics_state_t *st, const char *name) {
    for (size_t i = 0; i < st->counter_count; ++i) {
        if (strcmp(st->counters[i].name, name) == 0) return (int)i;
    }
    return -1;
}
static int logger_contains(const gm_fake_logger_state_t *st, const char *needle) {
    for (size_t i = 0; i < st->count; ++i) {
        if (strstr(st->message[i], needle) != NULL) return 1;
    }
    return 0;
}

int main(void) {
    printf("test_journal_e2e_libgit2... ");
    git_libgit2_init();

    gm_context_t ctx = {0};

    /* FS temp (real posix) */
    gm_posix_fs_state_t *fs_state = NULL;
    void (*fs_dispose)(gm_fs_temp_port_t *) = NULL;
    gm_result_void_t rfs = gm_posix_fs_temp_port_create(&ctx.fs_temp_port, &fs_state, &fs_dispose);
    assert(rfs.ok);
    ctx.fs_temp_port_dispose = fs_dispose;

    char repo_dir[GM_PATH_MAX];
    gm_result_void_t repo_dir_rc =
        gm_test_make_temp_repo_dir(&ctx.fs_temp_port, "journal-e2e-repo",
                                   repo_dir, sizeof(repo_dir));
    assert(repo_dir_rc.ok);

    git_repository *repo = NULL;
    int rc = init_repo_with_main(repo_dir, &repo);
    assert(rc == 0 && repo);

    /* Git repo port (libgit2) */
    gm_libgit2_repository_port_state_t *git_state = NULL;
    void (*git_dispose)(gm_git_repository_port_t *) = NULL;
    gm_result_void_t rgit = gm_libgit2_repository_port_create(&ctx.git_repo_port, &git_state, &git_dispose, repo);
    assert(rgit.ok);
    ctx.git_repo_port_dispose = git_dispose;

    /* Logger + Metrics (fakes) */
    gm_fake_logger_state_t *log_state = NULL;
    gm_result_void_t rlog = gm_fake_logger_port_init(&ctx.logger_port, &log_state);
    assert(rlog.ok && log_state);
    gm_fake_metrics_state_t *met_state = NULL;
    gm_result_void_t rmet = gm_fake_metrics_port_init(&ctx.metrics_port, &met_state);
    assert(rmet.ok && met_state);

    /* Inbound journal port */
    gm_cmd_journal_port_t jport = {0};
    gm_result_void_t jinit = gm_cmd_journal_port_init(&jport, &ctx);
    assert(jinit.ok);

    /* Build two edges from files in HEAD */
    gm_result_edge_t e1r = gm_edge_create(&ctx, "a.txt", "b.txt", GM_REL_REFERENCES);
    assert(e1r.ok);
    gm_result_edge_t e2r = gm_edge_create(&ctx, "b.txt", "a.txt", GM_REL_DEPENDS_ON);
    assert(e2r.ok);
    gm_edge_t edges[2] = { e1r.u.val, e2r.u.val };

    /* Append via inbound port */
    gm_result_void_t app_res = jport.vtbl->append(&jport, edges, 2);
    assert(app_res.ok);

    /* Validate logging + metrics for append */
    assert(logger_contains(log_state, "journal_append_start"));
    assert(logger_contains(log_state, "journal_append_ok"));
    assert(metrics_find_timing(met_state, "journal.append.duration_ms") >= 0);
    int cidx = metrics_find_counter(met_state, "journal.append.edges_total");
    assert(cidx >= 0 && met_state->counters[cidx].value >= 2);

    /* Now read back using the high-level API */
    read_counter_t counter = {0};
    int read_rc = gm_journal_read(&ctx, NULL, read_count_cb, &counter);
    assert(read_rc == GM_OK);
    assert(counter.count >= 2);

    /* Validate read metrics and logs */
    assert(logger_contains(log_state, "journal_read_start"));
    assert(logger_contains(log_state, "journal_read_ok"));
    assert(metrics_find_timing(met_state, "journal.read.duration_ms") >= 0);
    int rcidx = metrics_find_counter(met_state, "journal.read.edges_total");
    assert(rcidx >= 0 && met_state->counters[rcidx].value >= 2);

    /* Cleanup */
    gm_cmd_journal_port_dispose(&jport);
    if (ctx.git_repo_port_dispose) ctx.git_repo_port_dispose(&ctx.git_repo_port);
    git_repository_free(repo);
    gm_result_void_t rm_repo = gm_fs_temp_port_remove_tree(&ctx.fs_temp_port, repo_dir);
    assert(rm_repo.ok);
    if (ctx.fs_temp_port_dispose) ctx.fs_temp_port_dispose(&ctx.fs_temp_port);
    git_libgit2_shutdown();

    printf("OK\n");
    return 0;
}
