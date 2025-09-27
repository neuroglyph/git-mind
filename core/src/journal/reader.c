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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
} reader_ctx_t;

/* Convert legacy edge to attributed edge */
static void convert_legacy_to_attributed(const gm_edge_t *legacy,
                                         gm_edge_attributed_t *attributed) {
    /* Copy basic fields using secure byte-by-byte copying */
    for (size_t i = 0; i < GM_SHA1_SIZE; i++) {
        attributed->src_sha[i] = legacy->src_sha[i];
        attributed->tgt_sha[i] = legacy->tgt_sha[i];
    }
    attributed->rel_type = legacy->rel_type;
    attributed->confidence = legacy->confidence;
    attributed->timestamp = legacy->timestamp;

    /* Safe string copies using byte-by-byte copying */
    size_t src_len = strlen(legacy->src_path);
    if (src_len >= sizeof(attributed->src_path)) {
        src_len = sizeof(attributed->src_path) - 1;
    }
    for (size_t i = 0; i < src_len; i++) {
        attributed->src_path[i] = legacy->src_path[i];
    }
    attributed->src_path[src_len] = '\0';

    size_t tgt_len = strlen(legacy->tgt_path);
    if (tgt_len >= sizeof(attributed->tgt_path)) {
        tgt_len = sizeof(attributed->tgt_path) - 1;
    }
    for (size_t i = 0; i < tgt_len; i++) {
        attributed->tgt_path[i] = legacy->tgt_path[i];
    }
    attributed->tgt_path[tgt_len] = '\0';

    size_t ulid_len = strlen(legacy->ulid);
    if (ulid_len >= sizeof(attributed->ulid)) {
        ulid_len = sizeof(attributed->ulid) - 1;
    }
    for (size_t i = 0; i < ulid_len; i++) {
        attributed->ulid[i] = legacy->ulid[i];
    }
    attributed->ulid[ulid_len] = '\0';

    /* Set default attribution */
    attributed->attribution.source_type = GM_SOURCE_HUMAN;
    attributed->attribution.author[0] = '\0';
    attributed->attribution.session_id[0] = '\0';
    attributed->attribution.flags = 0;
    attributed->lane = GM_LANE_DEFAULT;
}

/* (Removed local attributed CBOR decoder; use public edge API) */

/* Process attributed edge from CBOR */
static int process_attributed_edge(const uint8_t *cbor_data, size_t remaining,
                                   reader_ctx_t *rctx, size_t *consumed) {
    gm_edge_attributed_t edge;
    gm_memset_safe(&edge, sizeof(edge), 0, sizeof(edge));

    /* Try to decode an attributed edge */
    int decode_result = gm_edge_attributed_decode_cbor_ex(cbor_data, remaining,
                                                          &edge, consumed);
    if (decode_result != 0 || *consumed == 0) {
        /* Try legacy format */
        gm_edge_t legacy_edge;
        size_t legacy_consumed = 0;

        decode_result = gm_edge_decode_cbor_ex(cbor_data, remaining,
                                               &legacy_edge, &legacy_consumed);
        if (decode_result != GM_OK || legacy_consumed == 0) {
            cbor_debug_log_two_values(
                "[CBOR DEBUG] Attributed decode failed at offset=%zu remaining=%zu\n",
                (size_t)0, remaining);
            return GM_ERR_INVALID_FORMAT;
        }

        /* Convert legacy edge to attributed edge */
        convert_legacy_to_attributed(&legacy_edge, &edge);
        *consumed = legacy_consumed;
    }

    if (rctx->edge_attr_callback == NULL) {
        return GM_ERR_INVALID_ARGUMENT;
    }
    return rctx->edge_attr_callback(&edge, rctx->userdata);
}

