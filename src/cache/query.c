/* SPDX-License-Identifier: Apache-2.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind.h"

/* Placeholder - will implement cache queries later */
int gm_cache_query(gm_context_t *ctx, const uint8_t *sha, 
                   gm_edge_t **edges, size_t *n_edges) {
    (void)ctx;
    (void)sha;
    (void)edges;
    (void)n_edges;
    return GM_NOT_FOUND;  /* Fall back to journal scan */
}