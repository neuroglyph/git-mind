/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/cache/internal/rebuild_service.h"

#include <git2/oid.h>

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "gitmind/cache.h"
#include "gitmind/cache/bitmap.h"
#include "gitmind/cache/internal/edge_map.h"
#include "gitmind/constants_internal.h"
#include "gitmind/context.h"
#include "gitmind/edge.h"
#include "gitmind/error.h"
#include "gitmind/journal.h"
#include "gitmind/ports/git_repository_port.h"
#include "gitmind/ports/fs_temp_port.h"
#include "gitmind/result.h"
#include "gitmind/security/memory.h"
#include "gitmind/security/string.h"
#include "gitmind/util/memory.h"
#include "gitmind/util/ref.h"

#define MAX_SHARD_PATH 32
#define CLOCKS_PER_MS                                                       \
    ((clock_t)((CLOCKS_PER_SEC + (MILLIS_PER_SECOND - 1)) / MILLIS_PER_SECOND))
#define CACHE_TEMP_COMPONENT "cache"

static int cache_get_journal_tip(const gm_git_repository_port_t *port,
                                 const char *branch, gm_cache_meta_t *meta);
static int cache_create_commit(const gm_git_repository_port_t *port,
                               const gm_oid_t *tree_oid,
                               const gm_cache_meta_t *meta,
                               gm_oid_t *commit_oid);
static int cache_update_ref(const gm_git_repository_port_t *port,
                            const char *branch, const gm_oid_t *commit_oid);

static int unwrap_result(gm_result_void_t result) {
    if (result.ok) {
        return GM_OK;
    }

    int code = GM_ERR_UNKNOWN;
    if (result.u.err != NULL) {
        code = result.u.err->code;
        gm_error_free(result.u.err);
    }
    return code;
}

static int compute_repo_id(gm_context_t *ctx, gm_repo_id_t *repo_id) {
    if (ctx == NULL || repo_id == NULL) {
        return GM_ERR_INVALID_ARGUMENT;
    }
    if (ctx->fs_temp_port.vtbl == NULL || ctx->git_repo_port.vtbl == NULL) {
        return GM_ERR_INVALID_STATE;
    }

    char repo_path[GM_PATH_MAX];
    int code = unwrap_result(gm_git_repository_port_repository_path(
        &ctx->git_repo_port, GM_GIT_REPOSITORY_PATH_GITDIR, repo_path,
        sizeof(repo_path)));
    if (code != GM_OK) {
        return code;
    }

    const char *canonical = NULL;
    gm_fs_canon_opts_t canon_opts = {.mode = GM_FS_CANON_PHYSICAL_EXISTING};
    code = unwrap_result(gm_fs_temp_port_canonicalize_ex(
        &ctx->fs_temp_port, repo_path, canon_opts, &canonical));
    if (code != GM_OK) {
        return code;
    }

    return unwrap_result(gm_repo_id_from_path(canonical, repo_id));
}

static int make_temp_workspace(gm_context_t *ctx, gm_tempdir_t *temp_dir) {
    if (temp_dir == NULL) {
        return GM_ERR_INVALID_ARGUMENT;
    }

    gm_repo_id_t repo_id = {0};
    int status = compute_repo_id(ctx, &repo_id);
    if (status != GM_OK) {
        return status;
    }

    return unwrap_result(gm_fs_temp_port_make_temp_dir(
        &ctx->fs_temp_port, repo_id, CACHE_TEMP_COMPONENT, true, temp_dir));
}

static void release_temp_dir(gm_context_t *ctx, const gm_tempdir_t *temp_dir) {
    if (ctx == NULL || temp_dir == NULL || temp_dir->path == NULL ||
        ctx->fs_temp_port.vtbl == NULL) {
        return;
    }

    gm_result_void_t remove_result =
        gm_fs_temp_port_remove_tree(&ctx->fs_temp_port, temp_dir->path);
    if (!remove_result.ok && remove_result.u.err != NULL) {
        gm_error_free(remove_result.u.err);
    }
}

static void get_oid_prefix(const gm_oid_t *oid, char *prefix, int bits) {
    int chars = (bits + 3) / BITS_PER_HEX_CHAR; /* Round up to hex chars */
    if (chars <= 0) {
        prefix[0] = '\0';
        return;
    }

    char hex[GM_OID_HEX_CHARS];
    git_oid_fmt(hex, oid); /* not null-terminated */
    if (chars > GM_OID_HEX_CHARS) {
        chars = GM_OID_HEX_CHARS;
    }

    int limit = (chars < (MAX_SHARD_PATH - 1)) ? chars : (MAX_SHARD_PATH - 1);
    for (int index = 0; index < limit; ++index) {
        prefix[index] = hex[index];
    }
    prefix[limit] = '\0';
}

