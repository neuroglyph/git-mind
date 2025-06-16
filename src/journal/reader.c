/* SPDX-License-Identifier: Apache-2.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind.h"
#include <git2.h>
#include <string.h>
#include <stdlib.h>

/* Constants */
#define REFS_GITMIND_PREFIX "refs/gitmind/edges/"

/* Reader context */
typedef struct {
    gm_context_t *gm_ctx;
    git_repository *repo;
    int (*edge_callback)(const gm_edge_t *edge, void *userdata);
    void *userdata;
    int error;
} reader_ctx_t;

/* Process a single commit */
static int process_commit(git_commit *commit, reader_ctx_t *rctx) {
    const char *message;
    size_t message_len;
    const uint8_t *cbor_data;
    size_t offset = 0;
    
    /* Get commit message (contains CBOR data) */
    message = git_commit_message(commit);
    if (!message) {
        return GM_ERROR;
    }
    
    message_len = strlen(message);
    cbor_data = (const uint8_t *)message;
    
    /* Decode edges from CBOR */
    while (offset < message_len) {
        gm_edge_t edge;
        size_t remaining = message_len - offset;
        
        /* Try to decode an edge */
        if (gm_edge_decode_cbor(cbor_data + offset, remaining, &edge) != GM_OK) {
            /* End of valid CBOR data or error */
            break;
        }
        
        /* Call user callback */
        int cb_result = rctx->edge_callback(&edge, rctx->userdata);
        if (cb_result != 0) {
            /* User wants to stop */
            return cb_result;
        }
        
        /* Move to next edge */
        /* Calculate consumed bytes by re-encoding (not ideal but simple) */
        uint8_t temp_buf[1024];
        size_t edge_size = sizeof(temp_buf);
        if (gm_edge_encode_cbor(&edge, temp_buf, &edge_size) == GM_OK) {
            offset += edge_size;
        } else {
            break;
        }
    }
    
    return GM_OK;
}

/* Walk journal commits */
static int walk_journal(reader_ctx_t *rctx, const char *ref_name) {
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
    while (git_revwalk_next(&oid, walker) == 0) {
        git_commit *commit = NULL;
        
        /* Lookup commit */
        error = git_commit_lookup(&commit, rctx->repo, &oid);
        if (error < 0) {
            continue;
        }
        
        /* Process commit */
        int result = process_commit(commit, rctx);
        git_commit_free(commit);
        
        if (result != GM_OK) {
            /* User callback requested stop or error */
            git_revwalk_free(walker);
            return result;
        }
    }
    
    git_revwalk_free(walker);
    return GM_OK;
}

/* Read journal for a branch */
int gm_journal_read(gm_context_t *ctx, const char *branch,
                    int (*callback)(const gm_edge_t *edge, void *userdata),
                    void *userdata) {
    reader_ctx_t rctx;
    char ref_name[256];
    char current_branch[128];
    
    if (!ctx || !callback) {
        return GM_INVALID_ARG;
    }
    
    /* Initialize reader context */
    rctx.gm_ctx = ctx;
    rctx.repo = (git_repository *)ctx->git_repo;
    rctx.edge_callback = callback;
    rctx.userdata = userdata;
    rctx.error = GM_OK;
    
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
    return walk_journal(&rctx, ref_name);
}