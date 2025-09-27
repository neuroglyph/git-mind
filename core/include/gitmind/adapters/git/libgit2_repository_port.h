/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_ADAPTERS_GIT_LIBGIT2_REPOSITORY_PORT_H
#define GITMIND_ADAPTERS_GIT_LIBGIT2_REPOSITORY_PORT_H

#include "gitmind/ports/git_repository_port.h"
#include "gitmind/result.h"

#include <git2/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gm_libgit2_repository_port_state
    gm_libgit2_repository_port_state_t;

gm_result_void_t gm_libgit2_repository_port_create(
    gm_git_repository_port_t *out_port,
    gm_libgit2_repository_port_state_t **out_state,
    void (**out_dispose)(gm_git_repository_port_t *), git_repository *repo);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_ADAPTERS_GIT_LIBGIT2_REPOSITORY_PORT_H */