static int oid_to_hex(const gm_oid_t *oid, char *out, size_t out_size) {
    if (out_size < SHA_HEX_SIZE) {
        return GM_ERR_INVALID_ARGUMENT;
    }
    (void)git_oid_tostr(out, out_size, oid);
    return GM_OK;
}

typedef struct {
    const char *temp_dir;
    int shard_bits;
    const char *suffix;
} edge_map_write_ctx_t;

typedef struct {
    uint32_t total_edges;
    clock_t start_time;
} cache_meta_inputs_t;

typedef struct {
    const char *branch;
    const gm_tempdir_t *temp_dir;
} cache_commit_inputs_t;

static bool is_valid_directory_name(char *path_buffer, size_t buffer_size,
                                    const char *temp_dir, const char *prefix) {
    if (gm_snprintf(path_buffer, buffer_size, "%s/%s", temp_dir, prefix) < 0) {
        return false;
    }
    if (mkdir(path_buffer, DIR_PERMS_NORMAL) == 0) {
        return true;
    }

    struct stat existing = {0};
    if (stat(path_buffer, &existing) == 0 && S_ISDIR(existing.st_mode)) {
        return true;
    }

    return false;
}

static int edge_map_write_callback(const gm_oid_t *oid,
                                   const roaring_bitmap_t *bitmap,
                                   void *userdata) {
    edge_map_write_ctx_t *ctx = (edge_map_write_ctx_t *)userdata;
    char path[GM_PATH_MAX];
    char prefix[MAX_SHARD_PATH];
    get_oid_prefix(oid, prefix, ctx->shard_bits);

    if (!is_valid_directory_name(path, sizeof(path), ctx->temp_dir, prefix)) {
        return GM_ERR_IO_FAILED;
    }

    char sha_hex[SHA_HEX_SIZE];
    int hex_status = oid_to_hex(oid, sha_hex, sizeof(sha_hex));
    if (hex_status != GM_OK) {
        return hex_status;
    }

    if (gm_snprintf(path, sizeof(path), "%s/%s/%s.%s", ctx->temp_dir, prefix,
                    sha_hex, ctx->suffix) < 0) {
        return GM_ERR_UNKNOWN;
    }

    return gm_bitmap_write_file(bitmap, path);
}

static int write_map_to_temp(gm_edge_map_t *map, const char *temp_dir,
                             int shard_bits, const char *suffix) {
    edge_map_write_ctx_t ctx = {
        .temp_dir = temp_dir,
        .shard_bits = shard_bits,
        .suffix = suffix,
    };

    gm_result_void_t visit_result =
        gm_edge_map_visit(map, edge_map_write_callback, &ctx);
    return unwrap_result(visit_result);
}

static int write_bitmaps_to_temp(gm_edge_map_t *forward, gm_edge_map_t *reverse,
                                 const char *temp_dir, int shard_bits) {
    int status = write_map_to_temp(forward, temp_dir, shard_bits, "forward");
    if (status != GM_OK) {
        return status;
    }
    return write_map_to_temp(reverse, temp_dir, shard_bits, "reverse");
}

static int cache_build_edge_map(gm_edge_map_t **forward_map,
                                gm_edge_map_t **reverse_map) {
    gm_result_void_t forward_result = gm_edge_map_create(EDGE_MAP_BUCKETS, forward_map);
    int result_code = unwrap_result(forward_result);
    if (result_code != GM_OK) {
        return result_code;
    }

    gm_result_void_t reverse_result = gm_edge_map_create(EDGE_MAP_BUCKETS, reverse_map);
    result_code = unwrap_result(reverse_result);
    if (result_code != GM_OK) {
        gm_edge_map_destroy(*forward_map);
        *forward_map = NULL;
        return result_code;
    }

    return GM_OK;
}

static int build_tree_from_temp(const gm_git_repository_port_t *port,
                                const char *temp_dir, gm_oid_t *tree_oid) {
    if (port == NULL || port->vtbl == NULL) {
        return GM_ERR_INVALID_STATE;
    }
    return unwrap_result(gm_git_repository_port_build_tree_from_directory(
        port, temp_dir, tree_oid));
}

static int cache_prepare_rebuild(gm_context_t *ctx,
                                 const char *branch __attribute__((unused)),
                                 bool force_full, gm_cache_meta_t *old_meta,
                                 bool *has_old_cache, gm_tempdir_t *temp_dir) {
    *has_old_cache = false;
    if (!force_full) {
        int load_status = gm_cache_load_meta(ctx, branch, old_meta);
        *has_old_cache = (load_status == GM_OK);
    }

    return make_temp_workspace(ctx, temp_dir);
}