/* Process regular edge from CBOR */
static int process_regular_edge(const uint8_t *cbor_data, size_t remaining,
                                reader_ctx_t *rctx, size_t *consumed) {
    gm_edge_t edge;

    /* Try to decode a regular edge */
    int decode_result = gm_edge_decode_cbor_ex(cbor_data, remaining, &edge, consumed);
    if (decode_result == GM_OK && *consumed > 0) {
        if (rctx->edge_callback == NULL) {
            return GM_ERR_INVALID_ARGUMENT;
        }
        return rctx->edge_callback(&edge, rctx->userdata);
    }

    /* Fallback: try attributed and down-convert to regular edge */
    gm_edge_attributed_t aedge;
    size_t aconsumed = 0;
    decode_result = gm_edge_attributed_decode_cbor_ex(cbor_data, remaining, &aedge, &aconsumed);
        if (decode_result != GM_OK || aconsumed == 0) {
            cbor_debug_log_two_values(
                "[CBOR DEBUG] Regular decode failed; attributed decode also failed at offset=%zu remaining=%zu\n",
                (size_t)0, remaining);
            return GM_ERR_INVALID_FORMAT;
        }

    gm_edge_t basic = {0};
    (void)gm_memcpy_span(basic.src_sha, GM_SHA1_SIZE, aedge.src_sha, GM_SHA1_SIZE);
    (void)gm_memcpy_span(basic.tgt_sha, GM_SHA1_SIZE, aedge.tgt_sha, GM_SHA1_SIZE);
    basic.src_oid = aedge.src_oid;
    basic.tgt_oid = aedge.tgt_oid;
    basic.rel_type = aedge.rel_type;
    basic.confidence = aedge.confidence;
    basic.timestamp = aedge.timestamp;
    (void)gm_strcpy_safe(basic.src_path, GM_PATH_MAX, aedge.src_path);
    (void)gm_strcpy_safe(basic.tgt_path, GM_PATH_MAX, aedge.tgt_path);
    (void)gm_strcpy_safe(basic.ulid, GM_ULID_SIZE + 1, aedge.ulid);

    *consumed = aconsumed;
    if (rctx->edge_callback == NULL) {
        return GM_ERR_INVALID_ARGUMENT;
    }
    return rctx->edge_callback(&basic, rctx->userdata);
}

/* Process a single commit - generic version */
static int decode_commit_message(const char *raw_message,
                                 uint8_t *decoded_message,
                                 size_t *decoded_length) {
    if (raw_message == NULL) {
        return GM_ERR_INVALID_FORMAT;
    }

    const size_t raw_length = strlen(raw_message);
    const int variant = sodium_base64_VARIANT_ORIGINAL;
    size_t out_len = 0;
    if (sodium_base642bin(decoded_message, MAX_CBOR_SIZE, raw_message,
                          raw_length, NULL, &out_len, NULL, variant) != 0) {
        return GM_ERR_INVALID_FORMAT;
    }

    *decoded_length = out_len;
    return GM_OK;
}

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
        if (head_result.u.err != NULL) {
            gm_error_free(head_result.u.err);
        }
        return GM_ERR_INVALID_FORMAT;
    }

    *resolved_branch = buffer;
    return GM_OK;
}

static int process_commit_generic(const char *raw_message, reader_ctx_t *rctx) {
    uint8_t decoded[MAX_CBOR_SIZE];
    size_t message_len = 0;
    int decode_status = decode_commit_message(raw_message, decoded, &message_len);
    if (decode_status != GM_OK) {
        return decode_status;
    }

    size_t offset = 0U;
    while (offset < message_len) {
        const size_t remaining = message_len - offset;
        size_t consumed = 0U;
        int edge_status = GM_OK;
        if (rctx->is_attributed) {
            edge_status = process_attributed_edge(decoded + offset, remaining,
                                                  rctx, &consumed);
        } else {
            edge_status = process_regular_edge(decoded + offset, remaining,
                                               rctx, &consumed);
        }

        if (edge_status == GM_ERR_INVALID_FORMAT) {
            cbor_debug_log_two_values(
                "[CBOR DEBUG] Invalid CBOR at commit decode offset=%zu remaining=%zu\n",
                offset, remaining);
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
        if (message_result.u.err != NULL) {
            gm_error_free(message_result.u.err);
        }
        return GM_ERR_INVALID_FORMAT;
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

    /* Walk the journal */
    return walk_journal_generic(&rctx, ref_name);
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
