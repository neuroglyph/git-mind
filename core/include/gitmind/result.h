/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_RESULT_H
#define GITMIND_RESULT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Forward declaration */
typedef struct gm_error gm_error_t;

/**
 * @brief Define a named Result type for a specific value type
 *
 * This macro creates a new struct type that can hold either a value or an
 * error. Using named types avoids the anonymous struct problem where each
 * expansion creates a different type.
 *
 * Example:
 *   GM_RESULT_DEF(gm_result_int, int);
 *   GM_RESULT_DEF(gm_result_string, char*);
 */
#define GM_RESULT_DEF(name, T)                                                 \
    typedef struct {                                                           \
        bool ok;                                                               \
        union {                                                                \
            T val;                                                             \
            gm_error_t *err;                                                   \
        } u;                                                                   \
    } name##_t

/* Define standard result types */
GM_RESULT_DEF(gm_result_void, int); /* int is dummy for void case */
GM_RESULT_DEF(gm_result_ptr, void *);
GM_RESULT_DEF(gm_result_int, int);
GM_RESULT_DEF(gm_result_bool, bool);
GM_RESULT_DEF(gm_result_size, size_t);
GM_RESULT_DEF(gm_result_u32, uint32_t);
GM_RESULT_DEF(gm_result_u64, uint64_t);

/* Legacy name for compatibility during migration */
typedef gm_result_ptr_t gm_result_t;

/* Success constructors */
static inline gm_result_void_t gm_ok_void(void) {
    return (gm_result_void_t){.ok = true, .u.val = 0};
}

static inline gm_result_ptr_t gm_ok_ptr(void *val) {
    return (gm_result_ptr_t){.ok = true, .u.val = val};
}

static inline gm_result_int_t gm_ok_int(int val) {
    return (gm_result_int_t){.ok = true, .u.val = val};
}

static inline gm_result_bool_t gm_ok_bool(bool val) {
    return (gm_result_bool_t){.ok = true, .u.val = val};
}

static inline gm_result_size_t gm_ok_size(size_t val) {
    return (gm_result_size_t){.ok = true, .u.val = val};
}

static inline gm_result_u32_t gm_ok_u32(uint32_t val) {
    return (gm_result_u32_t){.ok = true, .u.val = val};
}

/* Error constructors */
static inline gm_result_void_t gm_err_void(gm_error_t *err) {
    return (gm_result_void_t){.ok = false, .u.err = err};
}

static inline gm_result_ptr_t gm_err_ptr(gm_error_t *err) {
    return (gm_result_ptr_t){.ok = false, .u.err = err};
}

static inline gm_result_int_t gm_err_int(gm_error_t *err) {
    return (gm_result_int_t){.ok = false, .u.err = err};
}

static inline gm_result_bool_t gm_err_bool(gm_error_t *err) {
    return (gm_result_bool_t){.ok = false, .u.err = err};
}

static inline gm_result_size_t gm_err_size(gm_error_t *err) {
    return (gm_result_size_t){.ok = false, .u.err = err};
}

static inline gm_result_u32_t gm_err_u32(gm_error_t *err) {
    return (gm_result_u32_t){.ok = false, .u.err = err};
}

static inline gm_result_u64_t gm_ok_u64(uint64_t val) {
    return (gm_result_u64_t){.ok = true, .u.val = val};
}

static inline gm_result_u64_t gm_err_u64(gm_error_t *err) {
    return (gm_result_u64_t){.ok = false, .u.err = err};
}

/**
 * @brief Generic macros for result inspection
 *
 * These work with any result type created by GM_RESULT_DEF
 */
#define GM_IS_OK(r) ((r).ok)
#define GM_IS_ERR(r) (!(r).ok)
#define GM_UNWRAP(r) ((r).u.val)
#define GM_UNWRAP_ERR(r) ((r).u.err)

/**
 * @brief Early return on error
 *
 * If the expression evaluates to an error result, propagate it immediately.
 * Uses __auto_type (GNU extension) to work with any result type.
 */
#define GM_TRY(expr)                                                           \
    do {                                                                       \
        typeof(expr) _result = (expr);                                         \
        if (GM_IS_ERR(_result)) {                                              \
            return (typeof(_result)){.ok = false,                              \
                                     .u.err = GM_UNWRAP_ERR(_result)};         \
        }                                                                      \
    } while (0)

#endif /* GITMIND_RESULT_H */