typedef struct {
    gm_edge_map_t *forward;
    gm_edge_map_t *reverse;
    uint32_t edge_id;
} edge_collector_t;

static int collect_edge_callback(const gm_edge_t *edge, void *userdata) {
    edge_collector_t *collector = (edge_collector_t *)userdata;
    gm_result_void_t forward_result =
        gm_edge_map_add(collector->forward, &edge->src_oid, collector->edge_id);
    int code = unwrap_result(forward_result);
    if (code != GM_OK) {
        return code;
    }

    gm_result_void_t reverse_result =
        gm_edge_map_add(collector->reverse, &edge->tgt_oid, collector->edge_id);
    code = unwrap_result(reverse_result);
    if (code != GM_OK) {
        return code;
    }

    collector->edge_id++;
    return GM_OK;
}

static int cache_collect_edges(gm_context_t *ctx, const char *branch,
                               gm_edge_map_t *forward_map, gm_edge_map_t *reverse_map,
                               uint32_t starting_edge_id, uint32_t *total_edges) {
    edge_collector_t collector = {
        .forward = forward_map,
        .reverse = reverse_map,
        .edge_id = starting_edge_id,
    };

    int result_code = gm_journal_read(ctx, branch, collect_edge_callback, &collector);
    if (result_code == GM_OK) {
        *total_edges = collector.edge_id;
    }
    return result_code;
}

static int cache_collect_and_write(gm_context_t *ctx, const char *branch,
                                   gm_edge_map_t *forward_map,
                                   gm_edge_map_t *reverse_map,
                                   bool has_old_cache,
                                   const gm_cache_meta_t *old_meta,
                                   const gm_tempdir_t *temp_dir,
                                   uint32_t *total_edges) {
    if (temp_dir == NULL || temp_dir->path == NULL) {
        return GM_ERR_INVALID_ARGUMENT;
    }
    uint32_t starting_edge_id = has_old_cache ? (uint32_t)old_meta->edge_count : 0U;
    int result_code = cache_collect_edges(ctx, branch, forward_map, reverse_map,
                                          starting_edge_id, total_edges);
    if (result_code != GM_OK) {
        return result_code;
    }

    return write_bitmaps_to_temp(forward_map, reverse_map, temp_dir->path,
                                 GM_CACHE_SHARD_BITS);
}

static int cache_populate_meta(const gm_git_repository_port_t *port,
                               const char *branch,
                               const cache_meta_inputs_t *inputs,
                               gm_cache_meta_t *meta) {
    meta->journal_tip_time = (uint64_t)time(NULL);
    meta->edge_count = inputs->total_edges;
    meta->build_time_ms =
        (uint64_t)((clock() - inputs->start_time) / CLOCKS_PER_MS);
    meta->shard_bits = GM_CACHE_SHARD_BITS;
    meta->version = GM_CACHE_VERSION;
    if (strlen(branch) >= GM_CACHE_BRANCH_NAME_SIZE) {
        return GM_ERR_INVALID_ARGUMENT;
    }
    (void)gm_strcpy_safe(meta->branch, GM_CACHE_BRANCH_NAME_SIZE, branch);

    return cache_get_journal_tip(port, branch, meta);
}

static int cache_build_commit_and_update(const gm_git_repository_port_t *port,
                                         const cache_commit_inputs_t *inputs,
                                         const gm_cache_meta_t *meta) {
    if (inputs == NULL || inputs->temp_dir == NULL ||
        inputs->temp_dir->path == NULL) {
        return GM_ERR_INVALID_ARGUMENT;
    }
    gm_oid_t tree_oid;
    int result_code =
        build_tree_from_temp(port, inputs->temp_dir->path, &tree_oid);
    if (result_code != GM_OK) {
        return result_code;
    }

    gm_oid_t commit_oid;
    result_code = cache_create_commit(port, &tree_oid, meta, &commit_oid);
    if (result_code != GM_OK) {
        return result_code;
    }

    return cache_update_ref(port, inputs->branch, &commit_oid);
}

