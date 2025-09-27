/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/hooks/augment.h"

#include <ctype.h>
#include <string.h>
#include <time.h>

#include <git2/oid.h>

#include "gitmind/constants.h"
#include "gitmind/constants_internal.h"
#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/util/memory.h"
#include "gitmind/util/ref.h"
#include "gitmind/security/memory.h"

/* Note: replace any private header usage with public equivalents when available. */

/* Array management constants */
#define INITIAL_EDGE_ARRAY_SIZE 10
#define ARRAY_GROWTH_FACTOR 2
#define AUGMENT_CONFIDENCE 100

/* External functions */
int gm_journal_append(gm_context_t *ctx, const gm_edge_t *edges,
                      size_t n_edges);
int gm_journal_read(gm_context_t *ctx, const char *branch,
                    int (*callback)(const gm_edge_t *edge, void *userdata),
                    void *userdata);
int gm_ulid_generate(char *ulid);

typedef struct {
    gm_oid_t *storage;
    size_t capacity;
    size_t count;
} hook_commit_collect_ctx_t;

static int hook_result_to_code(gm_result_void_t result, int fallback) {
    if (result.ok) {
        return GM_OK;
    }
    int code = fallback;
    if (result.u.err != NULL) {
        code = result.u.err->code;
        gm_error_free(result.u.err);
    }
    return code;
}

static bool parse_head_offset(const char *commit_ref, size_t *offset_out) {
    if (commit_ref == NULL || offset_out == NULL) {
        return false;
    }
    if (strcmp(commit_ref, "HEAD") == 0) {
        *offset_out = 0U;
        return true;
    }
    const char *prefix = "HEAD~";
    const size_t prefix_len = strlen(prefix);
    if (strncmp(commit_ref, prefix, prefix_len) != 0) {
        return false;
    }
    const char *cursor = commit_ref + prefix_len;
    if (*cursor == '\0') {
        return false;
    }

    size_t value = 0U;
    while (*cursor != '\0') {
        if (!isdigit((unsigned char)*cursor)) {
            return false;
        }
        value = (value * 10U) + (size_t)(*cursor - '0');
        cursor++;
    }

    *offset_out = value;
    return true;
}

static int collect_commit_oid(const gm_oid_t *commit_oid, void *userdata) {
    hook_commit_collect_ctx_t *ctx = (hook_commit_collect_ctx_t *)userdata;
    if (ctx == NULL || commit_oid == NULL) {
        return GM_ERR_INVALID_ARGUMENT;
    }
    if (ctx->count < ctx->capacity) {
        ctx->storage[ctx->count] = *commit_oid;
        ctx->count += 1U;
        if (ctx->count >= ctx->capacity) {
            return GM_CALLBACK_STOP;
        }
    }
    return GM_OK;
}

/* Get blob SHA for a file at a specific commit */
int gm_hook_get_blob_sha(const gm_git_repository_port_t *repo_port,
                         const char *commit_ref,
                         const char *file_path, gm_oid_t *sha_out) {
    if (repo_port == NULL || commit_ref == NULL || file_path == NULL ||
        sha_out == NULL) {
        return GM_ERR_INVALID_ARGUMENT;
    }

    gm_memset_safe(sha_out, sizeof(*sha_out), 0, sizeof(*sha_out));

    size_t offset = 0U;
    if (!parse_head_offset(commit_ref, &offset)) {
        return GM_ERR_INVALID_ARGUMENT;
    }

    if (offset == 0U) {
        gm_result_void_t head_result =
            gm_git_repository_port_resolve_blob_at_head(repo_port, file_path,
                                                        sha_out);
        return hook_result_to_code(head_result, GM_ERR_NOT_FOUND);
    }

    if (offset >= GM_AUGMENT_LOOKBACK_LIMIT) {
        return GM_ERR_INVALID_ARGUMENT;
    }

    const size_t required_commits = offset + 1U;
    gm_oid_t commit_buffer[GM_AUGMENT_LOOKBACK_LIMIT];
    hook_commit_collect_ctx_t collect_ctx = {
        .storage = commit_buffer,
        .capacity = required_commits,
        .count = 0U,
    };

    const char *walk_ref = "HEAD";
    char branch[BUFFER_SIZE_SMALL];
    char ref_name[REF_NAME_BUFFER_SIZE];
    gm_result_void_t branch_result =
        gm_git_repository_port_head_branch(repo_port, branch, sizeof(branch));
    int branch_rc = hook_result_to_code(branch_result, GM_ERR_NOT_FOUND);
    if (branch_rc == GM_OK) {
        int ref_rc = gm_build_ref(ref_name, sizeof(ref_name), REFS_HEADS_PREFIX,
                                  branch);
        if (ref_rc == GM_OK) {
            walk_ref = ref_name;
        }
    } else if (branch_rc != GM_ERR_NOT_FOUND) {
        return branch_rc;
    } else {
        /* Detached HEAD: fall back to walking the symbolic HEAD reference. */
        walk_ref = "HEAD";
    }

    gm_result_void_t walk_result = gm_git_repository_port_walk_commits(
        repo_port, walk_ref, collect_commit_oid, &collect_ctx);
    int walk_rc = hook_result_to_code(walk_result, GM_ERR_NOT_FOUND);
    if (walk_rc != GM_OK) {
        return walk_rc;
    }

    if (collect_ctx.count <= offset) {
        return GM_ERR_NOT_FOUND;
    }

    gm_result_void_t resolve_result = gm_git_repository_port_resolve_blob_at_commit(
        repo_port, &collect_ctx.storage[offset], file_path, sha_out);
    return hook_result_to_code(resolve_result, GM_ERR_NOT_FOUND);
}

