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
    } name

/* Define standard result types */
GM_RESULT_DEF(gm_result_void, int); /* int is dummy for void case */
GM_RESULT_DEF(gm_result_ptr, void *);
GM_RESULT_DEF(gm_result_int, int);
GM_RESULT_DEF(gm_result_bool, bool);
GM_RESULT_DEF(gm_result_size, size_t);
GM_RESULT_DEF(gm_result_u32, uint32_t);
GM_RESULT_DEF(gm_result_u64, uint64_t);

/* Legacy name for compatibility during migration */
typedef gm_result_ptr gm_result_t;

/* Success constructors */
static inline gm_result_void gm_ok_void(void) {
    return (gm_result_void){.ok = true, .u.val = 0};
}

static inline gm_result_ptr gm_ok_ptr(void *v) {
    return (gm_result_ptr){.ok = true, .u.val = v};
}

static inline gm_result_int gm_ok_int(int v) {
    return (gm_result_int){.ok = true, .u.val = v};
}

static inline gm_result_bool gm_ok_bool(bool v) {
    return (gm_result_bool){.ok = true, .u.val = v};
}

static inline gm_result_size gm_ok_size(size_t v) {
    return (gm_result_size){.ok = true, .u.val = v};
}

static inline gm_result_u32 gm_ok_u32(uint32_t v) {
    return (gm_result_u32){.ok = true, .u.val = v};
}

/* Error constructors */
static inline gm_result_void gm_err_void(gm_error_t *e) {
    return (gm_result_void){.ok = false, .u.err = e};
}

static inline gm_result_ptr gm_err_ptr(gm_error_t *e) {
    return (gm_result_ptr){.ok = false, .u.err = e};
}

static inline gm_result_int gm_err_int(gm_error_t *e) {
    return (gm_result_int){.ok = false, .u.err = e};
}

static inline gm_result_bool gm_err_bool(gm_error_t *e) {
    return (gm_result_bool){.ok = false, .u.err = e};
}

static inline gm_result_size gm_err_size(gm_error_t *e) {
    return (gm_result_size){.ok = false, .u.err = e};
}

static inline gm_result_u32 gm_err_u32(gm_error_t *e) {
    return (gm_result_u32){.ok = false, .u.err = e};
}

static inline gm_result_u64 gm_ok_u64(uint64_t v) {
    return (gm_result_u64){.ok = true, .u.val = v};
}

static inline gm_result_u64 gm_err_u64(gm_error_t *e) {
    return (gm_result_u64){.ok = false, .u.err = e};
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
        __auto_type _result = (expr);                                          \
        if (GM_IS_ERR(_result)) {                                              \
            return (typeof(_result)){.ok = false,                              \
                                     .u.err = GM_UNWRAP_ERR(_result)};         \
        }                                                                      \
    } while (0)

#endif /* GITMIND_RESULT_H */