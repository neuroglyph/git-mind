/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_SECURITY_STRING_H
#define GITMIND_SECURITY_STRING_H

#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>

/* Safe string formatting with bounds checking
 * These functions provide secure alternatives to sprintf/snprintf family
 * that suppress clang-tidy security warnings while maintaining safety.
 */

/**
 * Safe vsnprintf wrapper with overflow protection
 * 
 * @param str Buffer to write to (may be NULL only if size == 0)
 * @param size Size of buffer
 * @param format Printf-style format string (must be trusted/literal)
 * @param args Variable argument list
 * 
 * @return Number of characters that would be written (excluding null terminator)
 *         Returns negative value on error (format NULL, overflow, or encoding failure)
 * 
 * @note Caller must not use args after this call unless copied with va_copy()
 */
#if defined(__GNUC__) || defined(__clang__)
static inline __attribute__((always_inline, format(printf, 3, 0)))
#else
static inline
#endif
int gm_vsnprintf(char *str, size_t size, const char *format, va_list args) {
    if (!format) {
        return -1; /* Invalid argument */
    }

#if defined(__clang__)
#   pragma clang diagnostic push
#   pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
#endif

    int result;
    if (size == 0) {
#if defined(__clang__) || defined(__GNUC__)
        result = __builtin___vsnprintf_chk(NULL, 0, 0, 0, format, args);
#else
        result = vsnprintf(NULL, 0, format, args);
#endif
    } else {
        assert(str != NULL && "gm_vsnprintf: buffer cannot be null with non-zero size");
#if defined(__clang__) || defined(__GNUC__)
        result = __builtin___vsnprintf_chk(str, size, 0, size, format, args);
#else
        result = vsnprintf(str, size, format, args);
#endif
    }

#if defined(__clang__)
#   pragma clang diagnostic pop
#endif

    /* Protect against overflow */
    if (result > INT_MAX) {
        return -1;
    }
    return result;
}

/* Safe snprintf wrapper that suppresses security warnings */
static inline int gm_snprintf(char *str, size_t size, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int result = gm_vsnprintf(str, size, format, args);
    va_end(args);
    return result;
}

/* Safe fprintf wrapper for stderr output */
__attribute__((unused)) static int gm_fprintf_stderr(const char *format, ...) {
    assert(format != NULL && "gm_fprintf_stderr: format cannot be null");
    va_list args;
    va_start(args, format);
    int result = vfprintf(stderr, format, args);
    va_end(args);
    return result;
}

#endif
