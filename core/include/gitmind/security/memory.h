/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_SECURITY_MEMORY_H
#define GITMIND_SECURITY_MEMORY_H

#include <assert.h>
#include <stddef.h>
#include <string.h>

/* Safe memory copy with bounds checking
 * dst    : pointer to destination buffer
 * dstsz  : total size of destination buffer in bytes
 * src    : pointer to source
 * n      : number of bytes to copy
 *
 * Guarantees:
 *   - Aborts or asserts in debug if n > dstsz
 *   - Compile-time diagnostic on Clang/GCC when sizes are constant
 */
static inline void gm_memcpy_safe(void *dst, size_t dstsz,
                                  const void *src, size_t n) {
    assert(n <= dstsz && "gm_memcpy_safe: out-of-bounds copy");
    if (n) {
#if defined(__STDC_LIB_EXT1__)
        memcpy_s(dst, dstsz, src, n);
#elif defined(__clang__) || defined(__GNUC__)
        __builtin___memcpy_chk(dst, src, n, dstsz);
#else
        memcpy(dst, src, n);
#endif
    }
}

/* Safe memory set with bounds checking
 * dst    : pointer to destination buffer
 * dstsz  : total size of destination buffer in bytes
 * val    : value to set
 * n      : number of bytes to set
 *
 * Guarantees:
 *   - Aborts or asserts in debug if n > dstsz
 *   - Compile-time diagnostic on Clang/GCC when sizes are constant
 */
static inline void gm_memset_safe(void *dst, size_t dstsz,
                                  int val, size_t n) {
    assert(n <= dstsz && "gm_memset_safe: out-of-bounds set");
    if (n) {
#if defined(__STDC_LIB_EXT1__)
        memset_s(dst, dstsz, val, n);
#elif defined(__clang__) || defined(__GNUC__)
        __builtin___memset_chk(dst, val, n, dstsz);
#else
        memset(dst, val, n);
#endif
    }
}

/* Convenience wrappers for common patterns */
static inline void gm_memcpy(void *dst, const void *src, size_t n) {
    gm_memcpy_safe(dst, n, src, n);
}

static inline void gm_memset(void *dst, int val, size_t n) {
    gm_memset_safe(dst, n, (unsigned char)val, n);
}

/* Compatibility macros - to be removed after migration */
#define GM_MEMCPY_SAFE gm_memcpy_safe
#define GM_MEMSET_SAFE gm_memset_safe
#define GM_MEMCPY gm_memcpy
#define GM_MEMSET gm_memset

#endif
