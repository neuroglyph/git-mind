/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_JOURNAL_H
#define GITMIND_JOURNAL_H

#include "gitmind/edge.h"
#include "gitmind/edge_attributed.h"
#include "gitmind/context.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file journal.h
 * @brief Journal operations for reading and writing edges to Git storage
 */

/**
 * Callback function for reading basic edges
 *
 * @param edge The edge data
 * @param userdata User-provided data
 * @return 0 to continue iteration, non-zero to stop
 */
typedef int (*gm_journal_read_callback_t)(const gm_edge_t *edge, void *userdata);

/**
 * Callback function for reading attributed edges
 *
 * @param edge The attributed edge data
 * @param userdata User-provided data
 * @return 0 to continue iteration, non-zero to stop
 */
typedef int (*gm_journal_read_attributed_callback_t)(const gm_edge_attributed_t *edge, void *userdata);

/**
 * Read edges from journal for specified branch
 *
 * @param ctx Git context
 * @param branch Branch name (NULL for current branch)
 * @param callback Function to call for each edge
 * @param userdata User data passed to callback
 * @return 0 on success, error code on failure
 */
int gm_journal_read(gm_context_t *ctx, const char *branch,
                    gm_journal_read_callback_t callback, void *userdata);

/**
 * Read attributed edges from journal for specified branch
 *
 * @param ctx Git context
 * @param branch Branch name (NULL for current branch)
 * @param callback Function to call for each attributed edge
 * @param userdata User data passed to callback
 * @return 0 on success, error code on failure
 */
int gm_journal_read_attributed(gm_context_t *ctx, const char *branch,
                              gm_journal_read_attributed_callback_t callback, void *userdata);

/**
 * Append edges to journal
 *
 * @param ctx Git context
 * @param edges Array of edges to append
 * @param count Number of edges
 * @return 0 on success, error code on failure
 */
int gm_journal_append(gm_context_t *ctx, const gm_edge_t *edges, size_t count);

/**
 * Append attributed edges to journal
 *
 * @param ctx Git context
 * @param edges Array of attributed edges to append
 * @param count Number of edges
 * @return 0 on success, error code on failure
 */
int gm_journal_append_attributed(gm_context_t *ctx, const gm_edge_attributed_t *edges, size_t count);

/**
 * Create a Git commit with journal data
 *
 * @param ctx Git context
 * @param ref Reference name to update
 * @param data CBOR-encoded journal data
 * @param len Length of data
 * @return 0 on success, error code on failure
 */
int gm_journal_create_commit(gm_context_t *ctx, const char *ref, const void *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_JOURNAL_H */
