/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/journal.h"
#include "gitmind/ports/git_repository_port.h"
#include "gitmind/result.h"
#include "gitmind/types.h"
#include "gitmind/context.h"
#include "gitmind/cbor/constants_cbor.h"
#include "gitmind/constants_internal.h"
#include "gitmind/error.h"
#include "gitmind/edge.h"
#include "gitmind/edge_attributed.h"
#include "gitmind/attribution.h"
#include "gitmind/security/memory.h"
#include "gitmind/security/string.h"
#include "gitmind/util/memory.h"
#include "gitmind/util/ref.h"

#include <sodium/utils.h>

#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

/* Telemetry and logging */
#include "gitmind/ports/logger_port.h"
#include "gitmind/ports/metrics_port.h"
#include "gitmind/ports/fs_temp_port.h"
#include "gitmind/ports/env_port.h"
#include "gitmind/telemetry/internal/config.h"
#include "gitmind/telemetry/internal/log_format.h"
#include "gitmind/ports/diagnostic_port.h"
#include "gitmind/journal/internal/codec.h"
#include "gitmind/journal/internal/read_decoder.h"
#define CLOCKS_PER_MS                                                       \
    ((clock_t)((CLOCKS_PER_SEC + (MILLIS_PER_SECOND - 1)) / MILLIS_PER_SECOND))

/* Constants */
#define MAX_CBOR_SIZE CBOR_MAX_STRING_LENGTH
#define CURRENT_BRANCH_BUFFER_SIZE GM_PATH_MAX
#define CBOR_DEBUG_BUFFER_SIZE 256

/* Debug flag for CBOR decoding (set GITMIND_CBOR_DEBUG=1) */
static int g_cbor_debug = -1;

static bool cbor_debug_enabled(void) {
    if (g_cbor_debug == -1) {
        const char *value = getenv("GITMIND_CBOR_DEBUG");
        g_cbor_debug = (value != NULL &&
                        (value[0] == '1' || value[0] == 't' || value[0] == 'T' ||
                         value[0] == 'y' || value[0] == 'Y'))
                           ? 1
                           : 0;
    }
    return g_cbor_debug == 1;
}

static void cbor_debug_log_two_values(const char *message, size_t first,
                                      size_t second) {
    if (!cbor_debug_enabled()) {
        return;
    }

    char buffer[CBOR_DEBUG_BUFFER_SIZE];
    (void)gm_snprintf(buffer, sizeof(buffer), message, first, second);
    (void)fputs(buffer, stderr);
}

typedef int (*edge_callback_fn)(const gm_edge_t *, void *);
typedef int (*edge_attr_callback_fn)(const gm_edge_attributed_t *, void *);

/* Generic reader context */
typedef struct {
    gm_context_t *gm_ctx;
    gm_git_repository_port_t *repo_port;
    edge_callback_fn edge_callback;
    edge_attr_callback_fn edge_attr_callback;
    void *userdata;
    bool is_attributed;
    size_t *edge_count_ptr; /* optional counter for metrics */
} reader_ctx_t;

/* Convert legacy edge to attributed edge */
/* Decode selection moved to domain/journal/read_decoder */

/* Process attributed edge from CBOR */
/* Process edge using domain decode helper */
static int process_edge_decoded(const uint8_t *cbor_data, size_t remaining,
                                reader_ctx_t *rctx, size_t *consumed) {
    gm_edge_t basic = {0};
    gm_edge_attributed_t attr = {0};
    bool got_attr = false;
    gm_result_void_t r = gm_journal_decode_edge(cbor_data, remaining,
                                                rctx->is_attributed,
                                                &basic, &attr, consumed,
                                                &got_attr);
    if (!r.ok) {
        if (r.u.err) gm_error_free(r.u.err);
        return GM_ERR_INVALID_FORMAT;
    }
    if (got_attr) {
        if (rctx->edge_attr_callback == NULL) return GM_ERR_INVALID_ARGUMENT;
        if (rctx->edge_count_ptr) (*rctx->edge_count_ptr)++;
        return rctx->edge_attr_callback(&attr, rctx->userdata);
    }
    if (rctx->edge_callback == NULL) return GM_ERR_INVALID_ARGUMENT;
    if (rctx->edge_count_ptr) (*rctx->edge_count_ptr)++;
    return rctx->edge_callback(&basic, rctx->userdata);
}

