/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_TYPES_ID_H
#define GITMIND_TYPES_ID_H

#include "gitmind/result.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Forward declaration */
typedef struct gm_crypto_context gm_crypto_context_t;

/* ID size constants */
#define GM_ID_SIZE 32      /* SHA-256 digest size in bytes */
#define GM_ID_HEX_SIZE 65  /* 32 bytes * 2 + null terminator */
#define GM_ID_HEX_CHARS 64 /* 32 bytes * 2 hex chars per byte */

/**
 * @brief 256-bit identifier (SHA-256 based)
 *
 * Stored in big-endian format for consistent sorting
 */
typedef struct gm_id {
    uint8_t bytes[GM_ID_SIZE]; /* Big-endian SHA-256 digest (network order, as
                                  printed hex) */
} gm_id_t;

/* Define result type for ID operations */
GM_RESULT_DEF(gm_result_id, gm_id_t);

/* ID operations */
bool gm_id_equal(gm_id_t id_a, gm_id_t id_b);
int gm_id_compare(gm_id_t id_a, gm_id_t id_b);
gm_result_u32_t gm_id_hash_with_context(const gm_crypto_context_t *ctx, gm_id_t new_identifier);

/* ID creation - all can fail */
gm_result_id_t gm_id_from_data_with_context(const gm_crypto_context_t *ctx, const void *data, size_t len);
gm_result_id_t gm_id_from_string_with_context(const gm_crypto_context_t *ctx, const char *str);
gm_result_id_t gm_id_generate_with_context(const gm_crypto_context_t *ctx); /* Random ID */

/* ID conversion (safe - validates buffer size) */
gm_result_void_t gm_id_to_hex(gm_id_t new_identifier, char *out,
                              size_t out_size);

/* ID parsing (can fail) */
gm_result_id_t gm_id_from_hex(const char *hex);

/**
 * @brief Strongly typed IDs prevent mixing different ID types
 *
 * Each type is distinct - you cannot accidentally pass a node_id
 * where an edge_id is expected.
 */
typedef struct {
    gm_id_t base;
} gm_node_id_t;
typedef struct {
    gm_id_t base;
} gm_edge_id_t;
typedef struct {
    gm_id_t base;
} gm_graph_id_t;
typedef struct {
    gm_id_t base;
} gm_session_id_t;
typedef struct {
    gm_id_t base;
} gm_commit_id_t;

/* Type-safe operations (compiler enforced!) */
static inline bool gm_node_id_equal(gm_node_id_t node_a, gm_node_id_t node_b) {
    return gm_id_equal(node_a.base, node_b.base);
}

static inline bool gm_edge_id_equal(gm_edge_id_t edge_a, gm_edge_id_t edge_b) {
    return gm_id_equal(edge_a.base, edge_b.base);
}

static inline bool gm_graph_id_equal(gm_graph_id_t graph_a,
                                     gm_graph_id_t graph_b) {
    return gm_id_equal(graph_a.base, graph_b.base);
}

static inline bool gm_session_id_equal(gm_session_id_t session_a,
                                       gm_session_id_t session_b) {
    return gm_id_equal(session_a.base, session_b.base);
}

static inline bool gm_commit_id_equal(gm_commit_id_t commit_a,
                                      gm_commit_id_t commit_b) {
    return gm_id_equal(commit_a.base, commit_b.base);
}

/* Forward declarations for typed ID generation */
typedef struct gm_path gm_path_t;
typedef struct gm_edge_type gm_edge_type_t;

/* Node ID: SHA256(path) */
gm_node_id_t gm_node_id_from_path(gm_path_t path);

/* Define result type for session ID */
GM_RESULT_DEF(gm_result_session_id, gm_session_id_t);

/* Edge ID: SHA256(source || target || type) */
gm_edge_id_t gm_edge_id_from_triple(gm_node_id_t source, gm_node_id_t target,
                                    gm_edge_type_t type);

/* Session ID: Random UUID v4 */
gm_result_session_id_t gm_session_id_new_with_context(const gm_crypto_context_t *ctx);

/* Conversion helpers */
static inline gm_result_void_t gm_node_id_to_hex(gm_node_id_t identifier,
                                                 char out[GM_ID_HEX_SIZE]) {
    return gm_id_to_hex(identifier.base, out, GM_ID_HEX_SIZE);
}

static inline gm_result_void_t gm_edge_id_to_hex(gm_edge_id_t identifier,
                                                 char out[GM_ID_HEX_SIZE]) {
    return gm_id_to_hex(identifier.base, out, GM_ID_HEX_SIZE);
}

#endif /* GITMIND_TYPES_ID_H */