/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_TIME_TIME_H
#define GITMIND_TIME_TIME_H

/* NOLINTNEXTLINE(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp,readability-identifier-naming) - POSIX feature test macro */
#define _POSIX_C_SOURCE 199309L

#include <gitmind/result.h>
#include <stddef.h>
#include <time.h>
/* NOLINTNEXTLINE(misc-include-cleaner) - provides clockid_t on some systems */
#include <sys/types.h>

/* Result types for time operations */
GM_RESULT_DEF(gm_result_time, time_t);
GM_RESULT_DEF(gm_result_tm_ptr, struct tm *);

/**
 * Time operations interface with Result types.
 * All operations return Result types for proper error handling.
 */
typedef struct gm_time_ops {
    gm_result_time_t (*time)(time_t *tloc);
    /* NOLINTNEXTLINE(misc-include-cleaner) - clockid_t from time.h/sys/types.h */
    gm_result_void_t (*clock_gettime)(clockid_t clk_id, struct timespec *timespec_ptr);
    gm_result_tm_ptr_t (*localtime_r)(const time_t *timep, struct tm *result);
    gm_result_tm_ptr_t (*gmtime_r)(const time_t *timep, struct tm *result);
    gm_result_size_t (*strftime)(char *str, size_t max, const char *format,
                                 const struct tm *time_ptr);
} gm_time_ops_t;

/**
 * Get default time operations (uses real system calls).
 * @return Pointer to default time operations structure
 */
const gm_time_ops_t *gm_time_ops_default(void);

#endif /* GITMIND_TIME_TIME_H */
