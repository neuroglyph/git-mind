/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#define _POSIX_C_SOURCE 200809L

#include "gitmind.h"
#include "gitmind/constants_internal.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

/* Half-precision float constants */
#define HALF_FLOAT_ONE 0x3C00  /* 1.0 in IEEE-754 half */

/* Get current timestamp in milliseconds */
static uint64_t get_timestamp(gm_context_t *ctx) {
    struct timespec ts;
    if (ctx && ctx->time_ops && ctx->time_ops->clock_gettime) {
        ctx->time_ops->clock_gettime(CLOCK_REALTIME, &ts);
    } else {
        /* Fallback to direct call */
        clock_gettime(CLOCK_REALTIME, &ts);
    }
    return (uint64_t)ts.tv_sec * MILLIS_PER_SECOND + ts.tv_nsec / NANOS_PER_MILLI;
}

/* Create edge from paths */
int gm_edge_create(gm_context_t *ctx, const char *src_path, const char *tgt_path,
                   gm_rel_type_t rel_type, gm_edge_t *edge) {
    int result;
    
    if (!ctx || !src_path || !tgt_path || !edge) {
        return GM_INVALID_ARG;
    }
    
    /* Clear edge structure */
    memset(edge, 0, sizeof(gm_edge_t));
    
    /* Resolve source SHA */
    result = gm_sha_from_path(ctx, src_path, edge->src_sha);
    if (result != GM_OK) {
        return result;
    }
    
    /* Resolve target SHA */
    result = gm_sha_from_path(ctx, tgt_path, edge->tgt_sha);
    if (result != GM_OK) {
        return result;
    }
    
    /* Set paths */
    strncpy(edge->src_path, src_path, GM_PATH_MAX - 1);
    edge->src_path[GM_PATH_MAX - 1] = '\0';
    
    strncpy(edge->tgt_path, tgt_path, GM_PATH_MAX - 1);
    edge->tgt_path[GM_PATH_MAX - 1] = '\0';
    
    /* Set metadata */
    edge->rel_type = rel_type;
    edge->confidence = 100;  /* Default confidence as percentage */
    edge->timestamp = get_timestamp(ctx) / MILLIS_PER_SECOND;  /* Store as seconds, not millis */
    
    /* Generate ULID */
    result = gm_ulid_generate(edge->ulid);
    if (result != GM_OK) {
        return result;
    }
    
    return GM_OK;
}

/* Compare edges for equality */
int gm_edge_equal(const gm_edge_t *a, const gm_edge_t *b) {
    if (!a || !b) {
        return 0;
    }
    
    return memcmp(a->src_sha, b->src_sha, GM_SHA1_SIZE) == 0 &&
           memcmp(a->tgt_sha, b->tgt_sha, GM_SHA1_SIZE) == 0 &&
           a->rel_type == b->rel_type;
}

/* Get relationship type string */
static const char* get_rel_type_string(gm_rel_type_t rel_type) {
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

/* Format edge for display */
int gm_edge_format(const gm_edge_t *edge, char *buffer, size_t len) {
    if (!edge || !buffer || len == 0) {
        return GM_INVALID_ARG;
    }
    
    /* Format: "TYPE: src_path -> tgt_path" */
    const char *type_str = get_rel_type_string(edge->rel_type);
    
    int written = snprintf(buffer, len, "%s: %s -> %s",
                          type_str, edge->src_path, edge->tgt_path);
    
    return (written > 0 && written < (int)len) ? GM_OK : GM_ERROR;
}