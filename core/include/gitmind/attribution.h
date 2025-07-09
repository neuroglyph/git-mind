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
    gm_source_type_t source_type; /**< Who created this edge */
    char author[64];              /**< Email or identifier */
    char session_id[32];          /**< Session/conversation ID */
    uint32_t flags;               /**< Future expansion */
} gm_attribution_t;

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_ATTRIBUTION_H */
