/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_TIME_OPS_H
#define GITMIND_TIME_OPS_H

#define _POSIX_C_SOURCE 200809L
#include <time.h>

/**
 * Time operations interface for dependency injection.
 * This allows for test doubles and deterministic testing.
 */
typedef struct gm_time_ops {
    time_t (*time)(time_t *t);
    int (*clock_gettime)(clockid_t clk_id, struct timespec *tp);
    struct tm *(*localtime_r)(const time_t *timep, struct tm *result);
    struct tm *(*gmtime_r)(const time_t *timep, struct tm *result);
    size_t (*strftime)(char *s, size_t max, const char *format,
                       const struct tm *tm);
} gm_time_ops_t;

/**
 * Get default time operations (uses real system calls).
 */
const gm_time_ops_t *gm_time_ops_default(void);

#endif /* GITMIND_TIME_OPS_H */