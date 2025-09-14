/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/journal.h"
#include "gitmind/types.h"
#include "gitmind/context.h"
#include "gitmind/cbor/constants_cbor.h"
#include "gitmind/cbor/cbor.h"
#include "gitmind/error.h"
#include "gitmind/edge.h"
#include "gitmind/edge_attributed.h"
#include "gitmind/attribution.h"

#include <git2/commit.h>
#include <git2/oid.h>
#include <git2/refdb.h>
#include <git2/refs.h>
#include <git2/repository.h>
#include <git2/revwalk.h>
#include <sodium.h>
#include <git2/repository.h>
#include <git2/refs.h>
#include <stdint.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gitmind/security/memory.h"
#include "gitmind/security/string.h"

/* Constants */
#define REFS_GITMIND_PREFIX "refs/gitmind/edges/"
#define MAX_CBOR_SIZE CBOR_MAX_STRING_LENGTH
#define REF_NAME_BUFFER_SIZE GM_PATH_MAX
#define CURRENT_BRANCH_BUFFER_SIZE GM_PATH_MAX

/* Forward declarations */
int gm_edge_decode_cbor_ex(const uint8_t *buffer, size_t len, gm_edge_t *edge,
                           size_t *consumed);
int gm_edge_attributed_decode_cbor_ex(const uint8_t *buffer, size_t len,
                                      gm_edge_attributed_t *edge,
                                      size_t *consumed);

/* Generic reader context */
typedef struct {
    gm_context_t *gm_ctx;
    git_repository *repo;
    void *callback;
    void *userdata;
    int error;
    int is_attributed;
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
    attributed->lane = GM_LANE_PRIMARY;
}

