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
#include "gitmind/security/memory.h"

#include <git2/types.h>
#include <sodium/utils.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

/* Local constants */
#include "gitmind/constants_internal.h"
#include "gitmind/util/ref.h"
#define MAX_CBOR_SIZE CBOR_MAX_STRING_LENGTH
#define COMMIT_ENCODING "UTF-8"
#define CBOR_OVERFLOW_MARGIN GM_FORMAT_BUFFER_SIZE /* CBOR encoding safety margin */
#define PARENT_COMMITS_MAX 1

/* Journal writer context */
typedef struct {
    gm_git_repository_port_t *repo_port;
    gm_oid_t empty_tree_oid;
    char ref_name[REF_NAME_BUFFER_SIZE];
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

    /* Parse empty tree OID */
    if (git_oid_fromstr(&jctx->empty_tree_oid, EMPTY_TREE_SHA) < 0) {
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

static int try_load_parent_commit(journal_ctx_t *jctx, git_commit **parent_out) {
    git_reference *ref = NULL;
    git_commit *local_parent = NULL;
    git_oid parent_oid;
    int lookup_rc = git_reference_lookup(&ref, jctx->repo, jctx->ref_name);
    if (lookup_rc != 0) {
        *parent_out = NULL;
        return GM_OK;
    }

    int name_to_id_rc =
        git_reference_name_to_id(&parent_oid, jctx->repo, jctx->ref_name);
    if (name_to_id_rc == 0) {
        name_to_id_rc =
            git_commit_lookup(&local_parent, jctx->repo, &parent_oid);
    }

    git_reference_free(ref);

    if (name_to_id_rc != 0) {
        if (local_parent != NULL) {
            git_commit_free(local_parent);
        }
        *parent_out = NULL;
        return GM_OK;
    }

    *parent_out = local_parent;
    return GM_OK;
}

static int encode_cbor_message(const uint8_t *cbor_data, size_t cbor_len,
                               char **message_out, size_t *message_len_out) {
    const int variant = sodium_base64_VARIANT_ORIGINAL;
    size_t required = sodium_base64_ENCODED_LEN(cbor_len, variant);
    char *encoded = (char *)malloc(required);
    if (encoded == NULL) {
        return GM_ERR_OUT_OF_MEMORY;
    }

    sodium_bin2base64(encoded, required, cbor_data, cbor_len, variant);
    if (required > 0U) {
        encoded[required - 1U] = '\0';
    }

    *message_out = encoded;
    if (message_len_out != NULL) {
        *message_len_out = required;
    }
    return GM_OK;
}

/* Create journal commit */
static int create_journal_commit(journal_ctx_t *jctx, const uint8_t *cbor_data,
                                 size_t cbor_len, git_oid *commit_oid) {
    git_signature *signature = NULL;
    git_tree *tree = NULL;
    git_commit *parent = NULL;
    const git_commit *parent_commits[PARENT_COMMITS_MAX] = {0};
    size_t parent_count = 0U;
    char *message = NULL;
    int signature_rc = git_signature_default(&signature, jctx->repo);
    if (signature_rc < 0) {
        return GM_ERR_UNKNOWN;
    }

    int tree_rc = git_tree_lookup(&tree, jctx->repo, &jctx->empty_tree_oid);
    if (tree_rc < 0) {
        git_signature_free(signature);
        return GM_ERR_UNKNOWN;
    }

    if (try_load_parent_commit(jctx, &parent) != GM_OK) {
        git_tree_free(tree);
        git_signature_free(signature);
        return GM_ERR_UNKNOWN;
    }
    if (parent != NULL) {
        parent_commits[0] = parent;
        parent_count = PARENT_COMMITS_MAX;
    }

    int message_rc = encode_cbor_message(cbor_data, cbor_len, &message, NULL);
    if (message_rc != GM_OK) {
        if (parent != NULL) {
            git_commit_free(parent);
        }
        git_tree_free(tree);
        git_signature_free(signature);
        return message_rc;
    }

    int commit_rc = git_commit_create(commit_oid, jctx->repo, jctx->ref_name,
                                      signature, signature, COMMIT_ENCODING,
                                      message, tree, parent_count,
                                      parent_commits);

    free(message);
    if (parent != NULL) {
        git_commit_free(parent);
    }
    git_tree_free(tree);
    git_signature_free(signature);

    return (commit_rc < 0) ? GM_ERR_UNKNOWN : GM_OK;
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
    const char *env_branch = getenv("GITMIND_TEST_BRANCH");
    if (env_branch != NULL && *env_branch != '\0') {
        size_t env_len = strlen(env_branch);
        if (env_len >= branch_len) {
            env_len = branch_len - 1U;
        }
        gm_memcpy_safe(branch, branch_len, env_branch, env_len);
        branch[env_len] = '\0';
        return GM_OK;
    }

    return get_current_branch(&ctx->git_repo_port, branch, branch_len);
}

static int flush_journal_batch(journal_ctx_t *jctx, const uint8_t *buffer,
                               size_t length) {
    git_oid commit_oid;
    return create_journal_commit(jctx, buffer, length, &commit_oid);
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

    if (ctx == NULL || batch.data == NULL || batch.count == 0U || encoder == NULL) {
        return GM_ERR_INVALID_ARGUMENT;
    }

    if (resolve_branch_name(ctx, branch, sizeof(branch)) != GM_OK) {
        return GM_ERR_UNKNOWN;
    }

    if (journal_init(&jctx, ctx, branch) != GM_OK) {
        return GM_ERR_UNKNOWN;
    }

    uint8_t *cbor_buffer = malloc(MAX_CBOR_SIZE);
    if (cbor_buffer == NULL) {
        return GM_ERR_OUT_OF_MEMORY;
    }

    int encode_result = encode_edges_to_journal(&jctx, &batch, cbor_buffer, encoder);
    free(cbor_buffer);
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
        ref_len = sizeof(jctx.ref_name) - 1U;
    }
    gm_memcpy_safe(jctx.ref_name, sizeof(jctx.ref_name), ref, ref_len);
    jctx.ref_name[ref_len] = '\0';

    /* Create commit */
    return create_journal_commit(&jctx, data, len, &commit_oid);
}
