/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_ATTRIBUTION_H
#define GITMIND_ATTRIBUTION_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file attribution.h
 * @brief Attribution metadata for edge tracking
 */

/* Buffer sizes for attribution fields */
#define GM_ATTRIBUTION_AUTHOR_SIZE 64
#define GM_ATTRIBUTION_SESSION_ID_SIZE 32

/* Source types for edge attribution */
typedef enum {
    GM_SOURCE_HUMAN = 0,     /**< Human-created edge */
    GM_SOURCE_AI_CLAUDE = 1, /**< Claude AI via MCP */
    GM_SOURCE_AI_GPT = 2,    /**< GPT-4 or similar */
    GM_SOURCE_AI_OTHER = 3,  /**< Other AI systems */
    GM_SOURCE_SYSTEM = 4,    /**< System-generated (e.g., AUGMENTS) */
    GM_SOURCE_IMPORT = 5,    /**< Imported from external source */
    GM_SOURCE_UNKNOWN = 255  /**< Unknown source */
} gm_source_type_t;

/* Attribution metadata */
typedef struct {
    gm_source_type_t source_type;                   /**< Who created this edge */
    char author[GM_ATTRIBUTION_AUTHOR_SIZE];        /**< Email or identifier */
    char session_id[GM_ATTRIBUTION_SESSION_ID_SIZE]; /**< Session/conversation ID */
    uint32_t flags;                                 /**< Future expansion */
} gm_attribution_t;

/**
 * Set default attribution based on source type
 *
 * @param attr Attribution structure to initialize
 * @param source Source type (human, AI, etc.)
 * @return 0 on success, -1 on error
 */
int gm_attribution_set_default(gm_attribution_t *attr, gm_source_type_t source);

/**
 * Get attribution from environment variables
 *
 * @param attr Attribution structure to populate
 * @return 0 on success, -1 on error
 */
int gm_attribution_from_env(gm_attribution_t *attr);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_ATTRIBUTION_H */
