/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/adapters/diagnostics/stderr_diagnostics_adapter.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/ports/diagnostic_port.h"
#include "gitmind/security/string.h"

typedef struct {
    bool emit_enabled;
} gm_stderr_diag_state_t;

enum {
    HexEscapeBufferLength = 5,
};

static void emit_escaped(const char *value) {
    if (value == NULL) {
        (void)fputs("(null)", stderr);
        return;
    }
    for (const char *cursor = value; *cursor != '\0'; ++cursor) {
        unsigned char ch_value = (unsigned char)*cursor;
        switch (ch_value) {
        case '\n':
            (void)fputs("\\n", stderr);
            break;
        case '\r':
            (void)fputs("\\r", stderr);
            break;
        case '\t':
            (void)fputs("\\t", stderr);
            break;
        default:
            if (iscntrl(ch_value)) {
                char escape_buffer[HexEscapeBufferLength];
                int hex_written = gm_snprintf(escape_buffer,
                                              sizeof(escape_buffer),
                                              "\\x%02x",
                                              ch_value);
                if (hex_written > 0 &&
                    (size_t)hex_written < sizeof(escape_buffer)) {
                    (void)fputs(escape_buffer, stderr);
                }
            } else {
                (void)fputc(ch_value, stderr);
            }
            break;
        }
    }
}

static gm_result_void_t emit_impl(void *self, const char *component,
                                  const char *event, const gm_diag_kv_t *kvs,
                                  size_t kv_count) {
    if (kv_count > 0U && kvs == NULL) {
        return gm_ok_void();
    }
    gm_stderr_diag_state_t *state = (gm_stderr_diag_state_t *)self;
    if (state != NULL && !state->emit_enabled) {
        return gm_ok_void();
    }
    if (component == NULL) {
        component = "";
    }
    if (event == NULL) {
        event = "";
    }
    (void)fputs("[diag] ", stderr);
    emit_escaped(component);
    (void)fputc(' ', stderr);
    emit_escaped(event);
    for (size_t i = 0; i < kv_count; ++i) {
        (void)fputc(' ', stderr);
        emit_escaped(kvs[i].key);
        (void)fputc('=', stderr);
        emit_escaped(kvs[i].value);
    }
    (void)fputc('\n', stderr);
    return gm_ok_void();
}

static void dispose_impl(void *self) {
    free(self);
}

static const gm_diagnostics_port_vtbl_t GmStderrDiagnosticsPortVtbl = {
    .emit = emit_impl,
    .dispose = dispose_impl,
};

gm_result_void_t gm_stderr_diagnostics_port_init(gm_diagnostics_port_t *port) {
    if (port == NULL) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, "stderr diagnostics requires out"));
    }
    gm_stderr_diag_state_t *state =
        (gm_stderr_diag_state_t *)calloc(1, sizeof(*state));
    if (state == NULL) {
        return gm_err_void(
            GM_ERROR(GM_ERR_OUT_OF_MEMORY, "alloc diag state failed"));
    }
    state->emit_enabled = true;
    port->vtbl = &GmStderrDiagnosticsPortVtbl;
    port->self = state;
    return gm_ok_void();
}

void gm_stderr_diagnostics_port_dispose(gm_diagnostics_port_t *port) {
    if (port == NULL) {
        return;
    }
    if (port->vtbl == &GmStderrDiagnosticsPortVtbl && port->self != NULL) {
        dispose_impl(port->self);
    } else if (port->vtbl != NULL && port->vtbl->dispose != NULL) {
        port->vtbl->dispose(port->self);
    }
    port->self = NULL;
    port->vtbl = NULL;
}