static int cache_get_journal_tip(const gm_git_repository_port_t *port,
                                 const char *branch,
                                 gm_cache_meta_t *meta) {
    char journal_ref_name[REF_NAME_BUFFER_SIZE];
    {
        int brc = gm_build_ref(journal_ref_name, sizeof(journal_ref_name),
                               GITMIND_EDGES_REF_PREFIX, branch);
        if (brc != GM_OK) {
            return brc;
        }
    }
    gm_git_reference_tip_t tip = {0};
    int tip_status = unwrap_result(
        gm_git_repository_port_reference_tip(port, journal_ref_name, &tip));
    if (tip_status != GM_OK) {
        return tip_status;
    }

    if (!tip.has_target) {
        meta->journal_tip_oid[0] = '\0';
        gm_memset_safe(&meta->journal_tip_oid_bin,
                       sizeof(meta->journal_tip_oid_bin), 0,
                       sizeof(meta->journal_tip_oid_bin));
        return GM_OK;
    }

    if (gm_strcpy_safe(meta->journal_tip_oid, sizeof(meta->journal_tip_oid),
                       tip.oid_hex) != GM_OK) {
        return GM_ERR_PATH_TOO_LONG;
    }
    meta->journal_tip_oid_bin = tip.oid;
    meta->journal_tip_time = tip.commit_time;
    return GM_OK;
}

static int cache_create_commit(const gm_git_repository_port_t *port,
                               const gm_oid_t *tree_oid,
                               const gm_cache_meta_t *meta __attribute__((unused)),
                               gm_oid_t *commit_oid) {
    if (port == NULL || port->vtbl == NULL) {
        return GM_ERR_INVALID_STATE;
    }

    gm_git_commit_spec_t spec = {
        .tree_oid = tree_oid,
        .message = "Cache metadata",
    };

    return unwrap_result(
        gm_git_repository_port_commit_create(port, &spec, commit_oid));
}

static int cache_update_ref(const gm_git_repository_port_t *port,
                            const char *branch, const gm_oid_t *commit_oid) {
    char ref_name[REF_NAME_BUFFER_SIZE];
    {
        int ref_status =
            gm_build_ref(ref_name, sizeof(ref_name), GM_CACHE_REF_PREFIX, branch);
        if (ref_status != GM_OK) {
            return ref_status;
        }
    }

    gm_git_reference_update_spec_t update_spec = {
        .ref_name = ref_name,
        .target_oid = commit_oid,
        .log_message = "Cache rebuild",
        .force = true,
    };

    return unwrap_result(
        gm_git_repository_port_reference_update(port, &update_spec));
}

static void cache_cleanup(gm_context_t *ctx, gm_edge_map_t *forward,
                          gm_edge_map_t *reverse, const gm_tempdir_t *temp_dir) {
    gm_edge_map_destroy(forward);
    gm_edge_map_destroy(reverse);
    release_temp_dir(ctx, temp_dir);
}

int gm_cache_rebuild_execute(gm_context_t *ctx, const char *branch,
                             bool force_full) {
    if (ctx == NULL || branch == NULL) {
        return GM_ERR_INVALID_ARGUMENT;
    }
    if (ctx->fs_temp_port.vtbl == NULL || ctx->git_repo_port.vtbl == NULL) {
        return GM_ERR_INVALID_STATE;
    }

    gm_edge_map_t *forward_map = NULL;
    gm_edge_map_t *reverse_map = NULL;
    gm_tempdir_t temp_dir = {0};
    gm_cache_meta_t old_meta = {0};
    gm_cache_meta_t meta = {0};
    bool has_old_cache = false;
    int result_code = GM_OK;
    clock_t start_time;

    result_code = cache_prepare_rebuild(ctx, branch, force_full, &old_meta,
                                        &has_old_cache, &temp_dir);
    if (result_code != GM_OK) {
        return result_code;
    }

    result_code = cache_build_edge_map(&forward_map, &reverse_map);
    if (result_code != GM_OK) {
        cache_cleanup(ctx, forward_map, reverse_map, &temp_dir);
        return result_code;
    }

    start_time = clock();
    uint32_t total_edges = 0;

    result_code = cache_collect_and_write(ctx, branch, forward_map, reverse_map,
                                          has_old_cache, &old_meta, &temp_dir,
                                          &total_edges);
    if (result_code != GM_OK) {
        goto cleanup;
    }

    cache_meta_inputs_t meta_inputs = {
        .total_edges = total_edges,
        .start_time = start_time,
    };
    result_code =
        cache_populate_meta(&ctx->git_repo_port, branch, &meta_inputs, &meta);
    if (result_code != GM_OK) {
        goto cleanup;
    }

    cache_commit_inputs_t commit_inputs = {
        .branch = branch,
        .temp_dir = &temp_dir,
    };
    result_code = cache_build_commit_and_update(&ctx->git_repo_port,
                                                &commit_inputs, &meta);

cleanup:
    cache_cleanup(ctx, forward_map, reverse_map, &temp_dir);
    return result_code;
}
