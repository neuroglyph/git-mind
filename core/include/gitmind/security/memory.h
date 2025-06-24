/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_SECURITY_MEMORY_H
#define GITMIND_SECURITY_MEMORY_H

#include <assert.h>
#include <stddef.h>
#include <string.h>

/* GM_MEMCPY_SAFE
 * dst    : pointer to destination buffer
 * dstsz  : total size of destination buffer in bytes
 * src    : pointer to source
 * n      : number of bytes to copy
 *
 * Guarantees:
 *   - Aborts or asserts in debug if n > dstsz
 *   - Compile-time diagnostic on Clang/GCC when sizes are constant
 */

/* Detect Annex K support */
#if defined(__STDC_LIB_EXT1__)
    /* The host libc provides memcpy_s, use it directly */
    #define GM_MEMCPY_SAFE(dst, dstsz, src, n)  memcpy_s((dst), (dstsz), (src), (n))
    #define GM_MEMSET_SAFE(dst, dstsz, val, n)  memset_s((dst), (dstsz), (val), (n))

#else
    /* Fallback: compile-time and run-time checks, then plain memcpy */
    #if defined(__clang__) || defined(__GNUC__)
        /* Clang/GCC: emit compile-time buffer-overflow errors when possible */
        #define GM_MEMCPY_SAFE(dst, dstsz, src, n)                               \
            __builtin___memcpy_chk((dst), (src), (n), (dstsz))
        
        #define GM_MEMSET_SAFE(dst, dstsz, val, n)                               \
            __builtin___memset_chk((dst), (val), (n), (dstsz))
    #else
        /* Generic: assert & memcpy */
        static inline void GM_MEMCPY_SAFE(void *dst, size_t dstsz,
                                          const void *src, size_t n)
        {
            assert(n <= dstsz && "GM_MEMCPY_SAFE: out-of-bounds copy");
            if (n) memcpy(dst, src, n);
        }
        
        static inline void GM_MEMSET_SAFE(void *dst, size_t dstsz,
                                          int val, size_t n)
        {
            assert(n <= dstsz && "GM_MEMSET_SAFE: out-of-bounds set");
            if (n) memset(dst, val, n);
        }
    #endif
#endif

/* Drop-in replacements for common memory functions */
#define GM_MEMCPY(dst, src, n)  GM_MEMCPY_SAFE((dst), (n), (src), (n))
#define GM_MEMSET(dst, val, n)  GM_MEMSET_SAFE((dst), (n), (val), (n))

#endif