/* Process a single commit - generic version */
/* moved to domain/journal/codec */

static int resolve_branch(gm_git_repository_port_t *port,
                          const char *requested_branch, char *buffer,
                          size_t buffer_len, const char **resolved_branch) {
    if (requested_branch != NULL) {
        *resolved_branch = requested_branch;
        return GM_OK;
    }

    gm_result_void_t head_result = gm_git_repository_port_head_branch(
        port, buffer, buffer_len);
    if (!head_result.ok) {
        int code = GM_ERR_INVALID_FORMAT;
        if (head_result.u.err != NULL) {
            code = head_result.u.err->code;
            gm_error_free(head_result.u.err);
        }
        return code;
    }

    *resolved_branch = buffer;
    return GM_OK;
}

static int process_commit_generic(const char *raw_message, reader_ctx_t *rctx) {
    uint8_t decoded[MAX_CBOR_SIZE];
    size_t message_len = 0;
    size_t cap = sizeof(decoded);
    gm_result_void_t dr = gm_journal_decode_message(raw_message, decoded, &cap);
    int decode_status = dr.ok ? GM_OK : (dr.u.err ? dr.u.err->code : GM_ERR_UNKNOWN);
    message_len = dr.ok ? cap : 0;
    if (!dr.ok && dr.u.err) gm_error_free(dr.u.err);
    if (decode_status != GM_OK) {
        return decode_status;
    }

    size_t offset = 0U;
    while (offset < message_len) {
        const size_t remaining = message_len - offset;
        size_t consumed = 0U;
        int edge_status = process_edge_decoded(decoded + offset, remaining,
                                               rctx, &consumed);

        if (edge_status == GM_ERR_INVALID_FORMAT) {
            cbor_debug_log_two_values(
                "[CBOR DEBUG] Invalid CBOR at commit decode offset=%zu remaining=%zu\n",
                offset, remaining);
            if (rctx->gm_ctx) {
                char offbuf[24]; char rembuf[24];
                (void)gm_snprintf(offbuf, sizeof offbuf, "%zu", offset);
                (void)gm_snprintf(rembuf, sizeof rembuf, "%zu", remaining);
                const gm_diag_kv_t kvs[] = {
                    {.key="offset", .value=offbuf},
                    {.key="remaining", .value=rembuf},
                };
                (void)gm_diag_emit(&rctx->gm_ctx->diag_port, "journal",
                                   "journal_cbor_invalid", kvs, 2);
            }
            break;
        }

        if (edge_status != GM_OK) {
            return edge_status;
        }

        cbor_debug_log_two_values(
            "[CBOR DEBUG] Decoded an edge (consumed=%zu) at offset=%zu\n",
            consumed, offset);

        offset += consumed;
    }

    return GM_OK;
}

/* Walk journal commits - generic version */
static int walk_commit_callback(const gm_oid_t *commit_oid, void *userdata) {
    reader_ctx_t *rctx = (reader_ctx_t *)userdata;
    if (rctx == NULL || commit_oid == NULL) {
        return GM_ERR_INVALID_ARGUMENT;
    }

    char *message = NULL;
    gm_result_void_t message_result = gm_git_repository_port_commit_read_message(
        rctx->repo_port, commit_oid, &message);
    if (!message_result.ok) {
        int code = GM_ERR_INVALID_FORMAT;
        if (message_result.u.err != NULL) {
            code = message_result.u.err->code;
            gm_error_free(message_result.u.err);
        }
        if (rctx->gm_ctx) {
            char cbuf[16]; (void)gm_snprintf(cbuf, sizeof cbuf, "%d", code);
            const gm_diag_kv_t kvs[] = {{.key="code", .value=cbuf}};
            (void)gm_diag_emit(&rctx->gm_ctx->diag_port, "journal",
                               "journal_read_message_failed", kvs, 1);
        }
        return code;
    }

    int process_status = process_commit_generic(message, rctx);
    gm_git_repository_port_commit_message_dispose(rctx->repo_port, message);
    return process_status;
}

