/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/journal.h"
#include "gitmind/types.h"
#include "gitmind/context.h"
#include "gitmind/ports/git_repository_port.h"
#include "gitmind/cbor/constants_cbor.h"
#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/edge.h"
#include "gitmind/edge_attributed.h"
#include "gitmind/security/memory.h"
#include "gitmind/util/oid.h"
#include <sodium/utils.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include "gitmind/security/string.h" /* gm_snprintf */

/* Local constants */
#include "gitmind/constants_internal.h"
#include "gitmind/util/ref.h"
#include "gitmind/cache/cache.h" /* GM_CACHE_BRANCH_NAME_SIZE */
#include "gitmind/ports/fs_temp_port.h"
#include "gitmind/ports/logger_port.h"
#include "gitmind/ports/metrics_port.h"
#include "gitmind/ports/env_port.h"
#include "gitmind/telemetry/internal/config.h"
#include "gitmind/telemetry/internal/log_format.h"
#include "gitmind/ports/diagnostic_port.h"
#include "gitmind/journal/internal/codec.h"
#include "gitmind/journal/internal/append_plan.h"
#define MAX_CBOR_SIZE CBOR_MAX_STRING_LENGTH
_Static_assert(CLOCKS_PER_SEC >= MILLIS_PER_SECOND,
               "CLOCKS_PER_SEC must be >= 1000");

static uint64_t monotonic_ms_now(void) {
#if defined(_WIN32)
    LARGE_INTEGER freq, counter;
    if (QueryPerformanceFrequency(&freq) && QueryPerformanceCounter(&counter)) {
        return (uint64_t)((counter.QuadPart * 1000ULL) /
                          (uint64_t)freq.QuadPart);
    }
#else
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
        return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)(ts.tv_nsec / 1000000ULL);
    }
#endif
    return (uint64_t)((clock() * 1000ULL) / CLOCKS_PER_SEC);
}
#define CLOCKS_PER_MS                                                       \
    ((clock_t)((CLOCKS_PER_SEC + (MILLIS_PER_SECOND - 1)) / MILLIS_PER_SECOND))
#define COMMIT_ENCODING "UTF-8"
#define CBOR_OVERFLOW_MARGIN GM_FORMAT_BUFFER_SIZE /* CBOR encoding safety margin */
#define PARENT_COMMITS_MAX 1

/* Journal writer context */
typedef struct {
    gm_git_repository_port_t *repo_port;
    gm_oid_t empty_tree_oid;
    char ref_name[REF_NAME_BUFFER_SIZE];
    gm_context_t *app_ctx; /* for diagnostics */
} journal_ctx_t;

typedef struct {
    const void *data;
    size_t count;
    size_t stride;
} journal_edge_batch_t;

/* Initialize journal context */
static int journal_init(journal_ctx_t *jctx, gm_context_t *ctx,
                        const char *branch) {
    if (ctx->git_repo_port.vtbl == NULL) {
        return GM_ERR_INVALID_STATE;
    }
    jctx->repo_port = &ctx->git_repo_port;
    jctx->app_ctx = ctx;

    /* Parse empty tree OID */
    if (gm_oid_from_hex(&jctx->empty_tree_oid, EMPTY_TREE_SHA) != GM_OK) {
        return GM_ERR_UNKNOWN;
    }

    /* Build ref name */
    int ref_rc = gm_build_ref(jctx->ref_name, sizeof(jctx->ref_name),
                              GITMIND_EDGES_REF_PREFIX, branch);
    if (ref_rc != GM_OK) {
        return ref_rc;
    }

    return GM_OK;
}

/* Get current branch name */
static int get_current_branch(gm_git_repository_port_t *port, char *branch_name,
                              size_t len) {
    gm_result_void_t result =
        gm_git_repository_port_head_branch(port, branch_name, len);
    if (!result.ok) {
        if (result.u.err != NULL) {
            gm_error_free(result.u.err);
        }
        return GM_ERR_UNKNOWN;
    }
    return GM_OK;
}

