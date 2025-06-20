/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "gitmind/attribution.h"
#include "gitmind/constants_internal.h"

/* Default author strings */
#define DEFAULT_AUTHOR_HUMAN   "user@local"
#define DEFAULT_AUTHOR_CLAUDE  "claude@anthropic"
#define DEFAULT_AUTHOR_GPT     "gpt@openai"
#define DEFAULT_AUTHOR_SYSTEM  "system@git-mind"
#define DEFAULT_AUTHOR_UNKNOWN "unknown@unknown"

/* Filter constants */
#define SOURCE_MASK_ALL        0xFFFFFFFF
#define CONFIDENCE_MIN_DEFAULT 0.0f
#define CONFIDENCE_MAX_DEFAULT 1.0f
#define CONFIDENCE_HALF_FLOAT_ONE 0x3C00

/* Source type strings */
#define SOURCE_STR_HUMAN   "human"
#define SOURCE_STR_CLAUDE  "claude"
#define SOURCE_STR_GPT     "gpt"
#define SOURCE_STR_SYSTEM  "system"

/**
 * Set default attribution based on source type
 */
int gm_attribution_set_default(gm_attribution_t *attr, gm_source_type_t source) {
    if (!attr) {
        return -1;
    }
    
    memset(attr, 0, sizeof(gm_attribution_t));
    attr->source_type = source;
    
    /* Set default author based on source */
    switch (source) {
        case GM_SOURCE_HUMAN:
            /* Try to get from git config */
            snprintf(attr->author, sizeof(attr->author), DEFAULT_AUTHOR_HUMAN);
            break;
            
        case GM_SOURCE_AI_CLAUDE:
            snprintf(attr->author, sizeof(attr->author), DEFAULT_AUTHOR_CLAUDE);
            break;
            
        case GM_SOURCE_AI_GPT:
            snprintf(attr->author, sizeof(attr->author), DEFAULT_AUTHOR_GPT);
            break;
            
        case GM_SOURCE_SYSTEM:
            snprintf(attr->author, sizeof(attr->author), DEFAULT_AUTHOR_SYSTEM);
            break;
            
        default:
            snprintf(attr->author, sizeof(attr->author), DEFAULT_AUTHOR_UNKNOWN);
            break;
    }
    
    return 0;
}

/**
 * Get attribution from environment variables
 */
int gm_attribution_from_env(gm_attribution_t *attr) {
    if (!attr) {
        return -1;
    }
    
    const char *source = getenv("GIT_MIND_SOURCE");
    const char *author = getenv("GIT_MIND_AUTHOR");
    const char *session = getenv("GIT_MIND_SESSION");
    
    /* Default to human if not specified */
    gm_source_type_t source_type = GM_SOURCE_HUMAN;
    
    if (source) {
        if (strcmp(source, SOURCE_STR_HUMAN) == 0) {
            source_type = GM_SOURCE_HUMAN;
        } else if (strcmp(source, SOURCE_STR_CLAUDE) == 0) {
            source_type = GM_SOURCE_AI_CLAUDE;
        } else if (strcmp(source, SOURCE_STR_GPT) == 0) {
            source_type = GM_SOURCE_AI_GPT;
        } else if (strcmp(source, SOURCE_STR_SYSTEM) == 0) {
            source_type = GM_SOURCE_SYSTEM;
        }
    }
    
    gm_attribution_set_default(attr, source_type);
    
    /* Override with environment values if present */
    if (author) {
        strncpy(attr->author, author, sizeof(attr->author) - 1);
        attr->author[sizeof(attr->author) - 1] = '\0';
    }
    
    if (session) {
        strncpy(attr->session_id, session, sizeof(attr->session_id) - 1);
        attr->session_id[sizeof(attr->session_id) - 1] = '\0';
    }
    
    return 0;
}

/**
 * Initialize default filter (show everything)
 */
int gm_filter_init_default(gm_filter_t *filter) {
    if (!filter) {
        return -1;
    }
    
    memset(filter, 0, sizeof(gm_filter_t));
    filter->source_mask = SOURCE_MASK_ALL;  /* All sources */
    filter->min_confidence = CONFIDENCE_MIN_DEFAULT;
    filter->max_confidence = CONFIDENCE_MAX_DEFAULT;
    filter->lane = GM_LANE_DEFAULT;
    filter->flags_required = 0;
    filter->flags_excluded = GM_ATTR_REJECTED;
    
    return 0;
}

/**
 * Initialize human-only filter
 */
int gm_filter_init_human_only(gm_filter_t *filter) {
    if (!filter) {
        return -1;
    }
    
    gm_filter_init_default(filter);
    filter->source_mask = (1 << GM_SOURCE_HUMAN);
    
    return 0;
}

/**
 * Initialize AI insights filter
 */
int gm_filter_init_ai_insights(gm_filter_t *filter, float min_confidence) {
    if (!filter) {
        return -1;
    }
    
    gm_filter_init_default(filter);
    filter->source_mask = (1 << GM_SOURCE_AI_CLAUDE) | 
                         (1 << GM_SOURCE_AI_GPT) | 
                         (1 << GM_SOURCE_AI_OTHER);
    filter->min_confidence = min_confidence;
    filter->flags_excluded = GM_ATTR_REJECTED | GM_ATTR_PENDING;
    
    return 0;
}

/**
 * Check if edge matches filter criteria
 */
int gm_filter_match(const gm_filter_t *filter, const gm_edge_attributed_t *edge) {
    if (!filter || !edge) {
        return 0;
    }
    
    /* Check source type */
    if (!(filter->source_mask & (1 << edge->attribution.source_type))) {
        return 0;
    }
    
    /* Check confidence range */
    float confidence = (float)edge->confidence / CONFIDENCE_HALF_FLOAT_ONE;  /* Convert from half float */
    if (confidence < filter->min_confidence || confidence > filter->max_confidence) {
        return 0;
    }
    
    /* Check lane */
    if (filter->lane != GM_LANE_DEFAULT && filter->lane != edge->lane) {
        return 0;
    }
    
    /* Check required flags */
    if (filter->flags_required && 
        !(edge->attribution.flags & filter->flags_required)) {
        return 0;
    }
    
    /* Check excluded flags */
    if (edge->attribution.flags & filter->flags_excluded) {
        return 0;
    }
    
    return 1;
}