static int walk_journal_generic(reader_ctx_t *rctx, const char *ref_name) {
    gm_result_void_t walk_result = gm_git_repository_port_walk_commits(
        rctx->repo_port, ref_name, walk_commit_callback, rctx);
    if (!walk_result.ok) {
        int code = GM_ERR_UNKNOWN;
        if (walk_result.u.err != NULL) {
            code = walk_result.u.err->code;
            gm_error_free(walk_result.u.err);
        }
        if (rctx->gm_ctx) {
            char cbuf[16]; (void)gm_snprintf(cbuf, sizeof cbuf, "%d", code);
            const gm_diag_kv_t kvs[] = {{.key="code", .value=cbuf}};
            (void)gm_diag_emit(&rctx->gm_ctx->diag_port, "journal",
                               "journal_walk_failed", kvs, 1);
        }
        return code;
    }
    return GM_OK;
}

/* Generic journal read function */
static int journal_read_generic(gm_context_t *ctx, const char *branch,
                                edge_callback_fn edge_cb,
                                edge_attr_callback_fn attr_cb,
                                bool is_attributed, void *userdata) {
    reader_ctx_t rctx;
    char ref_name[REF_NAME_BUFFER_SIZE];
    char current_branch[CURRENT_BRANCH_BUFFER_SIZE];
    /* Telemetry */
    gm_telemetry_cfg_t tcfg = {0};
    gm_result_void_t telemetry_rc =
        gm_telemetry_cfg_load(&tcfg, gm_env_port_system());
    if (!telemetry_rc.ok) {
        char err_msg[96];
        int32_t err_code = telemetry_rc.u.err ? telemetry_rc.u.err->code
                                              : GM_ERR_UNKNOWN;
        int err_len = gm_snprintf(err_msg, sizeof(err_msg),
                                  "journal telemetry cfg load failed code=%d",
                                  (int)err_code);
        if (telemetry_rc.u.err != NULL) {
            gm_error_free(telemetry_rc.u.err);
        }
        memset(&tcfg, 0, sizeof(tcfg));
        tcfg.metrics_enabled = false;
        tcfg.log_format = GM_LOG_FMT_TEXT;
        if (err_len < 0 || (size_t)err_len >= sizeof(err_msg)) {
            (void)gm_logger_log(&ctx->logger_port, GM_LOG_ERROR, "journal",
                                "telemetry_cfg_load_failed");
        } else {
            (void)gm_logger_log(&ctx->logger_port, GM_LOG_ERROR, "journal",
                                err_msg);
        }
    }
    const bool json = (tcfg.log_format == GM_LOG_FMT_JSON);
    const char *mode = is_attributed ? "read_attributed" : "read";
    char tags[256]; tags[0] = '\0';
    char repo_path[GM_PATH_MAX];
    char repo_canon_buf[GM_PATH_MAX];
    repo_canon_buf[0] = '\0';
    const char *repo_canon = NULL;
    gm_repo_id_t repo_id = {0};

    if (ctx == NULL) {
        return GM_ERR_INVALID_ARGUMENT;
    }
    if (ctx->git_repo_port.vtbl == NULL) {
        return GM_ERR_INVALID_STATE;
    }
    if (is_attributed) {
        if (attr_cb == NULL) {
            return GM_ERR_INVALID_ARGUMENT;
        }
    } else if (edge_cb == NULL) {
        return GM_ERR_INVALID_ARGUMENT;
    }

    /* Initialize reader context */
    rctx.gm_ctx = ctx;
    rctx.repo_port = &ctx->git_repo_port;
    rctx.edge_callback = edge_cb;
    rctx.edge_attr_callback = attr_cb;
    rctx.userdata = userdata;
    rctx.is_attributed = is_attributed;
    size_t edge_count = 0U;
    rctx.edge_count_ptr = &edge_count;

    const char *resolved_branch = branch;
    int branch_rc = resolve_branch(rctx.repo_port, branch, current_branch,
                                   sizeof(current_branch), &resolved_branch);
    if (branch_rc != GM_OK) {
        return branch_rc;
    }

    /* Build ref name */
    if (gm_build_ref(ref_name, sizeof(ref_name), GITMIND_EDGES_REF_PREFIX,
                     resolved_branch) != GM_OK) {
        return GM_ERR_BUFFER_TOO_SMALL;
    }

    /* Build tags now that branch is resolved */
    do {
        if (ctx && ctx->git_repo_port.vtbl && ctx->fs_temp_port.vtbl) {
            gm_result_void_t path_rc =
                gm_git_repository_port_repository_path(&ctx->git_repo_port,
                    GM_GIT_REPOSITORY_PATH_GITDIR, repo_path, sizeof(repo_path));
            if (path_rc.ok) {
                gm_fs_canon_opts_t copts = {.mode = GM_FS_CANON_PHYSICAL_EXISTING};
                const char *canon_tmp = NULL;
                gm_result_void_t canon_rc = gm_fs_temp_port_canonicalize_ex(
                    &ctx->fs_temp_port, repo_path, copts, &canon_tmp);
                if (canon_rc.ok && canon_tmp != NULL) {
                    if (gm_strcpy_safe(repo_canon_buf, sizeof(repo_canon_buf),
                                       canon_tmp) == GM_OK) {
                        repo_canon = repo_canon_buf;
                        gm_result_void_t repo_id_rc =
                            gm_repo_id_from_path(repo_canon, &repo_id);
                        if (!repo_id_rc.ok) {
                            memset(&repo_id, 0, sizeof(repo_id));
                            if (repo_id_rc.u.err != NULL) {
                                gm_error_free(repo_id_rc.u.err);
                            }
                        }
                    } else {
                        (void)gm_logger_log(&ctx->logger_port, GM_LOG_WARN,
                                            "journal",
                                            "repo_canon_truncated" );
                    }
                } else if (!canon_rc.ok) {
                    if (canon_rc.u.err != NULL) {
                        gm_error_free(canon_rc.u.err);
                    }
                }
            } else {
                if (path_rc.u.err != NULL) gm_error_free(path_rc.u.err);
            }
        }
    } while (0);
    gm_result_void_t tags_rc = gm_telemetry_build_tags(&tcfg, resolved_branch,
                                                       mode, repo_canon, &repo_id,
                                                       tags, sizeof(tags));
    if (!tags_rc.ok) {
        if (tags_rc.u.err != NULL) gm_error_free(tags_rc.u.err);
        tags[0] = '\0';
        (void)gm_logger_log(&ctx->logger_port, GM_LOG_WARN, "journal",
                            "telemetry_tags_build_failed");
    }
    /* Log start */
    {
        char msg[256];
        msg[0] = '\0';
        const gm_log_kv_t kvs[] = {
            {.key = "event", .value = "journal_read_start"},
            {.key = "branch", .value = resolved_branch},
            {.key = "mode", .value = mode},
        };
        gm_log_formatter_fn fmt = ctx && ctx->log_formatter ? ctx->log_formatter
                                                            : gm_log_format_render_default;
        gm_result_void_t fmt_rc = fmt(kvs, sizeof(kvs)/sizeof(kvs[0]), json, msg,
                                      sizeof(msg));
        if (!fmt_rc.ok) {
            if (fmt_rc.u.err != NULL) gm_error_free(fmt_rc.u.err);
            int alt = gm_snprintf(msg, sizeof(msg),
                                  "event=journal_read_start branch=%s mode=%s",
                                  resolved_branch, mode);
            if (alt < 0 || (size_t)alt >= sizeof(msg)) {
                msg[0] = '\0';
            }
        }
        if (msg[0] == '\0') {
            (void)gm_logger_log(&ctx->logger_port, GM_LOG_INFO, "journal",
                                "journal_read_start");
        } else {
            (void)gm_logger_log(&ctx->logger_port, GM_LOG_INFO, "journal", msg);
        }
    }

    uint64_t start_ms = monotonic_ms_now();
    int rc_walk = walk_journal_generic(&rctx, ref_name);

    uint64_t dur_ms = monotonic_ms_now() - start_ms;
    if (tcfg.metrics_enabled) {
        (void)gm_metrics_timing_ms(&ctx->metrics_port,
                                   "journal.read.duration_ms", dur_ms, tags);
        (void)gm_metrics_counter_add(&ctx->metrics_port,
                                     "journal.read.edges_total",
                                     (uint64_t)edge_count, tags);
    }
    {
        char msg[256];
        msg[0] = '\0';
        char dur_buf[32];
        int dur_fmt = gm_snprintf(dur_buf, sizeof(dur_buf), "%llu",
                                   (unsigned long long)dur_ms);
        if (dur_fmt < 0 || (size_t)dur_fmt >= sizeof(dur_buf)) {
            (void)gm_logger_log(&ctx->logger_port, GM_LOG_ERROR, "journal",
                                "journal_read_duration_format_failed");
            dur_buf[0] = '\0';
        }
        const gm_log_kv_t kvs[] = {
            {.key = "event", .value = (rc_walk == GM_OK) ? "journal_read_ok" : "journal_read_failed"},
            {.key = "branch", .value = resolved_branch},
            {.key = "mode", .value = mode},
            {.key = "duration_ms", .value = dur_buf},
        };
        gm_log_formatter_fn fmt = ctx && ctx->log_formatter ? ctx->log_formatter
                                                            : gm_log_format_render_default;
        gm_result_void_t fmt_rc = fmt(kvs, sizeof(kvs)/sizeof(kvs[0]), json, msg,
                                      sizeof(msg));
        if (!fmt_rc.ok) {
            if (fmt_rc.u.err != NULL) gm_error_free(fmt_rc.u.err);
            int alt = gm_snprintf(msg, sizeof(msg),
                                  "event=%s branch=%s mode=%s",
                                  (rc_walk == GM_OK) ? "journal_read_ok"
                                                     : "journal_read_failed",
                                  resolved_branch, mode);
            if (alt < 0 || (size_t)alt >= sizeof(msg)) {
                msg[0] = '\0';
            }
        }
        if (msg[0] == '\0') {
            (void)gm_logger_log(&ctx->logger_port,
                                (rc_walk == GM_OK) ? GM_LOG_INFO : GM_LOG_ERROR,
                                "journal",
                                (rc_walk == GM_OK) ? "journal_read_ok"
                                                   : "journal_read_failed");
        } else {
            (void)gm_logger_log(&ctx->logger_port,
                                (rc_walk == GM_OK) ? GM_LOG_INFO : GM_LOG_ERROR,
                                "journal", msg);
        }
    }
    return rc_walk;
}

/* Read journal for a branch */
int gm_journal_read(gm_context_t *ctx, const char *branch,
                    gm_journal_read_callback_t callback,
                    void *userdata) {
    return journal_read_generic(ctx, branch, callback, NULL, false, userdata);
}

/* Read attributed journal for a branch */
int gm_journal_read_attributed(gm_context_t *ctx, const char *branch,
                               gm_journal_read_attributed_callback_t callback,
                               void *userdata) {
    return journal_read_generic(ctx, branch, NULL, callback, true, userdata);
}
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
