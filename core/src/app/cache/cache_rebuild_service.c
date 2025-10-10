/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/cache/internal/rebuild_service.h"

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
#include "gitmind/cache/internal/oid_prefix.h"
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
#include "gitmind/util/oid.h"
#include "gitmind/util/ref.h"
#include "gitmind/telemetry/internal/config.h"
#include "gitmind/ports/diagnostic_port.h"

#define CLOCKS_PER_MS                                                       \
    ((clock_t)((CLOCKS_PER_SEC + (MILLIS_PER_SECOND - 1)) / MILLIS_PER_SECOND))
#define CACHE_TEMP_COMPONENT "cache"

static int cache_get_journal_tip(const gm_git_repository_port_t *port,
                                 const char *branch, gm_cache_meta_t *meta);
static int cache_create_commit(const gm_git_repository_port_t *port,
                               const gm_oid_t *parent_oid,
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

static void cache_diag_emit(gm_context_t *ctx, const char *event,
                            const char *branch, int code) {
    if (ctx == NULL) return;
    char code_buf[16];
    (void)gm_snprintf(code_buf, sizeof(code_buf), "%d", code);
    const gm_diag_kv_t kvs[] = {
        {.key = "branch", .value = branch ? branch : ""},
        {.key = "code", .value = code_buf},
    };
    (void)gm_diag_emit(&ctx->diag_port, "cache", event, kvs,
                       sizeof(kvs) / sizeof(kvs[0]));
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

/* moved to domain helper: gm_cache_oid_prefix */

static int oid_to_hex(const gm_oid_t *oid, char *out, size_t out_size) {
    return gm_oid_to_hex(oid, out, out_size);
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
    char prefix[GM_CACHE_MAX_SHARD_PATH];
    int prefix_rc = gm_cache_oid_prefix(oid, ctx->shard_bits, prefix,
                                        sizeof(prefix));
    if (prefix_rc != GM_OK) {
        return prefix_rc;
    }

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
    if (gm_strcpy_safe(meta->branch, GM_CACHE_BRANCH_NAME_SIZE, branch) != GM_OK) {
        gm_memset_safe(meta->branch, sizeof(meta->branch), 0, sizeof(meta->branch));
        return GM_ERR_BUFFER_TOO_SMALL;
    }

    return cache_get_journal_tip(port, branch, meta);
}

static int cache_build_commit_and_update(const gm_git_repository_port_t *port,
                                         const cache_commit_inputs_t *inputs,
                                         const gm_cache_meta_t *meta,
                                         gm_oid_t *out_commit_oid) {
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

    char ref_name[REF_NAME_BUFFER_SIZE];
    {
        int ref_status =
            gm_build_ref(ref_name, sizeof(ref_name), GM_CACHE_REF_PREFIX, inputs->branch);
        if (ref_status != GM_OK) {
            return ref_status;
        }
    }

    gm_git_reference_tip_t cache_tip = {0};
    int tip_status = unwrap_result(
        gm_git_repository_port_reference_tip(port, ref_name, &cache_tip));
    if (tip_status != GM_OK && tip_status != GM_ERR_NOT_FOUND) {
        return tip_status;
    }

    const gm_oid_t *parent_oid =
        (tip_status == GM_OK && cache_tip.has_target) ? &cache_tip.oid : NULL;

    gm_oid_t commit_oid;
    result_code = cache_create_commit(port, parent_oid, &tree_oid, meta, &commit_oid);
    if (result_code != GM_OK) {
        return result_code;
    }

    int ref_rc = cache_update_ref(port, inputs->branch, &commit_oid);
    if (ref_rc != GM_OK) {
        return ref_rc;
    }
    if (out_commit_oid != NULL) {
        *out_commit_oid = commit_oid;
    }
    return GM_OK;
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
                               const gm_oid_t *parent_oid,
                               const gm_oid_t *tree_oid,
                               const gm_cache_meta_t *meta __attribute__((unused)),
                               gm_oid_t *commit_oid) {
    if (port == NULL || port->vtbl == NULL) {
        return GM_ERR_INVALID_STATE;
    }

    gm_git_commit_spec_t spec = {
        .tree_oid = tree_oid,
        .message = "Cache metadata",
        .parents = parent_oid,
        .parent_count = (parent_oid != NULL) ? 1U : 0U,
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
        .force = false,
    };

    gm_result_void_t update_result =
        gm_git_repository_port_reference_update(port, &update_spec);
    if (!update_result.ok) {
        int update_code = GM_ERR_UNKNOWN;
        if (update_result.u.err != NULL) {
            update_code = update_result.u.err->code;
            gm_error_free(update_result.u.err);
        }
        return update_code;
    }

    return GM_OK;
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
    char stable_temp_path[GM_PATH_MAX];
    gm_cache_meta_t old_meta = {0};
    gm_cache_meta_t meta = {0};
    bool has_old_cache = false;
    int result_code = GM_OK;
    clock_t start_time;

    /* Telemetry configuration */
    gm_telemetry_cfg_t tcfg = {0};
    gm_result_void_t telemetry_rc =
        gm_telemetry_cfg_load(&tcfg, gm_env_port_system());
    if (!telemetry_rc.ok) {
        char err_msg[96];
        int32_t code = telemetry_rc.u.err ? telemetry_rc.u.err->code
                                          : GM_ERR_UNKNOWN;
        int err_snprintf =
            gm_snprintf(err_msg, sizeof(err_msg),
                        "telemetry_cfg_load_failed code=%d", (int)code);
        if (telemetry_rc.u.err != NULL) {
            gm_error_free(telemetry_rc.u.err);
        }
        memset(&tcfg, 0, sizeof(tcfg));
        tcfg.metrics_enabled = false;
        tcfg.log_format = GM_LOG_FMT_TEXT;
        if (err_snprintf < 0 || (size_t)err_snprintf >= sizeof(err_msg)) {
            (void)gm_logger_log(&ctx->logger_port, GM_LOG_ERROR, "cache",
                                "telemetry_cfg_load_failed");
        } else {
            (void)gm_logger_log(&ctx->logger_port, GM_LOG_ERROR, "cache",
                                err_msg);
        }
    }
    const char *mode = "full"; /* incremental mode to be detected once available */
    char tags[256];
    tags[0] = '\0';
    gm_repo_id_t repo_id = {0};
    char repo_path[GM_PATH_MAX];
    const char *repo_canon_view = NULL;
    do {
        int rp = unwrap_result(gm_git_repository_port_repository_path(
            &ctx->git_repo_port, GM_GIT_REPOSITORY_PATH_GITDIR, repo_path,
            sizeof(repo_path)));
        if (rp != GM_OK) break;
        gm_fs_canon_opts_t copts = {.mode = GM_FS_CANON_PHYSICAL_EXISTING};
        if (unwrap_result(gm_fs_temp_port_canonicalize_ex(
                &ctx->fs_temp_port, repo_path, copts, &repo_canon_view)) != GM_OK) {
            repo_canon_view = NULL;
        }
        (void)compute_repo_id(ctx, &repo_id);
    } while (0);
    gm_telemetry_tag_context_t tag_ctx = {
        .branch = branch,
        .mode = mode,
        .repo_canon_path = repo_canon_view,
        .repo_id = &repo_id,
    };
    gm_result_void_t tags_rc =
        gm_telemetry_build_tags(&tcfg, &tag_ctx, tags, sizeof(tags));
    if (!tags_rc.ok) {
        if (tags_rc.u.err != NULL) {
            gm_error_free(tags_rc.u.err);
        }
        tags[0] = '\0';
        (void)gm_logger_log(&ctx->logger_port, GM_LOG_WARN, "cache",
                            "telemetry_tags_build_failed");
    }
    if (tcfg.extras_dropped) {
        (void)gm_logger_log(&ctx->logger_port, GM_LOG_WARN, "cache",
                            "telemetry extras dropped=1");
    }

    /* Log start */
    {
        char msg[256];
        msg[0] = '\0';
        const gm_log_kv_t kvs[] = {
            {.key = "event", .value = "rebuild_start"},
            {.key = "branch", .value = branch},
            {.key = "mode", .value = mode},
        };
        gm_log_formatter_fn fmt = ctx->log_formatter ? ctx->log_formatter
                                                     : gm_log_format_render_default;
        gm_result_void_t fmt_rc = fmt(kvs, sizeof(kvs) / sizeof(kvs[0]),
                                      (tcfg.log_format == GM_LOG_FMT_JSON),
                                      msg, sizeof(msg));
        if (!fmt_rc.ok) {
            if (fmt_rc.u.err != NULL) gm_error_free(fmt_rc.u.err);
            int alt = gm_snprintf(msg, sizeof(msg),
                                  "event=rebuild_start branch=%s mode=%s",
                                  branch, mode);
            if (alt < 0 || (size_t)alt >= sizeof(msg)) {
                msg[0] = '\0';
            }
        }
        if (msg[0] == '\0') {
            (void)gm_logger_log(&ctx->logger_port, GM_LOG_INFO, "cache",
                                "rebuild_start");
        } else {
            (void)gm_logger_log(&ctx->logger_port, GM_LOG_INFO, "cache", msg);
        }
    }

    result_code = cache_prepare_rebuild(ctx, branch, force_full, &old_meta,
                                        &has_old_cache, &temp_dir);
    if (result_code != GM_OK) {
        cache_diag_emit(ctx, "rebuild_prep_failed", branch, result_code);
        return result_code;
    }

    /* Stabilize temp dir path because fs port may reuse internal buffers. */
    if (temp_dir.path != NULL) {
        if (gm_strcpy_safe(stable_temp_path, sizeof(stable_temp_path),
                           temp_dir.path) == GM_OK) {
            temp_dir.path = stable_temp_path;
        }
    }

    result_code = cache_build_edge_map(&forward_map, &reverse_map);
    if (result_code != GM_OK) {
        cache_diag_emit(ctx, "rebuild_edge_map_failed", branch, result_code);
        cache_cleanup(ctx, forward_map, reverse_map, &temp_dir);
        return result_code;
    }

    start_time = clock();
    uint32_t total_edges = 0;

    result_code = cache_collect_and_write(ctx, branch, forward_map, reverse_map,
                                          has_old_cache, &old_meta, &temp_dir,
                                          &total_edges);
    if (result_code != GM_OK) {
        cache_diag_emit(ctx, "rebuild_collect_write_failed", branch, result_code);
        goto cleanup;
    }

    cache_meta_inputs_t meta_inputs = {
        .total_edges = total_edges,
        .start_time = start_time,
    };
    result_code =
        cache_populate_meta(&ctx->git_repo_port, branch, &meta_inputs, &meta);
    if (result_code != GM_OK) {
        cache_diag_emit(ctx, "rebuild_meta_failed", branch, result_code);
        goto cleanup;
    }

    cache_commit_inputs_t commit_inputs = {
        .branch = branch,
        .temp_dir = &temp_dir,
    };
    gm_oid_t final_commit = {0};
    result_code = cache_build_commit_and_update(&ctx->git_repo_port,
                                                &commit_inputs, &meta,
                                                &final_commit);
    if (result_code == GM_OK && tcfg.metrics_enabled) {
        /* diagnostic success can be added later if needed */
        /* duration */
        (void)gm_metrics_timing_ms(&ctx->metrics_port,
                                   "cache.rebuild.duration_ms",
                                   meta.build_time_ms, tags);
        /* edges */
        (void)gm_metrics_counter_add(&ctx->metrics_port,
                                     "cache.edges_processed_total",
                                     (uint64_t)meta.edge_count, tags);
        /* tree size */
        if (!gm_oid_is_zero(&final_commit)) {
            uint64_t tree_size = 0;
            gm_result_void_t ts = gm_git_repository_port_commit_tree_size(
                &ctx->git_repo_port, &final_commit, &tree_size);
            if (ts.ok) {
                (void)gm_metrics_gauge_set(&ctx->metrics_port,
                                           "cache.tree_size_bytes",
                                           (double)tree_size, tags);
            } else if (ts.u.err != NULL) {
                gm_error_free(ts.u.err);
            }
        }
    }

    /* Log success */
    if (result_code == GM_OK) {
        char msg[256];
        msg[0] = '\0';
        char edge_count_buf[32];
        char dur_buf[32];
        int edge_rc = gm_snprintf(edge_count_buf, sizeof(edge_count_buf), "%u",
                                  (unsigned)meta.edge_count);
        if (edge_rc < 0 || (size_t)edge_rc >= sizeof(edge_count_buf)) {
            result_code = GM_ERR_BUFFER_TOO_SMALL;
            goto cleanup;
        }
        int dur_rc = gm_snprintf(dur_buf, sizeof(dur_buf), "%llu",
                                 (unsigned long long)meta.build_time_ms);
        if (dur_rc < 0 || (size_t)dur_rc >= sizeof(dur_buf)) {
            result_code = GM_ERR_BUFFER_TOO_SMALL;
            goto cleanup;
        }
        const gm_log_kv_t kvs[] = {
            {.key = "event", .value = "rebuild_ok"},
            {.key = "branch", .value = branch},
            {.key = "mode", .value = mode},
            {.key = "edge_count", .value = edge_count_buf},
            {.key = "duration_ms", .value = dur_buf},
        };
        gm_log_formatter_fn fmt = ctx->log_formatter ? ctx->log_formatter
                                                     : gm_log_format_render_default;
        gm_result_void_t fmt_rc = fmt(kvs, sizeof(kvs) / sizeof(kvs[0]),
                                      (tcfg.log_format == GM_LOG_FMT_JSON),
                                      msg, sizeof(msg));
        if (!fmt_rc.ok) {
            if (fmt_rc.u.err != NULL) gm_error_free(fmt_rc.u.err);
            int alt = gm_snprintf(msg, sizeof(msg),
                                  "event=rebuild_ok branch=%s mode=%s", branch,
                                  mode);
            if (alt < 0 || (size_t)alt >= sizeof(msg)) {
                msg[0] = '\0';
            }
        }
        if (msg[0] == '\0') {
            (void)gm_logger_log(&ctx->logger_port, GM_LOG_INFO, "cache",
                                "rebuild_ok");
        } else {
            (void)gm_logger_log(&ctx->logger_port, GM_LOG_INFO, "cache", msg);
        }
    }

cleanup:
    cache_cleanup(ctx, forward_map, reverse_map, &temp_dir);
    if (result_code != GM_OK) {
        char msg[256];
        msg[0] = '\0';
        char code_buf[16];
        int code_rc = gm_snprintf(code_buf, sizeof(code_buf), "%d",
                                  result_code);
        if (code_rc < 0 || (size_t)code_rc >= sizeof(code_buf)) {
            (void)gm_logger_log(&ctx->logger_port, GM_LOG_ERROR, "cache",
                                "rebuild_failed format_code_overflow");
            code_buf[0] = '\0';
        }
        const gm_log_kv_t kvs[] = {
            {.key = "event", .value = "rebuild_failed"},
            {.key = "branch", .value = branch},
            {.key = "mode", .value = mode},
            {.key = "code", .value = code_buf},
        };
        gm_log_formatter_fn fmt = ctx->log_formatter ? ctx->log_formatter
                                                     : gm_log_format_render_default;
        gm_result_void_t fmt_rc = fmt(kvs, sizeof(kvs) / sizeof(kvs[0]),
                                      (tcfg.log_format == GM_LOG_FMT_JSON),
                                      msg, sizeof(msg));
        if (!fmt_rc.ok) {
            if (fmt_rc.u.err != NULL) gm_error_free(fmt_rc.u.err);
            int alt = gm_snprintf(msg, sizeof(msg),
                                  "event=rebuild_failed branch=%s mode=%s", branch,
                                  mode);
            if (alt < 0 || (size_t)alt >= sizeof(msg)) {
                msg[0] = '\0';
            }
        }
        if (msg[0] == '\0') {
            (void)gm_logger_log(&ctx->logger_port, GM_LOG_ERROR, "cache",
                                "rebuild_failed");
        } else {
            (void)gm_logger_log(&ctx->logger_port, GM_LOG_ERROR, "cache", msg);
        }
        cache_diag_emit(ctx, "rebuild_failed", branch, result_code);
    }
    return result_code;
}
