/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_ATTRIBUTION_H
#define GITMIND_ATTRIBUTION_H

#include <stddef.h>
#include <stdint.h>
/* Forward declaration to avoid header cycles */
typedef struct gm_edge_attributed gm_edge_attributed_t;

#ifdef __cplusplus
extern "C" {
#endif

/* Source types for edge attribution */
typedef enum {
    GM_SOURCE_HUMAN = 0,     /* Human-created edge */
    GM_SOURCE_AI_CLAUDE = 1, /* Claude AI via MCP */
    GM_SOURCE_AI_GPT = 2,    /* GPT-4 or similar */
    GM_SOURCE_AI_OTHER = 3,  /* Other AI systems */
    GM_SOURCE_SYSTEM = 4,    /* System-generated (e.g., AUGMENTS) */
    GM_SOURCE_IMPORT = 5,    /* Imported from external source */
    GM_SOURCE_UNKNOWN = 255  /* Unknown source */
} gm_source_type_t;

/* Attribution metadata */
typedef struct {
    gm_source_type_t source_type; /* Who created this edge */
    char author[64];              /* Email or identifier */
    char session_id[32];          /* Session/conversation ID */
    uint32_t flags;               /* Future expansion */
} gm_attribution_t;

/* Attribution flags */
#define GM_ATTR_REVIEWED 0x0001  /* Human reviewed AI suggestion */
#define GM_ATTR_ACCEPTED 0x0002  /* AI suggestion accepted */
#define GM_ATTR_REJECTED 0x0004  /* AI suggestion rejected */
#define GM_ATTR_MODIFIED 0x0008  /* AI suggestion modified */
#define GM_ATTR_CONSENSUS 0x0010 /* Human and AI agree */
#define GM_ATTR_CONFLICT 0x0020  /* Human and AI disagree */
#define GM_ATTR_PENDING 0x0040   /* Awaiting review */

/* Lane types for organizing edges */
typedef enum {
    GM_LANE_DEFAULT = 0,      /* Default lane */
    GM_LANE_ARCHITECTURE = 1, /* Architecture documentation */
    GM_LANE_TESTING = 2,      /* Test coverage */
    GM_LANE_REFACTOR = 3,     /* Refactoring relationships */
    GM_LANE_ANALYSIS = 4,     /* AI analysis results */
    GM_LANE_CUSTOM = 100      /* User-defined lanes */
} gm_lane_type_t;

/* Filter criteria for edge queries */
typedef struct {
    gm_source_type_t source_mask; /* Bitmask of sources to include */
    float min_confidence;         /* Minimum confidence (0.0-1.0) */
    float max_confidence;         /* Maximum confidence (0.0-1.0) */
    gm_lane_type_t lane;          /* Specific lane or GM_LANE_DEFAULT for all */
    uint32_t flags_required;      /* Must have these flags */
    uint32_t flags_excluded;      /* Must not have these flags */
} gm_filter_t;

/* Edge type is declared in gitmind/edge_attributed.h */

/* Attribution functions */

/**
 * Set default attribution based on source type.
 *
 * @param attr Attribution structure to initialize
 * @param source Source type (human, AI, etc.)
 * @return GM_OK on success
 */
int gm_attribution_set_default(gm_attribution_t *attr, gm_source_type_t source);

/**
 * Read attribution from environment variables.
 *
 * Reads GIT_MIND_SOURCE, GIT_MIND_AUTHOR, GIT_MIND_SESSION.
 *
 * @param attr Attribution structure to populate
 * @return GM_OK on success, GM_NOT_FOUND if no env vars
 */
int gm_attribution_from_env(gm_attribution_t *attr);

/**
 * Encode attribution to binary format.
 *
 * @param attr Attribution to encode
 * @param buffer Output buffer
 * @param len In: buffer size, Out: bytes written
 * @return GM_OK on success
 */
int gm_attribution_encode(const gm_attribution_t *attr, uint8_t *buffer,
                          size_t *len);

/**
 * Decode attribution from binary format.
 *
 * @param buffer Binary data
 * @param len Buffer length
 * @param attr Output attribution
 * @return GM_OK on success
 */
int gm_attribution_decode(const uint8_t *buffer, size_t len,
                          gm_attribution_t *attr);

/* Filter functions */

/**
 * Initialize filter with default settings (show all).
 *
 * @param filter Filter to initialize
 * @return GM_OK on success
 */
int gm_filter_init_default(gm_filter_t *filter);

/**
 * Initialize filter to show human edges only.
 *
 * @param filter Filter to initialize
 * @return GM_OK on success
 */
int gm_filter_init_human_only(gm_filter_t *filter);

/**
 * Initialize filter for AI insights above confidence threshold.
 *
 * @param filter Filter to initialize
 * @param min_confidence Minimum confidence (0.0 to 1.0)
 * @return GM_OK on success
 */
int gm_filter_init_ai_insights(gm_filter_t *filter, float min_confidence);

/**
 * Check if edge matches filter criteria.
 *
 * @param filter Filter to apply
 * @param edge Edge to test
 * @return 1 if matches, 0 if filtered out
 */
int gm_filter_match(const gm_filter_t *filter,
                    const gm_edge_attributed_t *edge);

/* CBOR encode/decode for attributed edges live in gitmind/edge_attributed.h */

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_ATTRIBUTION_H */
