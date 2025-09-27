/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_AUGMENT_H
#define GITMIND_AUGMENT_H

#include "gitmind/context.h"
#include "gitmind/edge.h"
#include "gitmind/ports/git_repository_port.h"
#include "gitmind/types.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
// keep C linkage open until end
#endif

/* Constants (public, prefixed) */

#define GM_AUGMENT_MAX_CHANGED_FILES 50 /* Skip if more files changed */
#define GM_AUGMENT_LOOKBACK_LIMIT 200   /* Max edges to scan */
#define GM_AUGMENT_HOOK_TIMEOUT_MS 500  /* Abort if taking too long */
#define GM_AUGMENT_MAX_PATH_LENGTH 4096 /* Maximum file path length */

/* Backward-compat (deprecated): old macro names */
#define MAX_CHANGED_FILES GM_AUGMENT_MAX_CHANGED_FILES
#define LOOKBACK_LIMIT GM_AUGMENT_LOOKBACK_LIMIT
#define HOOK_TIMEOUT_MS GM_AUGMENT_HOOK_TIMEOUT_MS
#define MAX_PATH_LENGTH GM_AUGMENT_MAX_PATH_LENGTH

/* Get blob SHA for a file at a specific commit
 *
 * @param repo_port Repository port adapter
 * @param commit_ref Reference to commit (e.g., "HEAD~1")
 * @param file_path Path to file relative to repo root
 * @param sha_out   Output buffer for 20-byte SHA-1
 * @return          0 on success, error code otherwise
 */
int gm_hook_get_blob_sha(const gm_git_repository_port_t *repo_port,
                         const char *commit_ref,
                         const char *file_path, gm_oid_t *sha_out);

/* Find recent edges with given source blob
 *
 * @param ctx       git-mind context
 * @param src_sha   Source blob SHA to search for
 * @param edges_out Output array of matching edges (caller must free with free())
 * @param count_out Number of edges found
 * @return          0 on success, error code otherwise
 */
int gm_hook_find_edges_by_source(gm_context_t *ctx, const gm_oid_t *src_oid,
                                 gm_edge_t **edges_out, size_t *count_out);

/* Create AUGMENTS edge between two blob versions
 *
 * @param ctx       git-mind context
 * @param old_sha   Old blob SHA (source)
 * @param new_sha   New blob SHA (target)
 * @param file_path Path to file (for human readability)
 * @return          0 on success, error code otherwise
 */
int gm_hook_create_augments_edge(gm_context_t *ctx, const gm_oid_t *old_oid,
                                 const gm_oid_t *new_oid, const char *file_path);

/* Process a single changed file
 *
 * @param ctx       git-mind context
 * @param repo_port Repository port adapter
 * @param file_path Path to changed file
 * @return          0 on success, error code otherwise
 */
int gm_hook_process_changed_file(gm_context_t *ctx,
                                 const gm_git_repository_port_t *repo_port,
                                 const char *file_path);

/* Check if this is a merge commit
 *
 * @param repo      Git repository
 * @param is_merge  Output: true if merge commit
 * @return          0 on success, error code otherwise
 */
int gm_hook_is_merge_commit(const gm_git_repository_port_t *repo_port,
                            bool *is_merge);

/* Backward-compatibility helpers (deprecated) */
static inline int get_blob_sha(const gm_git_repository_port_t *repo_port,
                               const char *commit_ref, const char *file_path,
                               gm_oid_t *sha_out) {
    return gm_hook_get_blob_sha(repo_port, commit_ref, file_path, sha_out);
}

static inline int find_edges_by_source(gm_context_t *ctx, const gm_oid_t *src_oid,
                                       gm_edge_t **edges_out, size_t *count_out) {
    return gm_hook_find_edges_by_source(ctx, src_oid, edges_out, count_out);
}

static inline int create_augments_edge(gm_context_t *ctx, const gm_oid_t *old_oid,
                                       const gm_oid_t *new_oid,
                                       const char *file_path) {
    return gm_hook_create_augments_edge(ctx, old_oid, new_oid, file_path);
}

static inline int process_changed_file(gm_context_t *ctx,
                                       const gm_git_repository_port_t *repo_port,
                                       const char *file_path) {
    return gm_hook_process_changed_file(ctx, repo_port, file_path);
}

static inline int is_merge_commit(const gm_git_repository_port_t *repo_port,
                                  bool *is_merge) {
    return gm_hook_is_merge_commit(repo_port, is_merge);
}

/* Preferred aliases */
static inline int gm_augment_get_blob_sha(
    const gm_git_repository_port_t *repo_port, const char *commit_ref,
    const char *file_path, gm_oid_t *sha_out) {
    return gm_hook_get_blob_sha(repo_port, commit_ref, file_path, sha_out);
}

static inline int gm_augment_find_edges_by_source(gm_context_t *ctx,
                                                  const gm_oid_t *src_oid,
                                                  gm_edge_t **edges_out,
                                                  size_t *count_out) {
    return gm_hook_find_edges_by_source(ctx, src_oid, edges_out, count_out);
}

static inline int gm_augment_create_augments_edge(gm_context_t *ctx,
                                                  const gm_oid_t *old_oid,
                                                  const gm_oid_t *new_oid,
                                                  const char *file_path) {
    return gm_hook_create_augments_edge(ctx, old_oid, new_oid, file_path);
}

static inline int gm_augment_process_changed_file(
    gm_context_t *ctx, const gm_git_repository_port_t *repo_port,
    const char *file_path) {
    return gm_hook_process_changed_file(ctx, repo_port, file_path);
}

static inline int gm_augment_is_merge_commit(
    const gm_git_repository_port_t *repo_port, bool *is_merge) {
    return gm_hook_is_merge_commit(repo_port, is_merge);
}

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_AUGMENT_H */
