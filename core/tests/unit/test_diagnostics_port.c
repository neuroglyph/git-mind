/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "gitmind/context.h"
#include "gitmind/ports/diagnostic_port.h"

#include "core/tests/fakes/diagnostics/fake_diagnostics_port.h"
#include "core/tests/fakes/fs/fake_fs_temp_port.h"

#include "gitmind/cache/internal/rebuild_service.h"
#include "gitmind/ports/git_repository_port.h"

/* Minimal stub repo: repository path resolves but tree build fails to trigger
 * diagnostics during cache rebuild. */
typedef struct { gm_git_repository_port_t port; } diag_stub_repo_t;

static gm_result_void_t sr_repo_path(void *self, gm_git_repository_path_kind_t kind,
                                     char *out, size_t out_size) {
    (void)self; (void)kind;
    if (out == NULL || out_size == 0) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "repo path buffer missing"));
    }
    const char *p = "/fake/state/.git";
    if (snprintf(out, out_size, "%s", p) < 0) {
        return gm_err_void(GM_ERROR(GM_ERR_UNKNOWN, "repo path format failed"));
    }
    return gm_ok_void();
}

static gm_result_void_t sr_build_tree(void *self, const char *dir, gm_oid_t *out) {
    (void)self; (void)dir; (void)out; return gm_err_void(GM_ERROR(GM_ERR_UNKNOWN, "fail"));
}

static const gm_git_repository_port_vtbl_t SR_VTBL = {
    .repository_path = sr_repo_path,
    .build_tree_from_directory = sr_build_tree,
};

int main(void) {
    printf("test_diagnostics_port... ");

    gm_context_t ctx = {0};

    /* Fake diagnostics sink */
    gm_fake_diag_state_t *diag_state = NULL;
    assert(gm_fake_diag_port_init(&ctx.diag_port, &diag_state).ok);

    /* Fake FS so prep can succeed enough to build temp dir path; mark roots. */
    gm_fake_fs_temp_port_t fakefs; assert(gm_fake_fs_temp_port_init(&fakefs, "/fake/tmp", "/fake/state").ok);
    ctx.fs_temp_port = fakefs.port;

    /* Stub git repo port */
    diag_stub_repo_t sr = {0}; sr.port.vtbl = &SR_VTBL; sr.port.self = &sr;
    ctx.git_repo_port = sr.port;

    /* Trigger cache rebuild; with fake FS canonicalization the prep will fail,
       which should emit a diagnostics event (rebuild_prep_failed). */
    int rc = gm_cache_rebuild_execute(&ctx, "main", true);
    assert(rc != GM_OK);
    assert(diag_state->count >= 1);

    /* Look for a cache/rebuild_*_failed event */
    int found = 0;
    for (size_t i = 0; i < diag_state->count; ++i) {
        if (strcmp(diag_state->meta[i].component, "cache") == 0 &&
            (strcmp(diag_state->meta[i].event, "rebuild_failed") == 0 ||
             strcmp(diag_state->meta[i].event, "rebuild_prep_failed") == 0 ||
             strcmp(diag_state->meta[i].event, "rebuild_edge_map_failed") == 0 ||
             strcmp(diag_state->meta[i].event, "rebuild_collect_write_failed") == 0 ||
             strcmp(diag_state->meta[i].event, "rebuild_meta_failed") == 0)) {
            found = 1; break;
        }
    }
    assert(found == 1);

    gm_fake_diag_port_dispose(&ctx.diag_port);
    gm_fake_fs_temp_port_dispose(&fakefs);
    printf("OK\n");
    return 0;
}
