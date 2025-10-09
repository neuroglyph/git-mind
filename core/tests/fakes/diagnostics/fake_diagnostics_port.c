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
    if (kv_count > 0 && kvs == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "diagnostics kv list missing"));
    }
    size_t slot = st->count;
    st->kv_counts[slot] = 0;
    if (gm_strcpy_safe(st->meta[slot].component,
                       sizeof(st->meta[slot].component),
                       component ? component : "") != GM_OK) {
        st->meta[slot].component[0] = '\0';
        st->meta[slot].event[0] = '\0';
        return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL,
                                    "diagnostics component truncated"));
    }
    if (gm_strcpy_safe(st->meta[slot].event,
                       sizeof(st->meta[slot].event),
                       event ? event : "") != GM_OK) {
        st->meta[slot].component[0] = '\0';
        st->meta[slot].event[0] = '\0';
        return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL,
                                    "diagnostics event truncated"));
    }
    size_t cap = kv_count > 8 ? 8 : kv_count;
    for (size_t j = 0; j < cap; ++j) {
        if (gm_strcpy_safe(st->kvs[slot][j].key,
                           sizeof st->kvs[slot][j].key,
                           kvs[j].key ? kvs[j].key : "") != GM_OK) {
            st->meta[slot].component[0] = '\0';
            st->meta[slot].event[0] = '\0';
            for (size_t r = 0; r <= j; ++r) {
                st->kvs[slot][r].key[0] = '\0';
                st->kvs[slot][r].value[0] = '\0';
            }
            return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL,
                                        "diagnostics kv key truncated"));
        }
        if (gm_strcpy_safe(st->kvs[slot][j].value,
                           sizeof st->kvs[slot][j].value,
                           kvs[j].value ? kvs[j].value : "") != GM_OK) {
            st->meta[slot].component[0] = '\0';
            st->meta[slot].event[0] = '\0';
            for (size_t r = 0; r <= j; ++r) {
                st->kvs[slot][r].key[0] = '\0';
                st->kvs[slot][r].value[0] = '\0';
            }
            return gm_err_void(GM_ERROR(GM_ERR_BUFFER_TOO_SMALL,
                                        "diagnostics kv value truncated"));
        }
    }
    st->kv_counts[slot] = cap;
    st->count = slot + 1;
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
