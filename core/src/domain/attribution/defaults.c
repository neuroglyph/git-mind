/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/attribution/internal/defaults.h"

#include "gitmind/error.h"
#include "gitmind/util/memory.h"

static const char *author_for_source(gm_source_type_t source) {
    switch (source) {
    case GM_SOURCE_HUMAN:
        return "user@local";
    case GM_SOURCE_AI_CLAUDE:
        return "claude@anthropic";
    case GM_SOURCE_AI_GPT:
        return "gpt@openai";
    case GM_SOURCE_SYSTEM:
        return "system@git-mind";
    case GM_SOURCE_AI_OTHER:
        return "ai@unknown";
    case GM_SOURCE_IMPORT:
        return "import@git-mind";
    case GM_SOURCE_UNKNOWN:
    default:
        return "unknown@unknown";
    }
}

gm_result_void_t gm_attribution_defaults_apply(gm_attribution_t *attr,
                                                gm_source_type_t source) {
    if (attr == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "attr must not be null"));
    }

    *attr = (gm_attribution_t){0};
    attr->source_type = source;

    const char *author = author_for_source(source);
    if (gm_strcpy_safe(attr->author, sizeof attr->author, author) != 0) {
        return gm_err_void(
            GM_ERROR(GM_ERR_BUFFER_TOO_SMALL, "author default truncated for source %d", source));
    }

    return gm_ok_void();
}