/* Decode attributed edge (CBOR map) with OID-first fields */
int gm_edge_attributed_decode_cbor_ex(const uint8_t *buffer, size_t len,
                                      gm_edge_attributed_t *edge,
                                      size_t *consumed) {
    if (!buffer || len == 0 || !edge || !consumed) {
        return GM_ERR_INVALID_ARGUMENT;
    }

    size_t offset = 0;
    if (offset >= len) return GM_ERR_INVALID_FORMAT;
    uint8_t initial = buffer[offset++];
    if ((initial & CBOR_TYPE_MASK) != CBOR_TYPE_MAP) {
        return GM_ERR_INVALID_FORMAT;
    }
    uint8_t addl = (uint8_t)(initial & CBOR_ADDITIONAL_INFO_MASK);
    if (addl >= CBOR_IMMEDIATE_THRESHOLD) {
        return GM_ERR_INVALID_FORMAT; /* only small maps supported */
    }
    uint32_t fields = addl;

    /* Key ids (must match writer) */
    enum {
        K_SRC_SHA = 0,
        K_TGT_SHA = 1,
        K_REL_TYPE = 2,
        K_CONFID = 3,
        K_TS = 4,
        K_SRC_PATH = 5,
        K_TGT_PATH = 6,
        K_ULID = 7,
        K_SRC_OID = 8,
        K_TGT_OID = 9,
        K_SRC_TYPE = 10,
        K_AUTHOR = 11,
        K_SESSION = 12,
        K_FLAGS = 13,
        K_LANE = 14
    };

    gm_edge_attributed_t e = {0};

    for (uint32_t i = 0; i < fields; i++) {
        /* key */
        size_t key_off = offset;
        gm_result_uint64_t keyr = gm_cbor_read_uint(buffer, &offset, len);
        if (!keyr.ok) return GM_ERR_INVALID_FORMAT;
        uint64_t key = keyr.u.val;

        switch (key) {
        case K_SRC_SHA: {
            gm_result_void_t r = gm_cbor_read_bytes(buffer, &offset, len, e.src_sha, GM_SHA1_SIZE);
            if (!r.ok) return GM_ERR_INVALID_FORMAT;
            break;
        }
        case K_TGT_SHA: {
            gm_result_void_t r = gm_cbor_read_bytes(buffer, &offset, len, e.tgt_sha, GM_SHA1_SIZE);
            if (!r.ok) return GM_ERR_INVALID_FORMAT;
            break;
        }
        case K_REL_TYPE: {
            gm_result_uint64_t r = gm_cbor_read_uint(buffer, &offset, len);
            if (!r.ok) return GM_ERR_INVALID_FORMAT;
            e.rel_type = (uint16_t)r.u.val;
            break;
        }
        case K_CONFID: {
            gm_result_uint64_t r = gm_cbor_read_uint(buffer, &offset, len);
            if (!r.ok) return GM_ERR_INVALID_FORMAT;
            e.confidence = (uint16_t)r.u.val;
            break;
        }
        case K_TS: {
            gm_result_uint64_t r = gm_cbor_read_uint(buffer, &offset, len);
            if (!r.ok) return GM_ERR_INVALID_FORMAT;
            e.timestamp = r.u.val;
            break;
        }
        case K_SRC_PATH: {
            gm_result_void_t r = gm_cbor_read_text(buffer, &offset, len, e.src_path, GM_PATH_MAX);
            if (!r.ok) return GM_ERR_INVALID_FORMAT;
            break;
        }
        case K_TGT_PATH: {
            gm_result_void_t r = gm_cbor_read_text(buffer, &offset, len, e.tgt_path, GM_PATH_MAX);
            if (!r.ok) return GM_ERR_INVALID_FORMAT;
            break;
        }
        case K_ULID: {
            gm_result_void_t r = gm_cbor_read_text(buffer, &offset, len, e.ulid, GM_ULID_SIZE + 1);
            if (!r.ok) return GM_ERR_INVALID_FORMAT;
            break;
        }
        case K_SRC_OID: {
            uint8_t raw[GM_OID_RAWSZ] = {0};
            gm_result_void_t r = gm_cbor_read_bytes(buffer, &offset, len, raw, GM_OID_RAWSZ);
            if (!r.ok) return GM_ERR_INVALID_FORMAT;
            git_oid_fromraw(&e.src_oid, raw);
            break;
        }
        case K_TGT_OID: {
            uint8_t raw[GM_OID_RAWSZ] = {0};
            gm_result_void_t r = gm_cbor_read_bytes(buffer, &offset, len, raw, GM_OID_RAWSZ);
            if (!r.ok) return GM_ERR_INVALID_FORMAT;
            git_oid_fromraw(&e.tgt_oid, raw);
            break;
        }
        case K_SRC_TYPE: {
            gm_result_uint64_t r = gm_cbor_read_uint(buffer, &offset, len);
            if (!r.ok) return GM_ERR_INVALID_FORMAT;
            e.attribution.source_type = (gm_source_type_t)r.u.val;
            break;
        }
        case K_AUTHOR: {
            gm_result_void_t r = gm_cbor_read_text(buffer, &offset, len, e.attribution.author, sizeof e.attribution.author);
            if (!r.ok) return GM_ERR_INVALID_FORMAT;
            break;
        }
        case K_SESSION: {
            gm_result_void_t r = gm_cbor_read_text(buffer, &offset, len, e.attribution.session_id, sizeof e.attribution.session_id);
            if (!r.ok) return GM_ERR_INVALID_FORMAT;
            break;
        }
        case K_FLAGS: {
            gm_result_uint64_t r = gm_cbor_read_uint(buffer, &offset, len);
            if (!r.ok) return GM_ERR_INVALID_FORMAT;
            e.attribution.flags = (uint32_t)r.u.val;
            break;
        }
        case K_LANE: {
            gm_result_uint64_t r = gm_cbor_read_uint(buffer, &offset, len);
            if (!r.ok) return GM_ERR_INVALID_FORMAT;
            e.lane = (gm_lane_type_t)r.u.val;
            break;
        }
        default:
            (void)key_off; /* suppress unused */
            return GM_ERR_INVALID_FORMAT;
        }
    }

    /* Backfill OIDs from SHA if missing */
    if (git_oid_iszero(&e.src_oid)) {
        git_oid_fromraw(&e.src_oid, e.src_sha);
    }
    if (git_oid_iszero(&e.tgt_oid)) {
        git_oid_fromraw(&e.tgt_oid, e.tgt_sha);
    }

    *edge = e;
    *consumed = offset;
    return GM_OK;
}

