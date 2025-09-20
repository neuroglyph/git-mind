/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/attribution/internal/env_loader.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>

#include "gitmind/attribution/internal/defaults.h"
#include "gitmind/error.h"
#include "gitmind/ports/env_port.h"
#include "gitmind/util/memory.h"

static void test_defaults_human(void) {
    gm_attribution_t attr;
    gm_result_void_t result = gm_attribution_defaults_apply(&attr, GM_SOURCE_HUMAN);
    assert(result.ok);
    assert(attr.source_type == GM_SOURCE_HUMAN);
    assert(strcmp(attr.author, "user@local") == 0);
    assert(attr.session_id[0] == '\0');
}

typedef struct fake_env_ctx {
    const char *source;
    const char *author;
    const char *session;
    bool fail;
} fake_env_ctx_t;

static gm_result_bool_t fake_env_get(void *ctx, const char *key, char *buffer,
                                     size_t buffer_size) {
    fake_env_ctx_t *env = ctx;
    if (env->fail) {
        return gm_err_bool(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "forced failure"));
    }

    const char *value = NULL;
    if (strcmp(key, "GIT_MIND_SOURCE") == 0) {
        value = env->source;
    } else if (strcmp(key, "GIT_MIND_AUTHOR") == 0) {
        value = env->author;
    } else if (strcmp(key, "GIT_MIND_SESSION") == 0) {
        value = env->session;
    }

    if (value == NULL) {
        buffer[0] = '\0';
        return gm_ok_bool(false);
    }

    assert(buffer_size > strlen(value));
    (void)gm_strcpy_safe(buffer, buffer_size, value);
    return gm_ok_bool(true);
}

static void test_env_loader_overrides(void) {
    fake_env_ctx_t ctx = {
        .source = "claude",
        .author = "claude@example.com",
        .session = "session-123",
        .fail = false,
    };

    const gm_env_port_vtbl_t vtbl = {
        .get = fake_env_get,
    };
    const gm_env_port_t port = {
        .context = &ctx,
        .vtbl = &vtbl,
    };

    gm_attribution_t attr;
    gm_result_void_t result = gm_attribution_from_env_with_port(&attr, &port);
    assert(result.ok);
    assert(attr.source_type == GM_SOURCE_AI_CLAUDE);
    assert(strcmp(attr.author, ctx.author) == 0);
    assert(strcmp(attr.session_id, ctx.session) == 0);
}

static void test_env_loader_error_propagates(void) {
    fake_env_ctx_t ctx = {
        .source = NULL,
        .author = NULL,
        .session = NULL,
        .fail = true,
    };

    const gm_env_port_vtbl_t vtbl = {
        .get = fake_env_get,
    };
    const gm_env_port_t port = {
        .context = &ctx,
        .vtbl = &vtbl,
    };

    gm_attribution_t attr;
    gm_result_void_t result = gm_attribution_from_env_with_port(&attr, &port);
    assert(!result.ok);
    gm_error_free(result.u.err);
}

int main(void) {
    test_defaults_human();
    test_env_loader_overrides();
    test_env_loader_error_propagates();
    return 0;
}
