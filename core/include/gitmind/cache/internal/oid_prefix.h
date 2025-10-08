/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_CACHE_INTERNAL_OID_PREFIX_H
#define GITMIND_CACHE_INTERNAL_OID_PREFIX_H

#include <stddef.h>

#include "gitmind/util/oid.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum characters we ever write for an OID shard prefix */
#define GM_CACHE_MAX_SHARD_PATH 32

/*
 * Compute a hexadecimal OID prefix for sharding. The number of bits is rounded
 * up to the next whole hex character (4 bits per char). The output is
 * NUL-terminated and clamped to GM_CACHE_MAX_SHARD_PATH - 1 characters.
 */
GM_NODISCARD int gm_cache_oid_prefix(const gm_oid_t *oid, int bits,
                                     char *out, size_t out_size);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_CACHE_INTERNAL_OID_PREFIX_H */