/* Callback structure for edge search */
struct edge_search_ctx {
    const gm_oid_t *target_oid;
    gm_edge_t *edges;
    size_t capacity;
    size_t count;
    size_t scanned;
};

/* Callback for journal walk */
static int edge_search_callback(const gm_edge_t *edge, void *userdata) {
    struct edge_search_ctx *ctx = (struct edge_search_ctx *)userdata;

    /* Limit lookback */
    ctx->scanned++;
    if (ctx->scanned > LOOKBACK_LIMIT) {
        return 1; /* Stop iteration */
    }

    /* Check if source matches (OID compare) */
    if (git_oid_cmp(&edge->src_oid, ctx->target_oid) == 0) {
        /* Grow array if needed */
        if (ctx->count >= ctx->capacity) {
            size_t new_capacity = ctx->capacity * ARRAY_GROWTH_FACTOR;
            gm_edge_t *new_edges =
                realloc(ctx->edges, new_capacity * sizeof(gm_edge_t));
            if (!new_edges) {
                return GM_ERR_OUT_OF_MEMORY;
            }
            ctx->edges = new_edges;
            ctx->capacity = new_capacity;
        }

        /* Copy edge */
        ctx->edges[ctx->count++] = *edge;
    }

    return 0; /* Continue */
}

/* Find recent edges with given source blob */
int gm_hook_find_edges_by_source(gm_context_t *ctx, const gm_oid_t *src_oid,
                         gm_edge_t **edges_out, size_t *count_out) {
    struct edge_search_ctx search_ctx = {
        .target_oid = src_oid,
        .edges = malloc(INITIAL_EDGE_ARRAY_SIZE * sizeof(gm_edge_t)),
        .capacity = INITIAL_EDGE_ARRAY_SIZE,
        .count = 0,
        .scanned = 0};

    if (!search_ctx.edges) {
        return GM_ERR_OUT_OF_MEMORY;
    }

    /* Walk journal looking for edges */
    int error = gm_journal_read(ctx, NULL, edge_search_callback, &search_ctx);

    if (error < 0 && error != 1) { /* 1 means we stopped early */
        free(search_ctx.edges);
        return error;
    }

    *edges_out = search_ctx.edges;
    *count_out = search_ctx.count;
    return GM_OK;
}

