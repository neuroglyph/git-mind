/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/adapters/diagnostics/stderr_diagnostics_adapter.h"

#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "gitmind/error.h"
#include "gitmind/result.h"

typedef struct {
    bool emit_enabled;
} gm_stderr_diag_state_t;

static void emit_escaped(const char *value) {
    if (value == NULL) {
        fputs("(null)", stderr);
        return;
    }
    for (const char *p = value; *p != '\0'; ++p) {
        unsigned char ch = (unsigned char)*p;
        switch (ch) {
        case '\n':
            fputs("\\n", stderr);
            break;
        case '\r':
            fputs("\\r", stderr);
            break;
        case '\t':
            fputs("\\t", stderr);
            break;
        default:
            if (iscntrl(ch)) {
                fprintf(stderr, "\\x%02x", ch);
            } else {
                fputc(ch, stderr);
            }
            break;
        }
    }
}

static gm_result_void_t emit_impl(void *self, const char *component,
                                  const char *event, const gm_diag_kv_t *kvs,
                                  size_t kv_count) {
    gm_stderr_diag_state_t *state = (gm_stderr_diag_state_t *)self;
    if (state != NULL && !state->emit_enabled) {
        return gm_ok_void();
    }
    if (component == NULL) component = "";
    if (event == NULL) event = "";
    fputs("[diag] ", stderr);
    emit_escaped(component);
    fputc(' ', stderr);
    emit_escaped(event);
    for (size_t i = 0; i < kv_count; ++i) {
        fputc(' ', stderr);
        emit_escaped(kvs[i].key);
        fputc('=', stderr);
        emit_escaped(kvs[i].value);
    }
    fputc('\n', stderr);
    return gm_ok_void();
}

static void dispose_impl(void *self) {
    free(self);
}

static const gm_diagnostics_port_vtbl_t VTBL = {
    .emit = emit_impl,
    .dispose = dispose_impl,
};

gm_result_void_t gm_stderr_diagnostics_port_init(gm_diagnostics_port_t *port) {
    if (port == NULL) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, "stderr diagnostics requires out"));
    }
    gm_stderr_diag_state_t *st = (gm_stderr_diag_state_t *)calloc(1, sizeof(*st));
    if (st == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_OUT_OF_MEMORY, "alloc diag state failed"));
    }
    st->emit_enabled = true;
    port->vtbl = &VTBL;
    port->self = st;
    return gm_ok_void();
}

void gm_stderr_diagnostics_port_dispose(gm_diagnostics_port_t *port) {
    if (port == NULL) {
        return;
    }
    if (port->vtbl == &VTBL && port->self != NULL) {
        dispose_impl(port->self);
    } else if (port->vtbl != NULL && port->vtbl->dispose != NULL) {
        port->vtbl->dispose(port->self);
    }
    port->self = NULL;
    port->vtbl = NULL;
}