/* Process attributed edge from CBOR */
static int process_attributed_edge(const uint8_t *cbor_data, size_t remaining,
                                   reader_ctx_t *rctx, size_t *consumed) {
    gm_edge_attributed_t edge;
    memset(&edge, 0, sizeof(edge));

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
            return GM_ERR_INVALID_FORMAT;
        }

        /* Convert legacy edge to attributed edge */
        convert_legacy_to_attributed(&legacy_edge, &edge);
        *consumed = legacy_consumed;
    }

    /* Call attributed callback */
    return ((int (*)(const gm_edge_attributed_t *, void *))rctx->callback)(
        &edge, rctx->userdata);
}

/* Process regular edge from CBOR */
static int process_regular_edge(const uint8_t *cbor_data, size_t remaining,
                                reader_ctx_t *rctx, size_t *consumed) {
    gm_edge_t edge;

    /* Try to decode a regular edge */
    int decode_result = gm_edge_decode_cbor_ex(cbor_data, remaining, &edge, consumed);
    if (decode_result == GM_OK && *consumed > 0) {
        return ((int (*)(const gm_edge_t *, void *))rctx->callback)(&edge, rctx->userdata);
    }

    /* Fallback: try attributed and down-convert to regular edge */
    gm_edge_attributed_t aedge;
    size_t aconsumed = 0;
    decode_result = gm_edge_attributed_decode_cbor_ex(cbor_data, remaining, &aedge, &aconsumed);
    if (decode_result != GM_OK || aconsumed == 0) {
        return GM_ERR_INVALID_FORMAT;
    }

    gm_edge_t basic = {0};
    memcpy(basic.src_sha, aedge.src_sha, GM_SHA1_SIZE);
    memcpy(basic.tgt_sha, aedge.tgt_sha, GM_SHA1_SIZE);
    basic.src_oid = aedge.src_oid;
    basic.tgt_oid = aedge.tgt_oid;
    basic.rel_type = aedge.rel_type;
    basic.confidence = aedge.confidence;
    basic.timestamp = aedge.timestamp;
    strncpy(basic.src_path, aedge.src_path, GM_PATH_MAX - 1);
    basic.src_path[GM_PATH_MAX - 1] = '\0';
    strncpy(basic.tgt_path, aedge.tgt_path, GM_PATH_MAX - 1);
    basic.tgt_path[GM_PATH_MAX - 1] = '\0';
    strncpy(basic.ulid, aedge.ulid, GM_ULID_SIZE);
    basic.ulid[GM_ULID_SIZE] = '\0';

    *consumed = aconsumed;
    return ((int (*)(const gm_edge_t *, void *))rctx->callback)(&basic, rctx->userdata);
}

/* Process a single commit - generic version */
static int process_commit_generic(git_commit *commit, reader_ctx_t *rctx) {
    const char *raw_message;
    const uint8_t *cbor_data;
    size_t offset = 0;
    size_t message_len;
    uint8_t decoded[MAX_CBOR_SIZE];

    /* Get raw commit message (contains CBOR data) */
    raw_message = git_commit_message_raw(commit);
    if (!raw_message) {
        return GM_ERR_INVALID_FORMAT;
    }

    {
        size_t out_len = 0;
        const int variant = sodium_base64_VARIANT_ORIGINAL;
        if (sodium_base642bin(decoded, sizeof(decoded), raw_message,
                               strlen(raw_message), NULL, &out_len, NULL,
                               variant) != 0) {
            return GM_ERR_INVALID_FORMAT;
        }
        cbor_data = decoded;
        message_len = out_len;
    }

    /* Decode edges from CBOR */
    while (offset < message_len) {
        size_t remaining = message_len - offset;
        size_t consumed = 0;
        int cb_result;

        if (rctx->is_attributed) {
            cb_result = process_attributed_edge(cbor_data + offset, remaining,
                                                rctx, &consumed);
        } else {
            cb_result = process_regular_edge(cbor_data + offset, remaining,
                                             rctx, &consumed);
        }

        if (cb_result == GM_ERR_INVALID_FORMAT) {
            break; /* No more edges to decode */
        }

        if (cb_result != 0) {
            return cb_result; /* Callback requested stop */
        }

        offset += consumed;
    }

    return GM_OK;
}

