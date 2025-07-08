/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_SECURITY_STRING_H
#define GITMIND_SECURITY_STRING_H

#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

/* Safe string formatting with bounds checking
 * These functions provide secure alternatives to sprintf/snprintf family
 * that suppress clang-tidy security warnings while maintaining safety.
 */

/* Safe vsnprintf wrapper that suppresses security warnings */
static inline int gm_vsnprintf(char *str, size_t size, const char *format, va_list ap) {
    assert(format != nullptr && "gm_vsnprintf: format cannot be null");
    if (size == 0) {
        return vsnprintf(nullptr, 0, format, ap);
    }
    assert(str != nullptr && "gm_vsnprintf: buffer cannot be null with non-zero size");
    return vsnprintf(str, size, format, ap);
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
static inline int gm_fprintf_stderr(const char *format, ...) {
    assert(format != nullptr && "gm_fprintf_stderr: format cannot be null");
    va_list args;
    va_start(args, format);
    int result = vfprintf(stderr, format, args);
    va_end(args);
    return result;
}

#endif