static int encode_cbor_message(const uint8_t *cbor_data, size_t cbor_len,
                               char **message_out, size_t *message_len_out) {
    gm_result_void_t r = gm_journal_encode_message(cbor_data, cbor_len,
                                                   message_out, message_len_out);
    if (r.ok) return GM_OK;
    int code = GM_ERR_UNKNOWN;
    if (r.u.err) { code = r.u.err->code; gm_error_free(r.u.err); }
    return code;
}

typedef struct {
    gm_git_reference_tip_t tip;
    bool found;
} parent_lookup_ctx_t;

static int collect_parent_tip(const gm_oid_t *commit_oid, void *userdata) {
    parent_lookup_ctx_t *ctx = (parent_lookup_ctx_t *)userdata;
    if (ctx == NULL || commit_oid == NULL) {
        return GM_ERR_INVALID_ARGUMENT;
    }
    if (!ctx->found) {
        ctx->found = true;
        ctx->tip.has_target = true;
        ctx->tip.oid = *commit_oid;
        if (gm_oid_to_hex(commit_oid, ctx->tip.oid_hex,
                          sizeof(ctx->tip.oid_hex)) != GM_OK) {
            gm_memset_safe(ctx->tip.oid_hex, sizeof(ctx->tip.oid_hex), 0,
                           sizeof(ctx->tip.oid_hex));
        }
    }
    return GM_OK;
}

/* Create journal commit */
static int create_journal_commit(journal_ctx_t *jctx, const uint8_t *cbor_data,
                                 size_t cbor_len, gm_oid_t *commit_oid) {
    parent_lookup_ctx_t lookup_ctx;
    gm_memset_safe(&lookup_ctx, sizeof(lookup_ctx), 0, sizeof(lookup_ctx));
    gm_result_void_t walk_result = gm_git_repository_port_walk_commits(
        jctx->repo_port, jctx->ref_name, collect_parent_tip, &lookup_ctx);
    if (!walk_result.ok) {
        int walk_code = GM_ERR_UNKNOWN;
        if (walk_result.u.err != NULL) {
            walk_code = walk_result.u.err->code;
            gm_error_free(walk_result.u.err);
        }
        /* Treat missing ref as empty history (no parent). */
        if (walk_code != GM_ERR_NOT_FOUND) {
            return walk_code;
        }
    }

    char *message = NULL;
    int message_rc = encode_cbor_message(cbor_data, cbor_len, &message, NULL);
    if (message_rc != GM_OK) {
        return message_rc;
    }

    const gm_oid_t *parent_list = lookup_ctx.found ? &lookup_ctx.tip.oid : NULL;

    gm_journal_commit_plan_t plan = {0};
    gm_result_void_t pr = gm_journal_build_commit_plan(&jctx->empty_tree_oid,
                                                       parent_list, message,
                                                       &plan);
    if (!pr.ok) { int code = pr.u.err ? pr.u.err->code : GM_ERR_UNKNOWN; if (pr.u.err) gm_error_free(pr.u.err); free(message); return code; }

    gm_git_commit_spec_t spec = {
        .tree_oid = plan.tree_oid,
        .message = plan.message,
        .parents = plan.parents,
        .parent_count = (unsigned)plan.parent_count,
    };

    gm_result_void_t commit_result =
        gm_git_repository_port_commit_create(jctx->repo_port, &spec, commit_oid);
    free(message);
    if (!commit_result.ok) {
        int commit_code = GM_ERR_UNKNOWN;
        if (commit_result.u.err != NULL) {
            commit_code = commit_result.u.err->code;
            gm_error_free(commit_result.u.err);
        }
        return commit_code;
    }

    return GM_OK;
}

/* Generic journal append function */
typedef int (*edge_encoder_fn)(const void *edge, uint8_t *buffer, size_t *len);

static journal_edge_batch_t make_edge_batch(const void *edges,
                                            size_t count,
                                            size_t stride) {
    journal_edge_batch_t batch = {edges, count, stride};
    return batch;
}

static int resolve_branch_name(gm_context_t *ctx, char *branch,
                               size_t branch_len) {
    return get_current_branch(&ctx->git_repo_port, branch, branch_len);
}

