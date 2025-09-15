/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/hooks/augment.h"

#include <string.h>
#include "gitmind/util/memory.h"
#include <time.h>

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

/* Get blob SHA for a file at a specific commit */
int gm_hook_get_blob_sha(git_repository *repo, const char *commit_ref,
                 const char *file_path, gm_oid_t *sha_out) {
    git_object *obj = NULL;
    git_commit *commit = NULL;
    git_tree *tree = NULL;
    git_tree_entry *entry = NULL;
    int error;

    /* Resolve commit reference */
    error = git_revparse_single(&obj, repo, commit_ref);
    if (error < 0) {
        return GM_ERR_NOT_FOUND;
    }

    /* Get commit from object */
    error = git_commit_lookup(&commit, repo, git_object_id(obj));
    git_object_free(obj);
    if (error < 0) {
        return GM_ERR_NOT_FOUND;
    }

    /* Get tree from commit */
    error = git_commit_tree(&tree, commit);
    git_commit_free(commit);
    if (error < 0) {
        return GM_ERR_NOT_FOUND;
    }

    /* Look up file in tree */
    error = git_tree_entry_bypath(&entry, tree, file_path);
    git_tree_free(tree);
    if (error < 0) {
        return GM_NOT_FOUND;
    }

    /* Ensure it's a blob (not directory/submodule) */
    if (git_tree_entry_type(entry) != GIT_OBJECT_BLOB) {
        git_tree_entry_free(entry);
        return GM_ERR_NOT_FOUND;
    }

    /* Copy OID */
    const git_oid *oid = git_tree_entry_id(entry);
    *sha_out = *oid;

    git_tree_entry_free(entry);
    return GM_OK;
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

    /* Initialize edge */
    edge.src_oid = *old_oid;
    edge.tgt_oid = *new_oid;
    edge.rel_type = GM_REL_AUGMENTS;
    edge.confidence = AUGMENT_CONFIDENCE; /* Always 100% for augments */
    edge.timestamp = (uint64_t)time(NULL);

    /* Set paths (both same for AUGMENTS) */
    (void)gm_strcpy_safe(edge.src_path, GM_PATH_MAX, file_path);
    (void)gm_strcpy_safe(edge.tgt_path, GM_PATH_MAX, file_path);

    /* Generate ULID */
    gm_ulid_generate(edge.ulid);

    /* Append to journal */
    return gm_journal_append(ctx, &edge, 1);
}

/* Process a single changed file */
int gm_hook_process_changed_file(gm_context_t *ctx, git_repository *repo,
                         const char *file_path) {
    gm_oid_t old_oid;
    gm_oid_t new_oid;
    gm_edge_t *edges = NULL;
    size_t edge_count = 0;
    int error;

    /* Get old blob SHA */
    error = gm_hook_get_blob_sha(repo, "HEAD~1", file_path, &old_oid);
    if (error != GM_OK) {
        /* File might be new, skip */
        return GM_OK;
    }

    /* Get new blob SHA */
    error = gm_hook_get_blob_sha(repo, "HEAD", file_path, &new_oid);
    if (error != GM_OK) {
        /* File might be deleted, skip */
        return GM_OK;
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
int gm_hook_is_merge_commit(git_repository *repo, bool *is_merge) {
    git_reference *head_ref = NULL;
    git_commit *commit = NULL;
    int error;

    /* Get HEAD */
    error = git_repository_head(&head_ref, repo);
    if (error < 0) {
        return GM_ERR_UNKNOWN;
    }

    /* Get commit */
    git_oid oid;
    error = git_reference_name_to_id(&oid, repo, "HEAD");
    git_reference_free(head_ref);
    if (error < 0) {
        return GM_ERR_UNKNOWN;
    }

    error = git_commit_lookup(&commit, repo, &oid);
    if (error < 0) {
        return GM_ERR_UNKNOWN;
    }

    /* Check parent count */
    *is_merge = (git_commit_parentcount(commit) > 1);

    git_commit_free(commit);
    return GM_OK;
}
