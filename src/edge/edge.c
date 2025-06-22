/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind.h"
#include "gitmind/constants_internal.h"
#include "../util/gm_mem.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

/* Half-precision float constants */
#define HALF_FLOAT_ONE 0x3C00 /* 1.0 in IEEE-754 half */
#define DEFAULT_CONFIDENCE 100 /* Default confidence percentage */


/* Get current timestamp in milliseconds */
static uint64_t get_timestamp(gm_context_t *ctx) {
    struct timespec time_spec;
    if (ctx && ctx->time_ops && ctx->time_ops->clock_gettime) {
        ctx->time_ops->clock_gettime(CLOCK_REALTIME, &time_spec);
    } else {
        clock_gettime(CLOCK_REALTIME, &time_spec);
    }
    return (uint64_t)time_spec.tv_sec * MILLIS_PER_SECOND +
           time_spec.tv_nsec / NANOS_PER_MILLI;
}

/* Resolve SHA for a path */
static int resolve_sha(gm_context_t *ctx, const char *path, uint8_t *sha) {
    return gm_sha_from_path(ctx, path, sha);
}

/* Initialize edge with default values */
static void init_edge_defaults(gm_edge_t *edge) {
    /* Zero initialize */
    gm_memset(edge, 0, sizeof(gm_edge_t));
    
    /* Set defaults */
    edge->confidence = DEFAULT_CONFIDENCE;
}

/* Populate edge fields */
static int populate_edge(gm_edge_t *edge, gm_context_t *ctx,
                         const char *src_path, const char *tgt_path,
                         gm_rel_type_t rel_type) {
    int result;
    
    /* Resolve SHAs */
    result = resolve_sha(ctx, src_path, edge->src_sha);
    if (result != GM_OK) {
        return result;
    }
    result = resolve_sha(ctx, tgt_path, edge->tgt_sha);
    if (result != GM_OK) {
        return result;
    }
    
    /* Set fields */
    gm_strlcpy(edge->src_path, src_path, GM_PATH_MAX);
    gm_strlcpy(edge->tgt_path, tgt_path, GM_PATH_MAX);
    edge->rel_type = rel_type;
    edge->timestamp = get_timestamp(ctx) / MILLIS_PER_SECOND;
    
    return gm_ulid_generate(edge->ulid);
}

/* Create edge from paths */
int gm_edge_create(gm_context_t *ctx, const char *src_path,
                   const char *tgt_path, gm_rel_type_t rel_type,
                   gm_edge_t *edge) {
    if (!ctx || !src_path || !tgt_path || !edge) {
        return GM_INVALID_ARG;
    }
    init_edge_defaults(edge);
    return populate_edge(edge, ctx, src_path, tgt_path, rel_type);
}

/* Compare SHA arrays */
static int sha_equal(const uint8_t *sha1, const uint8_t *sha2) {
    size_t iter;
    for (iter = 0; iter < GM_SHA1_SIZE; iter++) {
        if (sha1[iter] != sha2[iter]) {
            return 0;
        }
    }
    return 1;
}

/* Compare edges for equality */
int gm_edge_equal(const gm_edge_t *edge_a, const gm_edge_t *edge_b) {
    if (!edge_a || !edge_b) {
        return 0;
    }
    return sha_equal(edge_a->src_sha, edge_b->src_sha) &&
           sha_equal(edge_a->tgt_sha, edge_b->tgt_sha) &&
           edge_a->rel_type == edge_b->rel_type;
}

/* Get relationship type string */
static const char *get_rel_type_string(gm_rel_type_t rel_type) {
    switch (rel_type) {
    case GM_REL_IMPLEMENTS:
        return "IMPLEMENTS";
    case GM_REL_REFERENCES:
        return "REFERENCES";
    case GM_REL_DEPENDS_ON:
        return "DEPENDS_ON";
    case GM_REL_AUGMENTS:
        return "AUGMENTS";
    default:
        return "CUSTOM";
    }
}


/* Build format string safely */
static int build_format_string(char *buffer, size_t len, const char *type_str,
                               const char *src, const char *tgt) {
    int written = gm_snprintf(buffer, len, "%s: %s -> %s", 
                              type_str, src, tgt);
    return (written > 0 && written < (int)len) ? written : -1;
}

/* Format edge for display */
int gm_edge_format(const gm_edge_t *edge, char *buffer, size_t len) {
    if (!edge || !buffer || len == 0) {
        return GM_INVALID_ARG;
    }

    /* Get type string */
    const char *type_str = get_rel_type_string(edge->rel_type);

    /* Build formatted string */
    int result = build_format_string(buffer, len, type_str, 
                                     edge->src_path, edge->tgt_path);
    
    return (result > 0) ? GM_OK : GM_ERROR;
}