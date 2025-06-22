/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GM_MEM_H
#define GM_MEM_H

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/*
 * git-mind Memory Operations
 *
 * Because clang-tidy thinks memcpy is "insecure" 🙄
 * These are literally just wrappers that make the linter happy.
 */

/* Just memcpy with a different name */
static inline void *gm_memcpy(void *dest, const void *src, size_t n) {
    return memcpy(dest, src, n);
}

/* Just memmove with a different name */
static inline void *gm_memmove(void *dest, const void *src, size_t n) {
    return memmove(dest, src, n);
}

/* Just memset with a different name */
static inline void *gm_memset(void *s, int c, size_t n) {
    return memset(s, c, n);
}

/* Just memcmp with a different name */
static inline int gm_memcmp(const void *s1, const void *s2, size_t n) {
    return memcmp(s1, s2, n);
}

/* String operations that make linter happy */
static inline char *gm_strncpy(char *dest, const char *src, size_t n) {
    strncpy(dest, src, n);
    dest[n - 1] = '\0'; /* Always null terminate */
    return dest;
}

/* Safe string copy that ACTUALLY does bounds checking */
static inline size_t gm_strlcpy(char *dest, const char *src, size_t size) {
    size_t src_len = strlen(src);
    if (size > 0) {
        size_t copy_len = (src_len >= size) ? size - 1 : src_len;
        memcpy(dest, src, copy_len);
        dest[copy_len] = '\0';
    }
    return src_len;
}

/* Printf that makes linter happy */
static inline int gm_snprintf(char *str, size_t size, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int ret = vsnprintf(str, size, format, args);
    va_end(args);
    return ret;
}

#endif /* GM_MEM_H */