/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#define _POSIX_C_SOURCE 200809L

#include <gitmind/time/time.h>
#include <gitmind/error.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* Test constants */
static const time_t TEST_TIMESTAMP = 1234567890; /* 2009-02-13 23:31:30 UTC */
static const char *TEST_FORMAT = "%Y-%m-%d %H:%M:%S";
static const int BUFFER_SIZE = 128;

/* Test time operations */
static void test_time_operations(void) {
    printf("test_time_operations... ");
    
    const gm_time_ops_t *time_ops = gm_time_ops_default();
    assert(time_ops != NULL);
    
    /* Test time() */
    time_t before = time(NULL);
    gm_result_time_t time_result = time_ops->time(NULL);
    assert(time_result.ok);
    assert(time_result.u.val >= before);
    
    /* Test time() with output parameter */
    time_t time_val;
    time_result = time_ops->time(&time_val);
    assert(time_result.ok);
    assert(time_result.u.val == time_val);
    
    printf("OK\n");
}

/* Test clock operations */
static void test_clock_operations(void) {
    printf("test_clock_operations... ");
    
    const gm_time_ops_t *time_ops = gm_time_ops_default();
    
    /* Test clock_gettime with CLOCK_REALTIME */
    struct timespec ts;
    gm_result_void_t clock_result = time_ops->clock_gettime(CLOCK_REALTIME, &ts);
    assert(clock_result.ok);
    assert(ts.tv_sec > 0);
    assert(ts.tv_nsec >= 0 && ts.tv_nsec < 1000000000);
    
    /* Test clock_gettime with CLOCK_MONOTONIC */
    clock_result = time_ops->clock_gettime(CLOCK_MONOTONIC, &ts);
    assert(clock_result.ok);
    assert(ts.tv_sec >= 0);
    assert(ts.tv_nsec >= 0 && ts.tv_nsec < 1000000000);
    
    printf("OK\n");
}

/* Test time conversion operations */
static void test_time_conversions(void) {
    printf("test_time_conversions... ");
    
    const gm_time_ops_t *time_ops = gm_time_ops_default();
    
    /* Test localtime_r */
    struct tm local_tm;
    gm_result_tm_ptr_t localtime_result = time_ops->localtime_r(&TEST_TIMESTAMP, &local_tm);
    assert(localtime_result.ok);
    assert(localtime_result.u.val == &local_tm);
    assert(local_tm.tm_year == 109); /* 2009 - 1900 */
    assert(local_tm.tm_mon == 1);    /* February (0-based) */
    assert(local_tm.tm_mday == 13);
    
    /* Test gmtime_r */
    struct tm gm_tm;
    gm_result_tm_ptr_t gmtime_result = time_ops->gmtime_r(&TEST_TIMESTAMP, &gm_tm);
    assert(gmtime_result.ok);
    assert(gmtime_result.u.val == &gm_tm);
    assert(gm_tm.tm_year == 109); /* 2009 - 1900 */
    assert(gm_tm.tm_mon == 1);    /* February (0-based) */
    assert(gm_tm.tm_mday == 13);
    assert(gm_tm.tm_hour == 23);
    assert(gm_tm.tm_min == 31);
    assert(gm_tm.tm_sec == 30);
    
    printf("OK\n");
}

/* Test time formatting */
static void test_time_formatting(void) {
    printf("test_time_formatting... ");
    
    const gm_time_ops_t *time_ops = gm_time_ops_default();
    
    /* Create a known time structure */
    struct tm test_tm = {
        .tm_year = 109,  /* 2009 - 1900 */
        .tm_mon = 1,     /* February */
        .tm_mday = 13,
        .tm_hour = 23,
        .tm_min = 31,
        .tm_sec = 30,
        .tm_isdst = 0
    };
    
    /* Test strftime */
    char buffer[128];
    gm_result_size_t strftime_result = time_ops->strftime(buffer, (size_t)BUFFER_SIZE, TEST_FORMAT, &test_tm);
    assert(strftime_result.ok);
    assert(strftime_result.u.val > 0);
    assert(strcmp(buffer, "2009-02-13 23:31:30") == 0);
    
    /* Test strftime with different format */
    strftime_result = time_ops->strftime(buffer, (size_t)BUFFER_SIZE, "%Y", &test_tm);
    assert(strftime_result.ok);
    assert(strcmp(buffer, "2009") == 0);
    
    /* Test strftime with small buffer (should fail) */
    strftime_result = time_ops->strftime(buffer, 1, TEST_FORMAT, &test_tm);
    assert(!strftime_result.ok);
    assert(strftime_result.u.err != NULL);
    assert(strftime_result.u.err->code == 5001); /* GM_ERROR_TIME_OPERATION */
    gm_error_free(strftime_result.u.err);
    
    printf("OK\n");
}

/* Test error handling */
static void test_error_handling(void) {
    printf("test_error_handling... ");
    
    const gm_time_ops_t *time_ops = gm_time_ops_default();
    
    /* Test clock_gettime with invalid clock ID */
    struct timespec ts;
    gm_result_void_t clock_result = time_ops->clock_gettime((clockid_t)-1, &ts);
    assert(!clock_result.ok);
    assert(clock_result.u.err != NULL);
    assert(clock_result.u.err->code == 5001); /* GM_ERROR_TIME_OPERATION */
    gm_error_free(clock_result.u.err);
    
    /* Clock gettime error was already tested above */
    
    printf("OK\n");
}

/* Test default operations availability */
static void test_default_operations(void) {
    printf("test_default_operations... ");
    
    const gm_time_ops_t *time_ops = gm_time_ops_default();
    assert(time_ops != NULL);
    
    /* Check all operations are available */
    assert(time_ops->time != NULL);
    assert(time_ops->clock_gettime != NULL);
    assert(time_ops->localtime_r != NULL);
    assert(time_ops->gmtime_r != NULL);
    assert(time_ops->strftime != NULL);
    
    printf("OK\n");
}

/* Test monotonic clock behavior */
static void test_monotonic_clock(void) {
    printf("test_monotonic_clock... ");
    
    const gm_time_ops_t *time_ops = gm_time_ops_default();
    
    /* Get two timestamps and ensure monotonic property */
    struct timespec ts1, ts2;
    gm_result_void_t result1 = time_ops->clock_gettime(CLOCK_MONOTONIC, &ts1);
    assert(result1.ok);
    
    /* Small delay */
    struct timespec delay = {.tv_sec = 0, .tv_nsec = 1000000}; /* 1ms */
    nanosleep(&delay, NULL);
    
    gm_result_void_t result2 = time_ops->clock_gettime(CLOCK_MONOTONIC, &ts2);
    assert(result2.ok);
    
    /* ts2 should be greater than ts1 */
    assert(ts2.tv_sec > ts1.tv_sec || 
           (ts2.tv_sec == ts1.tv_sec && ts2.tv_nsec > ts1.tv_nsec));
    
    printf("OK\n");
}

int main(void) {
    printf("Running time operation tests...\n");
    
    /* Run tests */
    test_default_operations();
    test_time_operations();
    test_clock_operations();
    test_time_conversions();
    test_time_formatting();
    test_error_handling();
    test_monotonic_clock();
    
    printf("All tests passed!\n");
    return 0;
}
