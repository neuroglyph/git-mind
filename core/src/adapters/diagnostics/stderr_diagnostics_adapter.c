/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "stderr_diagnostics_adapter.h"

#include <stdio.h>
#include <stdlib.h>

#include "gitmind/error.h"
#include "gitmind/result.h"

typedef struct {
    int enabled; /* future toggle; unused for now */
} gm_stderr_diag_state_t;

static gm_result_void_t emit_impl(void *self, const char *component,
                                  const char *event, const gm_diag_kv_t *kvs,
                                  size_t kv_count) {
    (void)self;
    if (component == NULL) component = "";
    if (event == NULL) event = "";
    fputs("[diag] ", stderr);
    fputs(component, stderr);
    fputc(' ', stderr);
    fputs(event, stderr);
    for (size_t i = 0; i < kv_count; ++i) {
        fputc(' ', stderr);
        fputs(kvs[i].key ? kvs[i].key : "", stderr);
        fputc('=', stderr);
        fputs(kvs[i].value ? kvs[i].value : "", stderr);
    }
    fputc('\n', stderr);
    return gm_ok_void();
}

static const gm_diagnostics_port_vtbl_t VTBL = {
    .emit = emit_impl,
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
    port->vtbl = &VTBL;
    port->self = st;
    return gm_ok_void();
}
