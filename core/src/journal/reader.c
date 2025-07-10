/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/journal.h"
#include "gitmind/types.h"
#include "gitmind/context.h"
#include "gitmind/cbor/cbor.h"
#include "gitmind/cbor/constants_cbor.h"
#include "gitmind/error.h"
#include "gitmind/result.h"

#include <git2.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    /* Copy basic fields */
    memcpy(attributed->src_sha, legacy->src_sha, GM_SHA1_SIZE);
    memcpy(attributed->tgt_sha, legacy->tgt_sha, GM_SHA1_SIZE);
    attributed->rel_type = legacy->rel_type;
    attributed->confidence = legacy->confidence;
    attributed->timestamp = legacy->timestamp;

    /* Safe string copies */
    size_t src_len = strlen(legacy->src_path);
    if (src_len >= sizeof(attributed->src_path))
        src_len = sizeof(attributed->src_path) - 1;
    memcpy(attributed->src_path, legacy->src_path, src_len);
    attributed->src_path[src_len] = '\0';

    size_t tgt_len = strlen(legacy->tgt_path);
    if (tgt_len >= sizeof(attributed->tgt_path))
        tgt_len = sizeof(attributed->tgt_path) - 1;
    memcpy(attributed->tgt_path, legacy->tgt_path, tgt_len);
    attributed->tgt_path[tgt_len] = '\0';

    size_t ulid_len = strlen(legacy->ulid);
    if (ulid_len >= sizeof(attributed->ulid))
        ulid_len = sizeof(attributed->ulid) - 1;
    memcpy(attributed->ulid, legacy->ulid, ulid_len);
    attributed->ulid[ulid_len] = '\0';

    /* Set default attribution */
    attributed->attribution.source_type = GM_SOURCE_HUMAN;
    attributed->attribution.author[0] = '\0';
    attributed->attribution.session_id[0] = '\0';
    attributed->attribution.flags = 0;
    attributed->lane = GM_LANE_PRIMARY;
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
    int decode_result =
        gm_edge_decode_cbor_ex(cbor_data, remaining, &edge, consumed);
    if (decode_result != GM_OK || *consumed == 0) {
        return GM_ERR_INVALID_FORMAT;
    }

    /* Call regular callback */
    return ((int (*)(const gm_edge_t *, void *))rctx->callback)(&edge,
                                                                rctx->userdata);
}

/* Process a single commit - generic version */
static int process_commit_generic(git_commit *commit, reader_ctx_t *rctx) {
    const char *raw_message;
    const uint8_t *cbor_data;
    size_t offset = 0;
    size_t message_len;

    /* Get raw commit message (contains CBOR data) */
    raw_message = git_commit_message_raw(commit);
    if (!raw_message) {
        return GM_ERR_INVALID_FORMAT;
    }

    cbor_data = (const uint8_t *)raw_message;
    message_len = MAX_CBOR_SIZE;

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

        strncpy(current_branch, name, sizeof(current_branch) - 1);
        current_branch[sizeof(current_branch) - 1] = '\0';
        branch = current_branch;

        git_reference_free(head);
    }

    /* Build ref name */
    snprintf(ref_name, sizeof(ref_name), "%s%s", REFS_GITMIND_PREFIX, branch);

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
