/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_UTIL_REF_H
#define GITMIND_UTIL_REF_H

#include <stddef.h>

/* Build a full Git ref name with bounds checking.
 * out/out_sz: destination buffer and size
 * prefix    : ref prefix (e.g., "refs/gitmind/edges/")
 * branch    : Git-style shorthand ref (may include '/'; must NOT start with "refs/")
 * Validation uses libgit2's git_reference_normalize_name on the combined ref.
 * Returns GM_OK or error code.
 */
int gm_build_ref(char *out, size_t out_sz, const char *prefix,
                 const char *branch);

#endif /* GITMIND_UTIL_REF_H */
