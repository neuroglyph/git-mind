/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_PORTS_JOURNAL_COMMAND_PORT_H
#define GITMIND_PORTS_JOURNAL_COMMAND_PORT_H

#include <stddef.h>

#include "gitmind/context.h"
#include "gitmind/result.h"
#include "gitmind/edge.h"
#include "gitmind/edge_attributed.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file journal_command_port.h
 * @brief Inbound driving port for journal commands (append, etc.).
 */

typedef struct gm_cmd_journal_port_vtbl gm_cmd_journal_port_vtbl_t;

typedef struct gm_cmd_journal_port {
    const gm_cmd_journal_port_vtbl_t *vtbl; /* non-owning */
    void *state;                             /* opaque */
} gm_cmd_journal_port_t;

struct gm_cmd_journal_port_vtbl {
    /** Append basic edges to the journal (current branch). */
    gm_result_void_t (*append)(gm_cmd_journal_port_t *self,
                               const gm_edge_t *edges, size_t count);
    /** Append attributed edges to the journal (current branch). */
    gm_result_void_t (*append_attributed)(gm_cmd_journal_port_t *self,
                                          const gm_edge_attributed_t *edges,
                                          size_t count);
};

/* Default implementation factory (thin coordinator over existing APIs). */
GM_NODISCARD gm_result_void_t gm_cmd_journal_port_init(
    gm_cmd_journal_port_t *port, gm_context_t *ctx);
void gm_cmd_journal_port_dispose(gm_cmd_journal_port_t *port);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_PORTS_JOURNAL_COMMAND_PORT_H */

