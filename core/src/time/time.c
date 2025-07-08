/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

/* Feature test macros - exempt from naming conventions */
/* NOLINTBEGIN(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp,readability-identifier-naming) */
#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L
/* NOLINTEND(bugprone-reserved-identifier,cert-dcl37-c,cert-dcl51-cpp,readability-identifier-naming) */

#include <gitmind/time/time.h>
#include <gitmind/error.h>

#include <errno.h>
#include <string.h>
#include <time.h>

/* Error code constants */
static const int GM_ERROR_TIME_OPERATION = 5001;

/* Wrapper functions for time operations with Result types */

static gm_result_time_t wrap_time(time_t *tloc) {
    time_t result = time(tloc);
    if (result == (time_t)-1) {
        return (gm_result_time_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_TIME_OPERATION, "Failed to get time: %s", strerror(errno))
        };
    }
    return (gm_result_time_t){.ok = true, .u.val = result};
}

static gm_result_void_t wrap_clock_gettime(clockid_t clk_id, struct timespec *tp) {
    if (clock_gettime(clk_id, tp) != 0) {
        return (gm_result_void_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_TIME_OPERATION, "Failed to get clock time: %s", strerror(errno))
        };
    }
    return (gm_result_void_t){.ok = true};
}

static gm_result_tm_ptr_t wrap_localtime_r(const time_t *timep, struct tm *result) {
    struct tm *tm_result = localtime_r(timep, result);
    if (!tm_result) {
        return (gm_result_tm_ptr_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_TIME_OPERATION, "Failed to convert to local time: %s", strerror(errno))
        };
    }
    return (gm_result_tm_ptr_t){.ok = true, .u.val = tm_result};
}

static gm_result_tm_ptr_t wrap_gmtime_r(const time_t *timep, struct tm *result) {
    struct tm *tm_result = gmtime_r(timep, result);
    if (!tm_result) {
        return (gm_result_tm_ptr_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_TIME_OPERATION, "Failed to convert to GMT: %s", strerror(errno))
        };
    }
    return (gm_result_tm_ptr_t){.ok = true, .u.val = tm_result};
}

static gm_result_size_t wrap_strftime(char *s, size_t max, const char *format,
                                      const struct tm *tm) {
    size_t result = strftime(s, max, format, tm);
    if (result == 0) {
        return (gm_result_size_t){
            .ok = false,
            .u.err = GM_ERROR(GM_ERROR_TIME_OPERATION, "Failed to format time: buffer too small")
        };
    }
    return (gm_result_size_t){.ok = true, .u.val = result};
}

/* Static operations structure */

static const gm_time_ops_t GmDefaultTimeOps = {
    .time = wrap_time,
    .clock_gettime = wrap_clock_gettime,
    .localtime_r = wrap_localtime_r,
    .gmtime_r = wrap_gmtime_r,
    .strftime = wrap_strftime
};

const gm_time_ops_t *gm_time_ops_default(void) {
    return &GmDefaultTimeOps;
}