/* Create AUGMENTS edge between two blob versions */
int gm_hook_create_augments_edge(gm_context_t *ctx, const gm_oid_t *old_oid,
                         const gm_oid_t *new_oid, const char *file_path) {
    gm_edge_t edge;
    gm_memset_safe(&edge, sizeof(edge), 0, sizeof(edge));

    /* Initialize edge */
    edge.src_oid = *old_oid;
    edge.tgt_oid = *new_oid;
    edge.rel_type = GM_REL_AUGMENTS;
    edge.confidence = AUGMENT_CONFIDENCE; /* Always 100% for augments */
    edge.timestamp = (uint64_t)time(NULL);

    /* Set paths (both same for AUGMENTS); fail on truncation */
    int rc_src = gm_strcpy_safe(edge.src_path, GM_PATH_MAX, file_path);
    int rc_tgt = gm_strcpy_safe(edge.tgt_path, GM_PATH_MAX, file_path);
    if (rc_src != GM_OK || rc_tgt != GM_OK) {
        gm_memset_safe(edge.src_path, sizeof edge.src_path, 0, sizeof edge.src_path);
        gm_memset_safe(edge.tgt_path, sizeof edge.tgt_path, 0, sizeof edge.tgt_path);
        return (rc_src != GM_OK) ? rc_src : rc_tgt;
    }

    /* Generate ULID */
    int ulid_rc = gm_ulid_generate(edge.ulid);
    if (ulid_rc != GM_OK) {
        gm_memset_safe(edge.ulid, sizeof edge.ulid, 0, sizeof edge.ulid);
        return ulid_rc;
    }

    /* Append to journal */
    return gm_journal_append(ctx, &edge, 1);
}

/* Process a single changed file */
int gm_hook_process_changed_file(gm_context_t *ctx,
                                 const gm_git_repository_port_t *repo_port,
                                 const char *file_path) {
    gm_oid_t old_oid;
    gm_oid_t new_oid;
    gm_edge_t *edges = NULL;
    size_t edge_count = 0;
    int error;

    /* Get old blob SHA */
    error = gm_hook_get_blob_sha(repo_port, "HEAD~1", file_path, &old_oid);
    if (error == GM_ERR_NOT_FOUND) {
        /* File might be new, skip */
        return GM_OK;
    }
    if (error != GM_OK) {
        return error;
    }

    /* Get new blob SHA */
    error = gm_hook_get_blob_sha(repo_port, "HEAD", file_path, &new_oid);
    if (error == GM_ERR_NOT_FOUND) {
        /* File might be deleted, skip */
        return GM_OK;
    }
    if (error != GM_OK) {
        return error;
    }

    /* Find edges with old blob as source */
    error = gm_hook_find_edges_by_source(ctx, &old_oid, &edges, &edge_count);
    if (error != GM_OK) {
        return error;
    }

    /* If no edges found, this file wasn't tracked */
    if (edge_count == 0) {
        free(edges);
        return GM_OK;
    }

    /* Create AUGMENTS edge */
    error = gm_hook_create_augments_edge(ctx, &old_oid, &new_oid, file_path);

    free(edges);
    return error;
}

/* Check if this is a merge commit */
int gm_hook_is_merge_commit(const gm_git_repository_port_t *repo_port,
                            bool *is_merge) {
    if (repo_port == NULL || is_merge == NULL) {
        return GM_ERR_INVALID_ARGUMENT;
    }

    gm_git_reference_tip_t tip;
    gm_memset_safe(&tip, sizeof(tip), 0, sizeof(tip));
    const char *ref_to_query = "HEAD";
    char branch[BUFFER_SIZE_SMALL];
    char ref_name[REF_NAME_BUFFER_SIZE];
    gm_result_void_t branch_result =
        gm_git_repository_port_head_branch(repo_port, branch, sizeof(branch));
    int branch_rc = hook_result_to_code(branch_result, GM_ERR_NOT_FOUND);
    if (branch_rc == GM_OK) {
        int ref_rc =
            gm_build_ref(ref_name, sizeof(ref_name), REFS_HEADS_PREFIX, branch);
        if (ref_rc == GM_OK) {
            ref_to_query = ref_name;
        }
    } else if (branch_rc != GM_ERR_NOT_FOUND) {
        return branch_rc;
    }

    gm_result_void_t tip_result =
        gm_git_repository_port_reference_tip(repo_port, ref_to_query, &tip);
    int tip_rc = hook_result_to_code(tip_result, GM_ERR_NOT_FOUND);
    if (tip_rc != GM_OK || !tip.has_target) {
        *is_merge = false;
        return tip_rc;
    }

    size_t parent_total = 0U;
    gm_result_void_t parent_result = gm_git_repository_port_commit_parent_count(
        repo_port, &tip.oid, &parent_total);
    int parent_rc = hook_result_to_code(parent_result, GM_ERR_UNKNOWN);
    if (parent_rc != GM_OK) {
        *is_merge = false;
        return parent_rc;
    }

    *is_merge = (parent_total > 1U);
    return GM_OK;
}