static int flush_journal_batch(journal_ctx_t *jctx, const uint8_t *buffer,
                               size_t length) {
    for (int attempt = 0; attempt < 2; ++attempt) {
        gm_oid_t commit_oid;
        int commit_rc = create_journal_commit(jctx, buffer, length, &commit_oid);
        if (commit_rc != GM_OK) {
            if (jctx->app_ctx) {
                char code_buf[16]; (void)gm_snprintf(code_buf, sizeof code_buf, "%d", commit_rc);
                const gm_diag_kv_t kvs[] = {{.key="code", .value=code_buf}};
                (void)gm_diag_emit(&jctx->app_ctx->diag_port, "journal",
                                   "journal_commit_create_failed", kvs, 1);
            }
            return commit_rc;
        }

        gm_git_reference_update_spec_t spec = {
            .ref_name = jctx->ref_name,
            .target_oid = &commit_oid,
            .log_message = "Journal append",
            .force = false,
        };
        gm_result_void_t update_result = gm_git_repository_port_reference_update(
            jctx->repo_port, &spec);
        if (update_result.ok) {
            return GM_OK;
        }

        int update_code = GM_ERR_UNKNOWN;
        if (update_result.u.err != NULL) {
            update_code = update_result.u.err->code;
            gm_error_free(update_result.u.err);
        }

        if (update_code == GM_ERR_ALREADY_EXISTS && attempt == 0) {
            if (jctx->app_ctx) {
                const gm_diag_kv_t kvs[] = {{.key="ref", .value=jctx->ref_name}};
                (void)gm_diag_emit(&jctx->app_ctx->diag_port, "journal",
                                   "journal_nff_retry", kvs, 1);
            }
            /* Reference moved forward; recompute parents and try once more. */
            continue;
        }
        if (jctx->app_ctx) {
            char code_buf[16]; (void)gm_snprintf(code_buf, sizeof code_buf, "%d", update_code);
            const gm_diag_kv_t kvs[] = {
                {.key="ref", .value=jctx->ref_name},
                {.key="code", .value=code_buf},
            };
            (void)gm_diag_emit(&jctx->app_ctx->diag_port, "journal",
                               "journal_ref_update_failed", kvs, 2);
        }
        return update_code;
    }

    return GM_ERR_UNKNOWN;
}

static bool should_flush_buffer(size_t offset) {
    return offset > MAX_CBOR_SIZE - CBOR_OVERFLOW_MARGIN;
}

static int encode_edge_into_buffer(const journal_edge_batch_t *batch,
                                   size_t index, uint8_t *buffer,
                                   size_t *offset, edge_encoder_fn encoder) {
    const uint8_t *edge_bytes = (const uint8_t *)batch->data;
    const void *edge = edge_bytes + (index * batch->stride);
    size_t remaining = MAX_CBOR_SIZE - *offset;
    size_t encoded_size = remaining;
    int encode_rc = encoder(edge, buffer + *offset, &encoded_size);
    if (encode_rc != GM_OK) {
        return encode_rc;
    }

    *offset += encoded_size;
    return GM_OK;
}

static int encode_edges_to_journal(journal_ctx_t *jctx,
                                   const journal_edge_batch_t *batch,
                                   uint8_t *buffer,
                                   edge_encoder_fn encoder) {
    size_t offset = 0U;

    for (size_t index = 0; index < batch->count; ++index) {
        int encode_status =
            encode_edge_into_buffer(batch, index, buffer, &offset, encoder);
        if (encode_status != GM_OK) {
            return encode_status;
        }

        if (should_flush_buffer(offset)) {
            if (flush_journal_batch(jctx, buffer, offset) != GM_OK) {
                return GM_ERR_UNKNOWN;
            }
            offset = 0U;
        }
    }

    if (offset > 0U) {
        if (flush_journal_batch(jctx, buffer, offset) != GM_OK) {
            return GM_ERR_UNKNOWN;
        }
    }

    return GM_OK;
}

