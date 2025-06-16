/* SPDX-License-Identifier: Apache-2.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind_lib.h"
#include "gitmind_internal.h"
#include <stdio.h>
#include <string.h>

/* Check if orphan ref exists */
int gm_orphan_ref_exists_ctx(gm_context_t* ctx) {
    if (!ctx || !ctx->backend || !ctx->backend->read_ref) {
        return GM_ERR_INVALID_ARG;
    }
    
    char sha[GM_SHA1_STRING_SIZE];
    int ret = ctx->backend->read_ref(ctx, GM_GRAPH_REF, sha);
    return (ret == GM_OK) ? GM_OK : GM_ERR_NOT_FOUND;
}

/* Create orphan ref */
int gm_orphan_ref_create_ctx(gm_context_t* ctx) {
    if (!ctx || !ctx->backend) {
        return GM_ERR_INVALID_ARG;
    }
    
    /* Check if already exists */
    if (gm_orphan_ref_exists_ctx(ctx) == GM_OK) {
        return GM_OK;  /* Already exists, idempotent */
    }
    
    /* Create empty tree */
    char empty_tree_sha[GM_SHA1_STRING_SIZE];
    int ret = ctx->backend->write_tree(ctx, "", empty_tree_sha);
    if (ret != GM_OK) {
        gm_set_error_ctx(ctx, ret, "Failed to create empty tree");
        return ret;
    }
    
    /* Create orphan commit */
    char commit_sha[GM_SHA1_STRING_SIZE];
    ret = ctx->backend->create_commit(ctx, empty_tree_sha, 
                                      "", "Initialize GitMind graph", commit_sha);
    if (ret != GM_OK) {
        gm_set_error_ctx(ctx, ret, "Failed to create orphan commit");
        return ret;
    }
    
    /* Update ref */
    ret = ctx->backend->update_ref(ctx, GM_GRAPH_REF, 
                                   commit_sha, "Initialize GitMind");
    if (ret != GM_OK) {
        gm_set_error_ctx(ctx, ret, "Failed to create orphan ref");
        return ret;
    }
    
    return GM_OK;
}

/* Get current graph tree */
int gm_get_graph_tree_ctx(gm_context_t* ctx, char* out_tree_sha) {
    if (!ctx || !ctx->backend || !out_tree_sha) {
        return GM_ERR_INVALID_ARG;
    }
    
    /* Read graph ref */
    char commit_sha[GM_SHA1_STRING_SIZE];
    int ret = ctx->backend->read_ref(ctx, GM_GRAPH_REF, commit_sha);
    if (ret != GM_OK) {
        gm_set_error_ctx(ctx, GM_ERR_NOT_FOUND, "Graph ref not found");
        return GM_ERR_NOT_FOUND;
    }
    
    /* Get tree from commit */
    ret = ctx->backend->read_commit_tree(ctx, commit_sha, out_tree_sha);
    if (ret != GM_OK) {
        gm_set_error_ctx(ctx, ret, "Failed to read commit tree");
        return ret;
    }
    
    return GM_OK;
}

/* Update graph ref with new tree */
int gm_update_graph_ref_ctx(gm_context_t* ctx, const char* new_tree_sha, 
                           const char* message) {
    if (!ctx || !ctx->backend || !new_tree_sha || !message) {
        return GM_ERR_INVALID_ARG;
    }
    
    /* Get current commit (parent) */
    char parent_sha[GM_SHA1_STRING_SIZE];
    int ret = ctx->backend->read_ref(ctx, GM_GRAPH_REF, parent_sha);
    if (ret != GM_OK) {
        /* No parent - this is the first commit after orphan */
        parent_sha[0] = '\0';
    }
    
    /* Create new commit */
    char new_commit_sha[GM_SHA1_STRING_SIZE];
    ret = ctx->backend->create_commit(ctx, new_tree_sha,
                                      parent_sha, message, new_commit_sha);
    if (ret != GM_OK) {
        gm_set_error_ctx(ctx, ret, "Failed to create commit");
        return ret;
    }
    
    /* Update ref */
    ret = ctx->backend->update_ref(ctx, GM_GRAPH_REF,
                                   new_commit_sha, message);
    if (ret != GM_OK) {
        gm_set_error_ctx(ctx, ret, "Failed to update graph ref");
        return ret;
    }
    
    return GM_OK;
}