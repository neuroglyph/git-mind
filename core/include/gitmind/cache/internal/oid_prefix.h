/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_CACHE_INTERNAL_OID_PREFIX_H
#define GITMIND_CACHE_INTERNAL_OID_PREFIX_H

#include <stddef.h>

#include "gitmind/result.h"
#include "gitmind/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Maximum characters we ever write for an OID shard prefix.
 * 32 characters permit up to 128 bits of prefix material (32 * 4) which keeps
 * shard IDs short while covering SHA-1/SHA-256 selector needs.
 */
#define GM_CACHE_MAX_SHARD_PATH 32

/*
 * Compute a hexadecimal OID prefix for sharding. The number of bits is rounded
 * up to the next whole hex character (4 bits per char). The output is
 * NUL-terminated and clamped to GM_CACHE_MAX_SHARD_PATH - 1 characters.
 * Returns GM_OK on success. Returns GM_ERR_INVALID_ARGUMENT when oid or out is
 * NULL, or when out_size is 0. Thread-safe/pure: no global state touched.
 */
GM_NODISCARD int gm_cache_oid_prefix(const gm_oid_t *oid, int bits,
                                     char *out, size_t out_size);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_CACHE_INTERNAL_OID_PREFIX_H */