static int journal_append_generic(gm_context_t *ctx, journal_edge_batch_t batch,
                                  edge_encoder_fn encoder) {
    journal_ctx_t jctx;
    char branch[GM_PATH_MAX];
    /* Telemetry setup */
    gm_telemetry_cfg_t tcfg = {0};
    (void)gm_telemetry_cfg_load(&tcfg, gm_env_port_system());
    const bool json = (tcfg.log_format == GM_LOG_FMT_JSON);
    const char *mode = "append";
    char tags[256]; tags[0] = '\0';
    char repo_path[GM_PATH_MAX];
    char repo_canon_buf[GM_PATH_MAX];
    repo_canon_buf[0] = '\0';
    gm_repo_id_t repo_id = {0};
    /* Build tags: branch, mode, repo(optional), extras */
    do {
        if (ctx && ctx->git_repo_port.vtbl && ctx->fs_temp_port.vtbl) {
            if (gm_git_repository_port_repository_path(&ctx->git_repo_port,
                    GM_GIT_REPOSITORY_PATH_GITDIR, repo_path, sizeof(repo_path)).ok) {
                gm_fs_canon_opts_t copts = {.mode = GM_FS_CANON_PHYSICAL_EXISTING};
                const char *canon_tmp = NULL;
                if (gm_fs_temp_port_canonicalize_ex(&ctx->fs_temp_port, repo_path,
                        copts, &canon_tmp).ok && canon_tmp != NULL) {
                    if (gm_strcpy_safe(repo_canon_buf, sizeof(repo_canon_buf),
                                       canon_tmp) == GM_OK) {
                        (void)gm_repo_id_from_path(repo_canon_buf, &repo_id);
                    }
                    free((void *)canon_tmp);
                }
            }
        }
    } while (0);
    const char *repo_canon = (repo_canon_buf[0] != '\0') ? repo_canon_buf : NULL;

    if (ctx == NULL || batch.data == NULL || batch.count == 0U || encoder == NULL) {
        return GM_ERR_INVALID_ARGUMENT;
    }

    if (resolve_branch_name(ctx, branch, sizeof(branch)) != GM_OK) {
        return GM_ERR_UNKNOWN;
    }

    if (journal_init(&jctx, ctx, branch) != GM_OK) {
        return GM_ERR_UNKNOWN;
    }

    /* Emit start log */
    {
        char msg[256];
        const gm_log_kv_t kvs[] = {
            {.key = "event", .value = "journal_append_start"},
            {.key = "branch", .value = branch},
            {.key = "mode", .value = mode},
        };
        gm_log_formatter_fn fmt = ctx && ctx->log_formatter ? ctx->log_formatter
                                                            : gm_log_format_render_default;
        (void)fmt(kvs, sizeof(kvs)/sizeof(kvs[0]), json, msg, sizeof(msg));
        (void)gm_logger_log(&ctx->logger_port, GM_LOG_INFO, "journal", msg);
    }

    uint8_t *cbor_buffer = malloc(MAX_CBOR_SIZE);
    if (cbor_buffer == NULL) {
        return GM_ERR_OUT_OF_MEMORY;
    }
    uint64_t start_time = monotonic_ms_now();
    int encode_result = encode_edges_to_journal(&jctx, &batch, cbor_buffer, encoder);
    free(cbor_buffer);

    /* Emit metrics + end log */
    uint64_t dur_ms = monotonic_ms_now() - start_time;
    (void)gm_telemetry_build_tags(&tcfg, branch, mode, repo_canon, &repo_id,
                                  tags, sizeof(tags));
    if (tcfg.metrics_enabled) {
        (void)gm_metrics_timing_ms(&ctx->metrics_port,
                                   "journal.append.duration_ms",
                                   dur_ms, tags);
        (void)gm_metrics_counter_add(&ctx->metrics_port,
                                     "journal.append.edges_total",
                                     (uint64_t)batch.count, tags);
    }
    {
        char msg[256]; char count_buf[32]; char dur_buf[32];
        (void)gm_snprintf(count_buf, sizeof(count_buf), "%llu",
                          (unsigned long long)batch.count);
        (void)gm_snprintf(dur_buf, sizeof(dur_buf), "%llu",
                          (unsigned long long)dur_ms);
        const gm_log_kv_t kvs[] = {
            {.key = "event", .value = (encode_result == GM_OK) ? "journal_append_ok" : "journal_append_failed"},
            {.key = "branch", .value = branch},
            {.key = "mode", .value = mode},
            {.key = "edges", .value = count_buf},
            {.key = "duration_ms", .value = dur_buf},
        };
        gm_log_formatter_fn fmt = ctx && ctx->log_formatter ? ctx->log_formatter
                                                            : gm_log_format_render_default;
        (void)fmt(kvs, sizeof(kvs)/sizeof(kvs[0]), json, msg, sizeof(msg));
        (void)gm_logger_log(&ctx->logger_port,
                            (encode_result == GM_OK) ? GM_LOG_INFO : GM_LOG_ERROR,
                            "journal", msg);
    }
    return encode_result;
}

