/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/output.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Create output context */
gm_output_t *gm_output_create(gm_output_level_t level,
                              gm_output_format_t format) {
    gm_output_t *out = calloc(1, sizeof(gm_output_t));
    if (!out)
        return NULL;

    out->level = level;
    out->format = format;
    out->suppress_errors = false;

    return out;
}

/* Print message (respects verbose flag) */
void gm_output_print(gm_output_t *out, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    gm_output_vprint(out, fmt, args);
    va_end(args);
}

void gm_output_vprint(gm_output_t *out, const char *fmt, va_list args) {
    if (!out || out->level == GM_OUTPUT_SILENT)
        return;

    if (out->format == GM_OUTPUT_PORCELAIN) {
        /* In porcelain mode, normal output is suppressed */
        return;
    }

    vprintf(fmt, args);
}

/* Print verbose message (only shown with --verbose) */
void gm_output_verbose(gm_output_t *out, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    gm_output_vverbose(out, fmt, args);
    va_end(args);
}

void gm_output_vverbose(gm_output_t *out, const char *fmt, va_list args) {
    if (!out || out->level < GM_OUTPUT_VERBOSE)
        return;

    if (out->format == GM_OUTPUT_PORCELAIN) {
        /* In porcelain mode, verbose output is suppressed */
        return;
    }

    vprintf(fmt, args);
}

/* Print error (always shown unless suppressed) */
void gm_output_error(gm_output_t *out, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    gm_output_verror(out, fmt, args);
    va_end(args);
}

void gm_output_verror(gm_output_t *out, const char *fmt, va_list args) {
    if (!out || out->suppress_errors)
        return;

    /* Errors always go to stderr, even in porcelain mode */
    vfprintf(stderr, fmt, args);
}

/* Print in porcelain format */
void gm_output_porcelain(gm_output_t *out, const char *key, const char *fmt,
                         ...) {
    va_list args;
    va_start(args, fmt);
    gm_output_vporcelain(out, key, fmt, args);
    va_end(args);
}

void gm_output_vporcelain(gm_output_t *out, const char *key, const char *fmt,
                          va_list args) {
    if (!out || out->format != GM_OUTPUT_PORCELAIN)
        return;

    /* Print key=value format for porcelain */
    printf("%s=", key);
    vprintf(fmt, args);
    printf("\n");
}

/* Print raw (bypass all formatting) */
void gm_output_raw(gm_output_t *out, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    gm_output_vraw(out, fmt, args);
    va_end(args);
}

void gm_output_vraw(gm_output_t *out, const char *fmt, va_list args) {
    if (!out)
        return;

    /* Raw output is always shown unless silent */
    if (out->level != GM_OUTPUT_SILENT) {
        vprintf(fmt, args);
    }
}

/* Check output settings */
bool gm_output_is_verbose(const gm_output_t *out) {
    return out && out->level >= GM_OUTPUT_VERBOSE;
}

bool gm_output_is_porcelain(const gm_output_t *out) {
    return out && out->format == GM_OUTPUT_PORCELAIN;
}

bool gm_output_is_silent(const gm_output_t *out) {
    return out && out->level == GM_OUTPUT_SILENT;
}

/* Destroy output context */
void gm_output_destroy(gm_output_t *out) {
    free(out);
}
