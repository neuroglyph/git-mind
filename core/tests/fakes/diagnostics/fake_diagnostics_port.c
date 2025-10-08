/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "core/tests/fakes/diagnostics/fake_diagnostics_port.h"

#include <stdlib.h>

#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/util/memory.h"

static gm_result_void_t emit_impl(void *self, const char *component,
                                  const char *event, const gm_diag_kv_t *kvs,
                                  size_t kv_count) {
    gm_fake_diag_state_t *st = (gm_fake_diag_state_t *)self;
    if (st == NULL) return gm_ok_void();
    if (st->count >= 64) return gm_ok_void();
    size_t i = st->count++;
    (void)gm_strcpy_safe(st->meta[i].component, sizeof(st->meta[i].component),
                         component ? component : "");
    (void)gm_strcpy_safe(st->meta[i].event, sizeof(st->meta[i].event),
                         event ? event : "");
    size_t cap = (kv_count > 8) ? 8 : kv_count;
    st->kv_counts[i] = cap;
    for (size_t j = 0; j < cap; ++j) {
        (void)gm_strcpy_safe(st->kvs[i][j].key, sizeof st->kvs[i][j].key,
                             kvs[j].key ? kvs[j].key : "");
        (void)gm_strcpy_safe(st->kvs[i][j].value, sizeof st->kvs[i][j].value,
                             kvs[j].value ? kvs[j].value : "");
    }
    return gm_ok_void();
}

static const gm_diagnostics_port_vtbl_t VTBL = {
    .emit = emit_impl,
};

gm_result_void_t gm_fake_diag_port_init(gm_diagnostics_port_t *out,
                                        gm_fake_diag_state_t **out_state) {
    if (out == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "fake diag requires out"));
    }
    gm_fake_diag_state_t *st = calloc(1, sizeof(*st));
    if (st == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_OUT_OF_MEMORY, "alloc fake diag"));
    }
    out->vtbl = &VTBL;
    out->self = st;
    if (out_state) *out_state = st;
    return gm_ok_void();
}

void gm_fake_diag_port_dispose(gm_diagnostics_port_t *port) {
    if (port == NULL) return;
    free(port->self);
    port->self = NULL;
    port->vtbl = NULL;
}