/* Walk journal commits - generic version */
static int walk_journal_generic(reader_ctx_t *rctx, const char *ref_name) {
    git_revwalk *walker = NULL;
    git_oid oid;
    int error;

    /* Create revision walker */
    error = git_revwalk_new(&walker, rctx->repo);
    if (error < 0) {
        return GM_ERR_INVALID_FORMAT;
    }

    /* Set sorting to time order */
    git_revwalk_sorting(walker, GIT_SORT_TIME);

    /* Push the ref to start walking from */
    error = git_revwalk_push_ref(walker, ref_name);
    if (error < 0) {
        git_revwalk_free(walker);
        return GM_ERR_NOT_FOUND;
    }

    /* Walk commits */
    int commit_count = 0;
    while (git_revwalk_next(&oid, walker) == 0) {
        git_commit *commit = NULL;
        commit_count++;

        /* Lookup commit */
        error = git_commit_lookup(&commit, rctx->repo, &oid);
        if (error < 0) {
            continue;
        }

        /* Process commit */
        int result = process_commit_generic(commit, rctx);
        git_commit_free(commit);

        if (result != GM_OK) {
            /* User callback requested stop or error */
            git_revwalk_free(walker);
            return result;
        }
    }

    /* If no commits found, return NOT_FOUND */
    if (commit_count == 0) {
        git_revwalk_free(walker);
        return GM_ERR_NOT_FOUND;
    }

    git_revwalk_free(walker);
    return GM_OK;
}

/* Generic journal read function */
static int journal_read_generic(gm_context_t *ctx, const char *branch,
                                void *callback, void *userdata,
                                int is_attributed) {
    reader_ctx_t rctx;
    char ref_name[REF_NAME_BUFFER_SIZE];
    char current_branch[CURRENT_BRANCH_BUFFER_SIZE];

    if (!ctx || !callback) {
        return GM_ERR_INVALID_ARGUMENT;
    }

    /* Initialize reader context */
    rctx.gm_ctx = ctx;
    rctx.repo = (git_repository *)ctx->git_repo;
    rctx.callback = callback;
    rctx.userdata = userdata;
    rctx.error = GM_OK;
    rctx.is_attributed = is_attributed;

    /* Determine branch */
    if (!branch) {
        /* Use current branch */
        git_reference *head = NULL;
        int error = git_repository_head(&head, rctx.repo);
        if (error < 0) {
            return GM_ERR_INVALID_FORMAT;
        }

        const char *name = git_reference_shorthand(head);
        if (!name) {
            git_reference_free(head);
            return GM_ERR_INVALID_FORMAT;
        }

        size_t name_len = strlen(name);
        if (name_len >= sizeof(current_branch)) {
            name_len = sizeof(current_branch) - 1;
        }
        gm_memcpy_safe(current_branch, sizeof(current_branch), name, name_len);
        current_branch[name_len] = '\0';
        branch = current_branch;

        git_reference_free(head);
    }

    /* Build ref name */
    {
        int rn = gm_snprintf(ref_name, sizeof(ref_name), "%s%s", REFS_GITMIND_PREFIX, branch);
        if (rn < 0 || (size_t)rn >= sizeof(ref_name)) {
            return GM_ERR_BUFFER_TOO_SMALL;
        }
    }

    /* Walk the journal */
    return walk_journal_generic(&rctx, ref_name);
}

/* Read journal for a branch */
int gm_journal_read(gm_context_t *ctx, const char *branch,
                    gm_journal_read_callback_t callback,
                    void *userdata) {
    return journal_read_generic(ctx, branch, (void*)callback, userdata, 0);
}

/* Read attributed journal for a branch */
int gm_journal_read_attributed(gm_context_t *ctx, const char *branch,
                               gm_journal_read_attributed_callback_t callback,
                               void *userdata) {
    return journal_read_generic(ctx, branch, (void*)callback, userdata, 1);
}
