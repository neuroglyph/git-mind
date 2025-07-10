/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/attribution.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Default author strings */
#define DEFAULT_AUTHOR_HUMAN "user@local"
#define DEFAULT_AUTHOR_CLAUDE "claude@anthropic"
#define DEFAULT_AUTHOR_GPT "gpt@openai"
#define DEFAULT_AUTHOR_SYSTEM "system@git-mind"
#define DEFAULT_AUTHOR_UNKNOWN "unknown@unknown"

/* Filter constants */
#define SOURCE_MASK_ALL 0xFFFFFFFF
#define CONFIDENCE_MIN_DEFAULT 0.0f
#define CONFIDENCE_MAX_DEFAULT 1.0f
#define CONFIDENCE_HALF_FLOAT_ONE 0x3C00

/* Source type strings */
#define SOURCE_STR_HUMAN "human"
#define SOURCE_STR_CLAUDE "claude"
#define SOURCE_STR_GPT "gpt"
#define SOURCE_STR_SYSTEM "system"

/**
 * Set default attribution based on source type
 */
int gm_attribution_set_default(gm_attribution_t *attr,
                               gm_source_type_t source) {
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

