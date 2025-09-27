/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_CLI_RUNTIME_H
#define GITMIND_CLI_RUNTIME_H

#include "gitmind/output.h"

typedef struct git_repository git_repository;

/* Minimal CLI runtime context (keeps core context pure) */
typedef struct gm_cli_ctx {
    gm_output_t *out;
    git_repository *repo;
} gm_cli_ctx_t;

#endif /* GITMIND_CLI_RUNTIME_H */
