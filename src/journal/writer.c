/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind.h"
#include <git2.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Constants */
#define EMPTY_TREE_SHA "4b825dc642cb6eb9a060e54bf8d69288fbee4904"
#define REFS_GITMIND_PREFIX "refs/gitmind/edges/"
#define MAX_CBOR_SIZE 8192
#define COMMIT_ENCODING "binary"

/* Journal writer context */
typedef struct {
    git_repository *repo;
    git_oid empty_tree_oid;
    char ref_name[256];
} journal_ctx_t;

/* Initialize journal context */
static int journal_init(journal_ctx_t *jctx, gm_context_t *ctx, const char *branch) {
    jctx->repo = (git_repository *)ctx->git_repo;
    
    /* Parse empty tree OID */
    if (git_oid_fromstr(&jctx->empty_tree_oid, EMPTY_TREE_SHA) < 0) {
        return GM_ERROR;
    }
    
    /* Build ref name */
    snprintf(jctx->ref_name, sizeof(jctx->ref_name), 
             "%s%s", REFS_GITMIND_PREFIX, branch);
    
    return GM_OK;
}

/* Get current branch name */
static int get_current_branch(git_repository *repo, char *branch_name, size_t len) {
    git_reference *head = NULL;
    const char *name;
    int error;
    
    /* Get HEAD reference */
    error = git_repository_head(&head, repo);
    if (error < 0) {
        return GM_ERROR;
    }
    
    /* Get branch name */
    name = git_reference_shorthand(head);
    if (!name) {
        git_reference_free(head);
        return GM_ERROR;
    }
    
    /* Copy branch name */
    strncpy(branch_name, name, len - 1);
    branch_name[len - 1] = '\0';
    
    git_reference_free(head);
    return GM_OK;
}

/* Create journal commit */
static int create_journal_commit(journal_ctx_t *jctx, const uint8_t *cbor_data, 
                                size_t cbor_len, git_oid *commit_oid) {
    git_signature *sig = NULL;
    git_reference *ref = NULL;
    git_commit *parent = NULL;
    git_tree *tree = NULL;
    git_oid parent_oid;
    const git_commit *parent_commits[1];
    int parent_count = 0;
    int error;
    
    (void)cbor_len; /* Unused for now */
    
    /* Get default signature */
    error = git_signature_default(&sig, jctx->repo);
    if (error < 0) {
        return GM_ERROR;
    }
    
    /* Get empty tree */
    error = git_tree_lookup(&tree, jctx->repo, &jctx->empty_tree_oid);
    if (error < 0) {
        git_signature_free(sig);
        return GM_ERROR;
    }
    
    /* Try to get existing ref for parent */
    error = git_reference_lookup(&ref, jctx->repo, jctx->ref_name);
    if (error == 0) {
        /* Ref exists, get parent commit */
        error = git_reference_name_to_id(&parent_oid, jctx->repo, jctx->ref_name);
        if (error == 0) {
            error = git_commit_lookup(&parent, jctx->repo, &parent_oid);
            if (error == 0) {
                parent_commits[0] = parent;
                parent_count = 1;
            }
        }
        git_reference_free(ref);
    }
    
    /* Create commit */
    error = git_commit_create(
        commit_oid,
        jctx->repo,
        jctx->ref_name,
        sig,
        sig,
        COMMIT_ENCODING,
        (const char *)cbor_data,
        tree,
        parent_count,
        parent_commits
    );
    
    /* Cleanup */
    git_signature_free(sig);
    git_tree_free(tree);
    if (parent) {
        git_commit_free(parent);
    }
    
    return (error < 0) ? GM_ERROR : GM_OK;
}

/* Append edges to journal */
int gm_journal_append(gm_context_t *ctx, const gm_edge_t *edges, size_t n_edges) {
    journal_ctx_t jctx;
    char branch[128];
    uint8_t *cbor_buffer = NULL;
    size_t offset = 0;
    git_oid commit_oid;
    int result = GM_ERROR;
    
    if (!ctx || !edges || n_edges == 0) {
        return GM_INVALID_ARG;
    }
    
    /* Get current branch */
    if (get_current_branch(ctx->git_repo, branch, sizeof(branch)) != GM_OK) {
        return GM_ERROR;
    }
    
    /* Initialize journal context */
    if (journal_init(&jctx, ctx, branch) != GM_OK) {
        return GM_ERROR;
    }
    
    /* Allocate buffer for all edges */
    cbor_buffer = malloc(MAX_CBOR_SIZE);
    if (!cbor_buffer) {
        return GM_NO_MEMORY;
    }
    
    /* Encode all edges to CBOR */
    for (size_t i = 0; i < n_edges; i++) {
        size_t edge_size = MAX_CBOR_SIZE - offset;
        
        if (gm_edge_encode_cbor(&edges[i], cbor_buffer + offset, &edge_size) != GM_OK) {
            goto cleanup;
        }
        
        offset += edge_size;
        
        /* Check buffer overflow */
        if (offset > MAX_CBOR_SIZE - 512) {
            /* Would overflow on next edge, commit this batch */
            if (create_journal_commit(&jctx, cbor_buffer, offset, &commit_oid) != GM_OK) {
                goto cleanup;
            }
            offset = 0;
        }
    }
    
    /* Commit any remaining edges */
    if (offset > 0) {
        if (create_journal_commit(&jctx, cbor_buffer, offset, &commit_oid) != GM_OK) {
            goto cleanup;
        }
    }
    
    result = GM_OK;
    
cleanup:
    free(cbor_buffer);
    return result;
}

/* Public wrapper for hooks to create commits */
int journal_create_commit(git_repository *repo, const char *ref, 
                         const void *data, size_t len) {
    journal_ctx_t jctx;
    git_oid commit_oid;
    
    /* Initialize context */
    jctx.repo = repo;
    
    /* Parse empty tree OID */
    if (git_oid_fromstr(&jctx.empty_tree_oid, EMPTY_TREE_SHA) < 0) {
        return GM_ERROR;
    }
    
    /* Copy ref name */
    strncpy(jctx.ref_name, ref, sizeof(jctx.ref_name) - 1);
    jctx.ref_name[sizeof(jctx.ref_name) - 1] = '\0';
    
    /* Create commit */
    return create_journal_commit(&jctx, data, len, &commit_oid);
}