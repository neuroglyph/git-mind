/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE

#include "gitmind/time_ops.h"

#include <time.h>

/* Default time operations - use real system calls */
static const gm_time_ops_t default_time_ops = {.time = time,
                                               .clock_gettime = clock_gettime,
                                               .localtime_r = localtime_r,
                                               .gmtime_r = gmtime_r,
                                               .strftime = strftime};

const gm_time_ops_t *gm_time_ops_default(void) {
    return &default_time_ops;
}