/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <errno.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include <git2.h>

#include "gitmind/cache/internal/rebuild_service.h"
#include "gitmind/cache/internal/oid_prefix.h"
#include "gitmind/context.h"
#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/security/string.h"
#include "gitmind/util/ref.h"
#include "gitmind/ports/fs_temp_port.h"
#include "gitmind/adapters/fs/posix_temp_adapter.h"
#include "gitmind/adapters/git/libgit2_repository_port.h"
#include "core/tests/support/temp_repo_helpers.h"

static void write_file(const char *path, const char *content) {
    FILE *f = fopen(path, "wb");
    assert(f != NULL);
    size_t n = fwrite(content, 1, strlen(content), f);
    assert(n == strlen(content));
    fclose(f);
}

int main(void) {
    printf("test_cache_tree_size... ");
    int exit_code = 0;

    gm_cache_result_t result = {0};
    gm_cache_result_t builder_total = {0};
    gm_edge_map_t forward_map = {0};
    gm_edge_map_t reverse_map = {0};
    gm_git_repository_port_t port = {0};
    char repo_path[GM_PATH_MAX] = {0};
    char src_dir[GM_PATH_MAX] = {0};
    gm_tempdir_t temp_repo = {.path = repo_path};
    gm_tempdir_t temp_src = {.path = src_dir};
    gm_fs_temp_port_t fs_port = {0};
    gm_posix_fs_state_t *fs_state = NULL;
    void (*fs_dispose)(gm_fs_temp_port_t *) = NULL;
    gm_libgit2_repository_port_state_t *state = NULL;
    void (*port_dispose)(gm_git_repository_port_t *) = NULL;
    git_repository *repo = NULL;

    git_libgit2_init();

    gm_result_void_t fs_result =
        gm_posix_fs_temp_port_create(&fs_port, &fs_state, &fs_dispose);
    if (!fs_result.ok) {
        exit_code = 1;
        goto cleanup;
    }

    if (!gm_test_make_temp_repo_dir(&fs_port, "cache-tree-repo",
                                    repo_path, sizeof(repo_path)).ok) {
        exit_code = 1;
        goto cleanup;
    }

    if (!gm_test_make_temp_repo_dir(&fs_port, "cache-tree-src",
                                    src_dir, sizeof(src_dir)).ok) {
        exit_code = 1;
        goto cleanup;
    }

    if (git_repository_init(&repo, temp_repo.path, true) != 0 || repo == NULL) {
        exit_code = 1;
        goto cleanup;
    }

    char root_file[GM_PATH_MAX];
    if (gm_snprintf(root_file, sizeof(root_file), "%s/root.txt", temp_src.path) < 0) {
        exit_code = 1;
        goto cleanup;
    }
    write_file(root_file, "root\n");

    gm_context_t ctx = {0};
    gm_cache_ctx_t cache_ctx = {0};
    ctx.cache_ctx = &cache_ctx;

    gm_result_void_t port_result =
        gm_libgit2_repository_port_create(&port, &state, &port_dispose, repo);
    if (!port_result.ok) {
        exit_code = 1;
        goto cleanup;
    }

    gm_cache_context_inputs_t inputs = {
        .temp_dir = &temp_src,
        .branch = "main",
        .reverse_edge_map = &reverse_map,
        .forward_edge_map = &forward_map,
        .total_edges = 1U,
    };

    gm_cache_meta_t meta;
    memset(&meta, 0, sizeof(meta));
    if (cache_collect_metadata(&port, &inputs, &meta) != GM_OK) {
        exit_code = 1;
        goto cleanup;
    }

    gm_result_void_t visit_result =
        gm_cache_collect_edges(&port, &ctx, inputs.temp_dir->path,
                               &forward_map, &reverse_map);
    if (!visit_result.ok) {
        exit_code = 1;
        goto cleanup;
    }

    uint64_t total = 0;
    if (gm_cache_tree_size(forward_map.root, &total) != GM_OK ||
        total != inputs.total_edges) {
        exit_code = 1;
        goto cleanup;
    }

    cache_builder_inputs_t cb_inputs = {
        .branch = "main",
        .forward = forward_map.root,
        .reverse = reverse_map.root,
        .total_edges = inputs.total_edges,
        .temp_dir = &temp_src,
    };
    if (cache_build_collect(&port, &cb_inputs, &meta, &result) != GM_OK ||
        result.total_edges != inputs.total_edges) {
        exit_code = 1;
        goto cleanup;
    }

    if (gm_cache_result_merge(&result, &builder_total) != GM_OK ||
        builder_total.total_edges != inputs.total_edges) {
        exit_code = 1;
        goto cleanup;
    }

cleanup:
    gm_cache_result_dispose(&result);
    gm_cache_result_dispose(&builder_total);
    gm_edge_map_dispose(&forward_map);
    gm_edge_map_dispose(&reverse_map);
    if (port_dispose != NULL) {
        port_dispose(&port);
    }
    if (repo != NULL) {
        git_repository_free(repo);
    }
    if (fs_port.vtbl != NULL && repo_path[0] != '\0') {
        gm_fs_temp_port_remove_tree(&fs_port, repo_path);
    }
    if (fs_port.vtbl != NULL && src_dir[0] != '\0') {
        gm_fs_temp_port_remove_tree(&fs_port, src_dir);
    }
    if (fs_dispose != NULL) {
        fs_dispose(&fs_port);
    }
    git_libgit2_shutdown();
    printf(exit_code == 0 ? "OK\n" : "FAIL\n");
    return exit_code;
}
