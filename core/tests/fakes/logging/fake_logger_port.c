/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "fake_logger_port.h"

#include <stdlib.h>
#include <string.h>

#include "gitmind/error.h"
#include "gitmind/security/string.h"
#include "gitmind/util/memory.h"

static gm_result_void_t log_impl(void *self, gm_log_level_t level,
                                 const char *component, const char *message) {
    gm_fake_logger_state_t *st = (gm_fake_logger_state_t *)self;
    if (st == NULL) return gm_ok_void();
    if (st->count >= 32) return gm_ok_void();
    size_t i = st->count++;
    st->level[i] = level;
    (void)gm_strcpy_safe(st->component[i], sizeof(st->component[i]),
                         component ? component : "");
    (void)gm_strcpy_safe(st->message[i], sizeof(st->message[i]),
                         message ? message : "");
    return gm_ok_void();
}

static const gm_logger_port_vtbl_t VTBL = {
    .log = log_impl,
};

gm_result_void_t gm_fake_logger_port_init(gm_logger_port_t *out,
                                          gm_fake_logger_state_t **out_state) {
    if (out == NULL) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, "fake logger requires out"));
    }
    gm_fake_logger_state_t *st = calloc(1, sizeof(*st));
    if (st == NULL) {
        return gm_err_void(
            GM_ERROR(GM_ERR_OUT_OF_MEMORY, "allocating fake logger state"));
    }
    out->vtbl = &VTBL;
    out->self = st;
    if (out_state) *out_state = st;
    return gm_ok_void();
}

void gm_fake_logger_port_dispose(gm_logger_port_t *port) {
    if (port == NULL) return;
    free(port->self);
    port->self = NULL;
    port->vtbl = NULL;
}