/* Wrapper for regular edge encoder */
static int edge_encoder_wrapper(const void *edge, uint8_t *buffer,
                                size_t *len) {
    const gm_edge_t *edge_value = (const gm_edge_t *)edge;
    if (edge_value == NULL || buffer == NULL || len == NULL) {
        return GM_ERR_INVALID_ARGUMENT;
    }

    gm_result_void_t result = gm_edge_encode_cbor(edge_value, buffer, len);
    if (result.ok) {
        return GM_OK;
    }
    return GM_ERR_INVALID_FORMAT;
}

/* Wrapper for attributed edge encoder */
static int edge_attributed_encoder_wrapper(const void *edge, uint8_t *buffer,
                                           size_t *len) {
    const gm_edge_attributed_t *edge_value =
        (const gm_edge_attributed_t *)edge;
    if (edge_value == NULL || buffer == NULL || len == NULL) {
        return GM_ERR_INVALID_ARGUMENT;
    }

    gm_result_void_t result =
        gm_edge_attributed_encode_cbor(edge_value, buffer, len);
    if (result.ok) {
        return GM_OK;
    }
    return GM_ERR_INVALID_FORMAT;
}

/* Append edges to journal */
int gm_journal_append(gm_context_t *ctx, const gm_edge_t *edges,
                      size_t count) {
    journal_edge_batch_t batch = make_edge_batch(edges, count, sizeof(gm_edge_t));
    return journal_append_generic(ctx, batch, edge_encoder_wrapper);
}

/* Append attributed edges to journal */
int gm_journal_append_attributed(gm_context_t *ctx,
                                 const gm_edge_attributed_t *edges,
                                 size_t count) {
    journal_edge_batch_t batch =
        make_edge_batch(edges, count, sizeof(gm_edge_attributed_t));
    return journal_append_generic(ctx, batch, edge_attributed_encoder_wrapper);
}

/* Public wrapper for hooks to create commits */
int gm_journal_create_commit(gm_context_t *ctx, const char *ref,
                          const void *data, size_t len) {
    journal_ctx_t jctx;
    gm_oid_t commit_oid;

    /* Initialize context */
    jctx.repo_port = &ctx->git_repo_port;

    /* Parse empty tree OID */
    if (gm_oid_from_hex(&jctx.empty_tree_oid, EMPTY_TREE_SHA) != GM_OK) {
        return GM_ERR_UNKNOWN;
    }

    /* Copy ref name securely */
    size_t ref_len = strlen(ref);
    if (ref_len >= sizeof(jctx.ref_name)) {
        ref_len = sizeof(jctx.ref_name) - 1U;
    }
    gm_memcpy_safe(jctx.ref_name, sizeof(jctx.ref_name), ref, ref_len);
    jctx.ref_name[ref_len] = '\0';

    /* Create commit */
    int commit_rc = create_journal_commit(&jctx, data, len, &commit_oid);
    if (commit_rc != GM_OK) {
        return commit_rc;
    }

    /* Update the provided ref to point at the new commit */
    gm_git_reference_update_spec_t spec = {
        .ref_name = jctx.ref_name,
        .target_oid = &commit_oid,
        .log_message = "Journal commit",
        .force = false,
    };
    gm_result_void_t upd =
        gm_git_repository_port_reference_update(jctx.repo_port, &spec);
    if (!upd.ok) {
        int code = GM_ERR_UNKNOWN;
        if (upd.u.err) {
            code = upd.u.err->code;
            gm_error_free(upd.u.err);
        }
        return code;
    }
    return GM_OK;
}
