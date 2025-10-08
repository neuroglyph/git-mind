/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/ports/journal_command_port.h"

#include <stdlib.h>

#include "gitmind/error.h"
#include "gitmind/journal.h"
#include "gitmind/result.h"

typedef struct {
    gm_context_t *ctx; /* not owned */
} journal_cmd_state_t;

static gm_result_void_t append_impl(gm_cmd_journal_port_t *self,
                                    const gm_edge_t *edges, size_t count) {
    if (self == NULL || self->state == NULL || edges == NULL || count == 0U) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, "journal append requires edges"));
    }
    journal_cmd_state_t *st = (journal_cmd_state_t *)self->state;
    int rc = gm_journal_append(st->ctx, edges, count);
    if (rc != GM_OK) {
        return gm_err_void(GM_ERROR(rc, "journal append failed"));
    }
    return gm_ok_void();
}

static gm_result_void_t append_attr_impl(gm_cmd_journal_port_t *self,
                                         const gm_edge_attributed_t *edges,
                                         size_t count) {
    if (self == NULL || self->state == NULL || edges == NULL || count == 0U) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "journal append (attributed) requires edges"));
    }
    journal_cmd_state_t *st = (journal_cmd_state_t *)self->state;
    int rc = gm_journal_append_attributed(st->ctx, edges, count);
    if (rc != GM_OK) {
        return gm_err_void(GM_ERROR(rc, "journal append attributed failed"));
    }
    return gm_ok_void();
}

static const gm_cmd_journal_port_vtbl_t VTBL = {
    .append = append_impl,
    .append_attributed = append_attr_impl,
};

gm_result_void_t gm_cmd_journal_port_init(gm_cmd_journal_port_t *port,
                                          gm_context_t *ctx) {
    if (port == NULL || ctx == NULL) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, "journal port requires ctx"));
    }
    journal_cmd_state_t *st = (journal_cmd_state_t *)calloc(1, sizeof(*st));
    if (st == NULL) {
        return gm_err_void(
            GM_ERROR(GM_ERR_OUT_OF_MEMORY, "allocating journal port state"));
    }
    st->ctx = ctx;
    port->vtbl = &VTBL;
    port->state = st;
    return gm_ok_void();
}

void gm_cmd_journal_port_dispose(gm_cmd_journal_port_t *port) {
    if (port == NULL) return;
    free(port->state);
    port->state = NULL;
    port->vtbl = NULL;
}

