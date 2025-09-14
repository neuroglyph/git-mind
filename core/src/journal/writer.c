/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/journal.h"
#include "gitmind/types.h"
#include "gitmind/context.h"
#include "gitmind/cbor/constants_cbor.h"
#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/edge.h"
#include "gitmind/edge_attributed.h"
#include "gitmind/security/string.h"

#include <git2/repository.h>
#include <git2/oid.h>
#include <git2/commit.h>
#include <git2/tree.h>
#include <git2/refs.h>
#include <git2/signature.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Local constants */
#define REFS_GITMIND_PREFIX "refs/gitmind/edges/"
#define MAX_CBOR_SIZE CBOR_MAX_STRING_LENGTH
#define REF_NAME_BUFFER_SIZE GM_PATH_MAX
#define COMMIT_ENCODING "binary"
#define CBOR_OVERFLOW_MARGIN GM_FORMAT_BUFFER_SIZE /* CBOR encoding safety margin */
#define PARENT_COMMITS_MAX 1
#define EMPTY_TREE_SHA "4b825dc642cb6eb9a060e54bf8d69288fbee4904"
#define BRANCH_BUFFER_SIZE GM_PATH_MAX

/* Journal writer context */
typedef struct {
    git_repository *repo;
    git_oid empty_tree_oid;
    char ref_name[REF_NAME_BUFFER_SIZE];
} journal_ctx_t;

/* Initialize journal context */
static int journal_init(journal_ctx_t *jctx, gm_context_t *ctx,
                        const char *branch) {
    jctx->repo = (git_repository *)ctx->git_repo;

    /* Parse empty tree OID */
    if (git_oid_fromstr(&jctx->empty_tree_oid, EMPTY_TREE_SHA) < 0) {
        return GM_ERR_UNKNOWN;
    }

    /* Build ref name */
    gm_snprintf(jctx->ref_name, sizeof(jctx->ref_name), "%s%s",
                REFS_GITMIND_PREFIX, branch);

    return GM_OK;
}

/* Get current branch name */
static int get_current_branch(git_repository *repo, char *branch_name,
                              size_t len) {
    git_reference *head = NULL;
    const char *name;
    int error;

    /* Get HEAD reference */
    error = git_repository_head(&head, repo);
    if (error < 0) {
        return GM_ERR_UNKNOWN;
    }

    /* Get branch name */
    name = git_reference_shorthand(head);
    if (!name) {
        git_reference_free(head);
        return GM_ERR_UNKNOWN;
    }

    /* Copy branch name */
    size_t n = strlen(name);
    if (n >= len) n = len - 1;
    memcpy(branch_name, name, n);
    branch_name[n] = '\0';

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
    const git_commit *parent_commits[PARENT_COMMITS_MAX];
    int parent_count = 0;
    int error;

    (void)cbor_len; /* Unused for now */

    /* Get default signature */
    error = git_signature_default(&sig, jctx->repo);
    if (error < 0) {
        return GM_ERR_UNKNOWN;
    }

    /* Get empty tree */
    error = git_tree_lookup(&tree, jctx->repo, &jctx->empty_tree_oid);
    if (error < 0) {
        git_signature_free(sig);
        return GM_ERR_UNKNOWN;
    }

    /* Try to get existing ref for parent */
    error = git_reference_lookup(&ref, jctx->repo, jctx->ref_name);
    if (error == 0) {
        /* Ref exists, get parent commit */
        error =
            git_reference_name_to_id(&parent_oid, jctx->repo, jctx->ref_name);
        if (error == 0) {
            error = git_commit_lookup(&parent, jctx->repo, &parent_oid);
            if (error == 0) {
                parent_commits[0] = parent;
                parent_count = PARENT_COMMITS_MAX;
            }
        }
        git_reference_free(ref);
    }

    /* Create commit */
    error = git_commit_create(commit_oid, jctx->repo, jctx->ref_name, sig, sig,
                              COMMIT_ENCODING, (const char *)cbor_data, tree,
                              (size_t)parent_count, parent_commits);

    /* Cleanup */
    git_signature_free(sig);
    git_tree_free(tree);
    if (parent) {
        git_commit_free(parent);
    }

    return (error < 0) ? GM_ERR_UNKNOWN : GM_OK;
}

/* Generic journal append function */
typedef int (*edge_encoder_fn)(const void *edge, uint8_t *buffer, size_t *len);

