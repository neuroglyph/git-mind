/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/attribution.h"
#include "gitmind/attribution/internal/env_loader.h"

#include <string.h>

#include "gitmind/attribution/internal/defaults.h"
#include "gitmind/error.h"
#include "gitmind/util/memory.h"

static gm_source_type_t parse_source_type(const char *value) {
    if (value == NULL || value[0] == '\0') {
        return GM_SOURCE_HUMAN;
    }

    if (strcmp(value, "human") == 0) {
        return GM_SOURCE_HUMAN;
    }
    if (strcmp(value, "claude") == 0) {
        return GM_SOURCE_AI_CLAUDE;
    }
    if (strcmp(value, "gpt") == 0) {
        return GM_SOURCE_AI_GPT;
    }
    if (strcmp(value, "system") == 0) {
        return GM_SOURCE_SYSTEM;
    }
    if (strcmp(value, "ai") == 0 || strcmp(value, "other") == 0) {
        return GM_SOURCE_AI_OTHER;
    }
    if (strcmp(value, "import") == 0) {
        return GM_SOURCE_IMPORT;
    }

    return GM_SOURCE_UNKNOWN;
}

gm_result_void_t gm_attribution_from_env_with_port(gm_attribution_t *attr,
                                                    const gm_env_port_t *port) {
    if (attr == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "attr must not be null"));
    }
    if (port == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "env port must not be null"));
    }

    GM_TRY(gm_attribution_defaults_apply(attr, GM_SOURCE_HUMAN));

    enum { SOURCE_CAP = 16 };
    char source[SOURCE_CAP] = {0};
    gm_result_bool_t source_res = gm_env_get(port, "GIT_MIND_SOURCE", source, sizeof source);
    if (GM_IS_ERR(source_res)) {
        return gm_err_void(GM_UNWRAP_ERR(source_res));
    }
    if (GM_UNWRAP(source_res)) {
        gm_source_type_t parsed = parse_source_type(source);
        attr->source_type = parsed;
        GM_TRY(gm_attribution_defaults_apply(attr, parsed));
    }

    enum { AUTHOR_CAP = sizeof(((gm_attribution_t *)0)->author) };
    char author[AUTHOR_CAP] = {0};
    gm_result_bool_t author_res =
        gm_env_get(port, "GIT_MIND_AUTHOR", author, sizeof author);
    if (GM_IS_ERR(author_res)) {
        return gm_err_void(GM_UNWRAP_ERR(author_res));
    }
    if (GM_UNWRAP(author_res)) {
        if (gm_strcpy_safe(attr->author, sizeof attr->author, author) != 0) {
            return gm_err_void(
                GM_ERROR(GM_ERR_BUFFER_TOO_SMALL, "author env truncated (%s)", author));
        }
    }

    enum { SESSION_CAP = sizeof(((gm_attribution_t *)0)->session_id) };
    char session[SESSION_CAP] = {0};
    gm_result_bool_t session_res =
        gm_env_get(port, "GIT_MIND_SESSION", session, sizeof session);
    if (GM_IS_ERR(session_res)) {
        return gm_err_void(GM_UNWRAP_ERR(session_res));
    }
    if (GM_UNWRAP(session_res)) {
        if (gm_strcpy_safe(attr->session_id, sizeof attr->session_id, session) != 0) {
            return gm_err_void(
                GM_ERROR(GM_ERR_BUFFER_TOO_SMALL, "session env truncated (%s)", session));
        }
    }

    return gm_ok_void();
}
