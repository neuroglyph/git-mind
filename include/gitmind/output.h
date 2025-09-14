/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_OUTPUT_H
#define GITMIND_OUTPUT_H

#include <stdarg.h>
#include <stdbool.h>

/* Output levels */
typedef enum {
    GM_OUTPUT_SILENT = 0, /* No output except errors */
    GM_OUTPUT_NORMAL = 1, /* Default output level */
    GM_OUTPUT_VERBOSE = 2 /* Verbose output */
} gm_output_level_t;

/* Output formats */
typedef enum {
    GM_OUTPUT_HUMAN = 0,    /* Human-readable output */
    GM_OUTPUT_PORCELAIN = 1 /* Machine-readable output */
} gm_output_format_t;

/* Output context structure */
typedef struct gm_output {
    gm_output_level_t level;
    gm_output_format_t format;
    bool suppress_errors; /* For testing only */
} gm_output_t;

/**
 * Create output context for controlling program output.
 *
 * @param level Output verbosity level
 * @param format Output format (human or machine)
 * @return New output context (caller must free)
 */
gm_output_t *gm_output_create(gm_output_level_t level,
                              gm_output_format_t format);

/**
 * Print normal message.
 *
 * Only printed if output level >= NORMAL.
 *
 * @param out Output context
 * @param fmt Printf-style format string
 */
void gm_output_print(gm_output_t *out, const char *fmt, ...);
void gm_output_vprint(gm_output_t *out, const char *fmt, va_list args);

/**
 * Print verbose message.
 *
 * Only printed if output level >= VERBOSE.
 *
 * @param out Output context
 * @param fmt Printf-style format string
 */
void gm_output_verbose(gm_output_t *out, const char *fmt, ...);
void gm_output_vverbose(gm_output_t *out, const char *fmt, va_list args);

/**
 * Print error message.
 *
 * Always shown unless errors are suppressed (testing only).
 *
 * @param out Output context
 * @param fmt Printf-style format string
 */
void gm_output_error(gm_output_t *out, const char *fmt, ...);
void gm_output_verror(gm_output_t *out, const char *fmt, va_list args);

/**
 * Print in machine-readable format.
 *
 * Format: key=value
 *
 * @param out Output context
 * @param key Key name
 * @param fmt Printf-style format for value
 */
void gm_output_porcelain(gm_output_t *out, const char *key, const char *fmt,
                         ...);
void gm_output_vporcelain(gm_output_t *out, const char *key, const char *fmt,
                          va_list args);

/* Print raw (bypass all formatting) */
void gm_output_raw(gm_output_t *out, const char *fmt, ...);
void gm_output_vraw(gm_output_t *out, const char *fmt, va_list args);

/* Check output settings */
bool gm_output_is_verbose(const gm_output_t *out);
bool gm_output_is_porcelain(const gm_output_t *out);
bool gm_output_is_silent(const gm_output_t *out);

/**
 * Free output context.
 *
 * @param out Output context to free
 */
void gm_output_destroy(gm_output_t *out);

#endif /* GITMIND_OUTPUT_H */
