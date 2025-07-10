/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/attribution.h"

#define __STDC_WANT_LIB_EXT1__ 1
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Secure string functions - use Annex K where available, fallback otherwise */
#ifdef __STDC_LIB_EXT1__
#define HAVE_SECURE_STRINGS 1
#else
#define HAVE_SECURE_STRINGS 0

/* Secure fallback implementations */
static inline int strcpy_s_fallback(char *dest, size_t dest_size, const char *src) {
    if (!dest || !src || dest_size == 0) return 1;
    size_t len = strlen(src);
    if (len >= dest_size) return 1;
    memcpy(dest, src, len + 1);
    return 0;
}

static inline int strncpy_s_fallback(char *dest, size_t dest_size, const char *src, size_t count) {
    if (!dest || !src || dest_size == 0) return 1;
    size_t copy_len = strlen(src);
    if (copy_len > count) copy_len = count;
    if (copy_len >= dest_size) copy_len = dest_size - 1;
    memcpy(dest, src, copy_len);
    dest[copy_len] = '\0';
    return 0;
}

#define strcpy_s(dest, dest_size, src) strcpy_s_fallback(dest, dest_size, src)
#define strncpy_s(dest, dest_size, src, count) strncpy_s_fallback(dest, dest_size, src, count)
#endif

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

    /* Zero-initialize structure */
    *attr = (gm_attribution_t){0};
    attr->source_type = source;

    /* Set default author based on source */
    switch (source) {
    case GM_SOURCE_HUMAN:
        /* Try to get from git config */
        (void)strcpy_s(attr->author, sizeof(attr->author), DEFAULT_AUTHOR_HUMAN);
        break;

    case GM_SOURCE_AI_CLAUDE:
        (void)strcpy_s(attr->author, sizeof(attr->author), DEFAULT_AUTHOR_CLAUDE);
        break;

    case GM_SOURCE_AI_GPT:
        (void)strcpy_s(attr->author, sizeof(attr->author), DEFAULT_AUTHOR_GPT);
        break;

    case GM_SOURCE_SYSTEM:
        (void)strcpy_s(attr->author, sizeof(attr->author), DEFAULT_AUTHOR_SYSTEM);
        break;

    default:
        (void)strcpy_s(attr->author, sizeof(attr->author), DEFAULT_AUTHOR_UNKNOWN);
        break;
    }

    return 0;
}

/**
 * Parse source type from string
 */
static gm_source_type_t parse_source_type(const char *source) {
    if (!source) {
        return GM_SOURCE_HUMAN;
    }
    
    if (strcmp(source, SOURCE_STR_HUMAN) == 0) {
        return GM_SOURCE_HUMAN;
    }
    if (strcmp(source, SOURCE_STR_CLAUDE) == 0) {
        return GM_SOURCE_AI_CLAUDE;
    }
    if (strcmp(source, SOURCE_STR_GPT) == 0) {
        return GM_SOURCE_AI_GPT;
    }
    if (strcmp(source, SOURCE_STR_SYSTEM) == 0) {
        return GM_SOURCE_SYSTEM;
    }
    
    return GM_SOURCE_HUMAN; /* Default */
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

    gm_source_type_t source_type = parse_source_type(source);
    gm_attribution_set_default(attr, source_type);

    /* Override with environment values if present */
    if (author) {
        (void)strncpy_s(attr->author, sizeof(attr->author), author, strlen(author));
    }

    if (session) {
        (void)strncpy_s(attr->session_id, sizeof(attr->session_id), session, strlen(session));
    }

    return 0;
}

