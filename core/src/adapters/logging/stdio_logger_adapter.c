/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/adapters/logging/stdio_logger_adapter.h"

#include "gitmind/error.h"
#include "gitmind/result.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
    FILE *stream;
    gm_log_level_t min_level;
} stdio_logger_state_t;

static const char *level_name(gm_log_level_t level) {
    switch (level) {
    case GM_LOG_DEBUG: return "DEBUG";
    case GM_LOG_INFO:  return "INFO";
    case GM_LOG_WARN:  return "WARN";
    case GM_LOG_ERROR: return "ERROR";
    default: return "LOG";
    }
}

static gm_result_void_t log_impl(void *self, gm_log_level_t level,
                                 const char *component,
                                 const char *message) {
    stdio_logger_state_t *state = (stdio_logger_state_t *)self;
    if (state == NULL || state->stream == NULL) {
        return gm_ok_void();
    }
    if (level < state->min_level) {
        return gm_ok_void();
    }
    time_t now = time(NULL);
    struct tm tm;
#if defined(_WIN32)
    gmtime_s(&tm, &now);
#else
    gmtime_r(&now, &tm);
#endif
    char ts[32];
    if (strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%SZ", &tm) == 0) {
        ts[0] = '\0';
    }
    fprintf(state->stream, "%s [%s] %s: %s\n", ts, level_name(level),
            component ? component : "core", message ? message : "");
    return gm_ok_void();
}

static const gm_logger_port_vtbl_t STDIO_VTBL = {
    .log = log_impl,
};

GM_NODISCARD gm_result_void_t gm_stdio_logger_port_init(
    gm_logger_port_t *port, FILE *stream, gm_log_level_t min_level) {
    if (port == NULL || stream == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT, "logger requires stream"));
    }
    stdio_logger_state_t *state = (stdio_logger_state_t *)calloc(1, sizeof(*state));
    if (state == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_OUT_OF_MEMORY, "allocating logger state"));
    }
    state->stream = stream;
    state->min_level = min_level;
    port->vtbl = &STDIO_VTBL;
    port->self = state;
    return gm_ok_void();
}

void gm_stdio_logger_port_dispose(gm_logger_port_t *port) {
    if (port == NULL) return;
    free(port->self);
    port->self = NULL;
    port->vtbl = NULL;
}
