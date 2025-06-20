/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind.h"
#include "gitmind/constants_internal.h"
#include "gitmind/constants_cbor.h"
#include <git2.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Constants */
#define REFS_GITMIND_PREFIX "refs/gitmind/edges/"
#define MAX_CBOR_SIZE CBOR_MAX_STRING_LENGTH

/* Forward declarations */
int gm_edge_decode_cbor_ex(const uint8_t *buffer, size_t len, gm_edge_t *edge, size_t *consumed);
int gm_edge_attributed_decode_cbor_ex(const uint8_t *buffer, size_t len, 
                                     gm_edge_attributed_t *edge, size_t *consumed);

/* Generic reader context */
typedef struct {
    gm_context_t *gm_ctx;
    git_repository *repo;
    void *callback;
    void *userdata;
    int error;
    int is_attributed;
} reader_ctx_t;

/* Process a single commit - generic version */
static int process_commit_generic(git_commit *commit, reader_ctx_t *rctx) {
    const char *raw_message;
    const uint8_t *cbor_data;
    size_t offset = 0;
    size_t message_len;
    
    /* Get raw commit message (contains CBOR data) */
    raw_message = git_commit_message_raw(commit);
    if (!raw_message) {
        return GM_ERROR;
    }
    
    cbor_data = (const uint8_t *)raw_message;
    message_len = MAX_CBOR_SIZE;
    
    /* Decode edges from CBOR */
    while (offset < message_len) {
        size_t remaining = message_len - offset;
        size_t consumed = 0;
        int cb_result;
        
        if (rctx->is_attributed) {
            gm_edge_attributed_t edge;
            memset(&edge, 0, sizeof(edge));
            
            /* Try to decode an attributed edge */
            int decode_result = gm_edge_attributed_decode_cbor_ex(cbor_data + offset, remaining, &edge, &consumed);
            if (decode_result != 0 || consumed == 0) {
                /* Try legacy format */
                gm_edge_t legacy_edge;
                size_t legacy_consumed = 0;
                
                decode_result = gm_edge_decode_cbor_ex(cbor_data + offset, remaining, &legacy_edge, &legacy_consumed);
                if (decode_result != GM_OK || legacy_consumed == 0) {
                    break;
                }
                
                /* Convert legacy edge to attributed edge */
                memcpy(edge.src_sha, legacy_edge.src_sha, SHA_BYTES_SIZE);
                memcpy(edge.tgt_sha, legacy_edge.tgt_sha, SHA_BYTES_SIZE);
                edge.rel_type = legacy_edge.rel_type;
                edge.confidence = legacy_edge.confidence;
                edge.timestamp = legacy_edge.timestamp;
                
                /* Safe string copies */
                size_t src_len = strlen(legacy_edge.src_path);
                if (src_len >= sizeof(edge.src_path)) src_len = sizeof(edge.src_path) - 1;
                memcpy(edge.src_path, legacy_edge.src_path, src_len);
                edge.src_path[src_len] = '\0';
                
                size_t tgt_len = strlen(legacy_edge.tgt_path);
                if (tgt_len >= sizeof(edge.tgt_path)) tgt_len = sizeof(edge.tgt_path) - 1;
                memcpy(edge.tgt_path, legacy_edge.tgt_path, tgt_len);
                edge.tgt_path[tgt_len] = '\0';
                
                size_t ulid_len = strlen(legacy_edge.ulid);
                if (ulid_len >= sizeof(edge.ulid)) ulid_len = sizeof(edge.ulid) - 1;
                memcpy(edge.ulid, legacy_edge.ulid, ulid_len);
                edge.ulid[ulid_len] = '\0';
                
                /* Set default attribution */
                edge.attribution.source_type = GM_SOURCE_HUMAN;
                edge.attribution.author[0] = '\0';
                edge.attribution.session_id[0] = '\0';
                edge.attribution.flags = 0;
                edge.lane = GM_LANE_DEFAULT;
                
                consumed = legacy_consumed;
            }
            
            /* Call attributed callback */
            cb_result = ((int (*)(const gm_edge_attributed_t *, void *))rctx->callback)(&edge, rctx->userdata);
        } else {
            gm_edge_t edge;
            
            /* Try to decode a regular edge */
            int decode_result = gm_edge_decode_cbor_ex(cbor_data + offset, remaining, &edge, &consumed);
            if (decode_result != GM_OK || consumed == 0) {
                break;
            }
            
            /* Call regular callback */
            cb_result = ((int (*)(const gm_edge_t *, void *))rctx->callback)(&edge, rctx->userdata);
        }
        
        if (cb_result != 0) {
            return cb_result;
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
        return GM_ERROR;
    }
    
    /* Set sorting to time order */
    git_revwalk_sorting(walker, GIT_SORT_TIME);
    
    /* Push the ref to start walking from */
    error = git_revwalk_push_ref(walker, ref_name);
    if (error < 0) {
        git_revwalk_free(walker);
        return GM_NOT_FOUND;
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
        return GM_NOT_FOUND;
    }
    
    git_revwalk_free(walker);
    return GM_OK;
}

/* Generic journal read function */
static int journal_read_generic(gm_context_t *ctx, const char *branch,
                               void *callback, void *userdata, int is_attributed) {
    reader_ctx_t rctx;
    char ref_name[REF_NAME_BUFFER_SIZE];
    char current_branch[BUFFER_SIZE_TINY];
    
    if (!ctx || !callback) {
        return GM_INVALID_ARG;
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
            return GM_ERROR;
        }
        
        const char *name = git_reference_shorthand(head);
        if (!name) {
            git_reference_free(head);
            return GM_ERROR;
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
                    int (*callback)(const gm_edge_t *edge, void *userdata),
                    void *userdata) {
    return journal_read_generic(ctx, branch, callback, userdata, 0);
}

/* Read attributed journal for a branch */
int gm_journal_read_attributed(gm_context_t *ctx, const char *branch,
                              int (*callback)(const gm_edge_attributed_t *edge, void *userdata),
                              void *userdata) {
    return journal_read_generic(ctx, branch, callback, userdata, 1);
}