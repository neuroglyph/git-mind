/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_CACHE_INTERNAL_H
#define GITMIND_CACHE_INTERNAL_H

#include <git2/oid.h>
#include <git2/repository.h>
#include <stdint.h>

/* Private cache helpers (not part of public API) */

int gm_cache_calculate_size(git_repository *repo, const git_oid *tree_oid,
                            uint64_t *size_bytes);

int gm_build_tree_from_directory(git_repository *repo, const char *dir_path,
                                 git_oid *tree_oid);

#endif /* GITMIND_CACHE_INTERNAL_H */

