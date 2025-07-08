/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_TIME_TIME_H
#define GITMIND_TIME_TIME_H

#include <gitmind/result.h>
#include <stddef.h>
#include <time.h>

/* Result types for time operations */
GM_RESULT_DEF(gm_result_time, time_t);
GM_RESULT_DEF(gm_result_tm_ptr, struct tm *);

/**
 * Time operations interface with Result types.
 * All operations return Result types for proper error handling.
 */
typedef struct gm_time_ops {
    gm_result_time_t (*time)(time_t *tloc);
    gm_result_void_t (*clock_gettime)(clockid_t clk_id, struct timespec *tp);
    gm_result_tm_ptr_t (*localtime_r)(const time_t *timep, struct tm *result);
    gm_result_tm_ptr_t (*gmtime_r)(const time_t *timep, struct tm *result);
    gm_result_size_t (*strftime)(char *s, size_t max, const char *format,
                                 const struct tm *tm);
} gm_time_ops_t;

/**
 * Get default time operations (uses real system calls).
 * @return Pointer to default time operations structure
 */
const gm_time_ops_t *gm_time_ops_default(void);

#endif /* GITMIND_TIME_TIME_H */