static int journal_append_generic(gm_context_t *ctx, const void *edges,
                                  size_t n_edges, size_t edge_size,
                                  edge_encoder_fn encoder) {
    journal_ctx_t jctx;
    char branch[BRANCH_BUFFER_SIZE];
    uint8_t *cbor_buffer = NULL;
    size_t offset = 0;
    git_oid commit_oid;
    int result = GM_ERR_UNKNOWN;

    if (!ctx || !edges || n_edges == 0) {
        return GM_ERR_INVALID_ARGUMENT;
    }

    /* Get current branch */
    if (get_current_branch(ctx->git_repo, branch, sizeof(branch)) != GM_OK) {
        return GM_ERR_UNKNOWN;
    }

    /* Initialize journal context */
    if (journal_init(&jctx, ctx, branch) != GM_OK) {
        return GM_ERR_UNKNOWN;
    }

    /* Allocate buffer for all edges */
    cbor_buffer = malloc(MAX_CBOR_SIZE);
    if (!cbor_buffer) {
        return GM_ERR_OUT_OF_MEMORY;
    }

    /* Encode all edges to CBOR */
    for (size_t i = 0; i < n_edges; i++) {
        size_t cbor_edge_size = MAX_CBOR_SIZE - offset;
        const void *edge = (const char *)edges + (i * edge_size);

        if (encoder(edge, cbor_buffer + offset, &cbor_edge_size) != GM_OK) {
            goto cleanup;
        }

        offset += cbor_edge_size;

        /* Check buffer overflow */
        if (offset > MAX_CBOR_SIZE - CBOR_OVERFLOW_MARGIN) {
            /* Would overflow on next edge, commit this batch */
            if (create_journal_commit(&jctx, cbor_buffer, offset,
                                      &commit_oid) != GM_OK) {
                goto cleanup;
            }
            offset = 0;
        }
    }

    /* Commit any remaining edges */
    if (offset > 0) {
        if (create_journal_commit(&jctx, cbor_buffer, offset, &commit_oid) !=
            GM_OK) {
            goto cleanup;
        }
    }

    result = GM_OK;

cleanup:
    free(cbor_buffer);
    return result;
}

/* Wrapper for regular edge encoder */
static int edge_encoder_wrapper(const void *edge, uint8_t *buffer,
                                size_t *len) {
    gm_result_void_t result = gm_edge_encode_cbor((const gm_edge_t *)edge, buffer, len);
    return (int)result.ok ? GM_OK : GM_ERR_INVALID_FORMAT;
}

/* Wrapper for attributed edge encoder */
static int edge_attributed_encoder_wrapper(const void *edge, uint8_t *buffer,
                                           size_t *len) {
    const gm_edge_attributed_t *attr_edge = (const gm_edge_attributed_t *)edge;
    
    /* Create a basic edge from the attributed edge fields */
    gm_edge_t basic_edge = {
        .rel_type = attr_edge->rel_type,
        .confidence = attr_edge->confidence,
        .timestamp = attr_edge->timestamp
    };
    
    /* Copy SHA arrays securely */
    for (size_t i = 0; i < GM_SHA1_SIZE; i++) {
        basic_edge.src_sha[i] = attr_edge->src_sha[i];
        basic_edge.tgt_sha[i] = attr_edge->tgt_sha[i];
    }
    
    /* Copy paths securely */
    size_t src_len = strlen(attr_edge->src_path);
    if (src_len >= sizeof(basic_edge.src_path)) {
        src_len = sizeof(basic_edge.src_path) - 1;
    }
    for (size_t i = 0; i < src_len; i++) {
        basic_edge.src_path[i] = attr_edge->src_path[i];
    }
    basic_edge.src_path[src_len] = '\0';
    
    size_t tgt_len = strlen(attr_edge->tgt_path);
    if (tgt_len >= sizeof(basic_edge.tgt_path)) {
        tgt_len = sizeof(basic_edge.tgt_path) - 1;
    }
    for (size_t i = 0; i < tgt_len; i++) {
        basic_edge.tgt_path[i] = attr_edge->tgt_path[i];
    }
    basic_edge.tgt_path[tgt_len] = '\0';
    
    /* Copy ULID securely */
    size_t ulid_len = strlen(attr_edge->ulid);
    if (ulid_len >= sizeof(basic_edge.ulid)) {
        ulid_len = sizeof(basic_edge.ulid) - 1;
    }
    for (size_t i = 0; i < ulid_len; i++) {
        basic_edge.ulid[i] = attr_edge->ulid[i];
    }
    basic_edge.ulid[ulid_len] = '\0';
    
    /* Encode the basic edge first */
    gm_result_void_t result = gm_edge_encode_cbor(&basic_edge, buffer, len);
    return (int)result.ok ? GM_OK : GM_ERR_INVALID_FORMAT;
}

/* Append edges to journal */
int gm_journal_append(gm_context_t *ctx, const gm_edge_t *edges,
                      size_t count) {
    return journal_append_generic(ctx, edges, count, sizeof(gm_edge_t),
                                  edge_encoder_wrapper);
}

/* Append attributed edges to journal */
int gm_journal_append_attributed(gm_context_t *ctx,
                                 const gm_edge_attributed_t *edges,
                                 size_t count) {
    return journal_append_generic(ctx, edges, count,
                                  sizeof(gm_edge_attributed_t),
                                  edge_attributed_encoder_wrapper);
}

/* Public wrapper for hooks to create commits */
int gm_journal_create_commit(gm_context_t *ctx, const char *ref,
                          const void *data, size_t len) {
    journal_ctx_t jctx;
    git_oid commit_oid;

    /* Initialize context */
    jctx.repo = (git_repository *)ctx->git_repo;

    /* Parse empty tree OID */
    if (git_oid_fromstr(&jctx.empty_tree_oid, EMPTY_TREE_SHA) < 0) {
        return GM_ERR_UNKNOWN;
    }

    /* Copy ref name securely */
    size_t ref_len = strlen(ref);
    if (ref_len >= sizeof(jctx.ref_name)) {
        ref_len = sizeof(jctx.ref_name) - 1;
    }
    for (size_t i = 0; i < ref_len; i++) {
        jctx.ref_name[i] = ref[i];
    }
    jctx.ref_name[ref_len] = '\0';

    /* Create commit */
    return create_journal_commit(&jctx, data, len, &commit_oid);
}
