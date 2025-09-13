/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GM_AUGMENT_H
#define GM_AUGMENT_H

#include "gitmind/context.h"
#include "gitmind/edge.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <git2/types.h>
#include <git2/oid.h>
#include <git2/repository.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Constants */

#ifdef __cplusplus
}
#endif
#define MAX_CHANGED_FILES 50 /* Skip if more files changed */
#define LOOKBACK_LIMIT 200   /* Max edges to scan */
#define HOOK_TIMEOUT_MS 500  /* Abort if taking too long */
#define MAX_PATH_LENGTH 4096 /* Maximum file path length */

/* Get blob SHA for a file at a specific commit
 *
 * @param repo      Git repository
 * @param commit_ref Reference to commit (e.g., "HEAD~1")
 * @param file_path Path to file relative to repo root
 * @param sha_out   Output buffer for 20-byte SHA-1
 * @return          0 on success, error code otherwise
 */
int get_blob_sha(git_repository *repo, const char *commit_ref,
                 const char *file_path, uint8_t *sha_out);

/* Find recent edges with given source blob
 *
 * @param ctx       git-mind context
 * @param src_sha   Source blob SHA to search for
 * @param edges_out Output array of matching edges (caller must free)
 * @param count_out Number of edges found
 * @return          0 on success, error code otherwise
 */
int find_edges_by_source(gm_context_t *ctx, const uint8_t *src_sha,
                         gm_edge_t **edges_out, size_t *count_out);

/* Create AUGMENTS edge between two blob versions
 *
 * @param ctx       git-mind context
 * @param old_sha   Old blob SHA (source)
 * @param new_sha   New blob SHA (target)
 * @param file_path Path to file (for human readability)
 * @return          0 on success, error code otherwise
 */
int create_augments_edge(gm_context_t *ctx, const uint8_t *old_sha,
                         const uint8_t *new_sha, const char *file_path);

/* Process a single changed file
 *
 * @param ctx       git-mind context
 * @param repo      Git repository
 * @param file_path Path to changed file
 * @return          0 on success, error code otherwise
 */
int process_changed_file(gm_context_t *ctx, git_repository *repo,
                         const char *file_path);

/* Check if this is a merge commit
 *
 * @param repo      Git repository
 * @param is_merge  Output: true if merge commit
 * @return          0 on success, error code otherwise
 */
int is_merge_commit(git_repository *repo, bool *is_merge);

#endif /* GM_AUGMENT_H */
