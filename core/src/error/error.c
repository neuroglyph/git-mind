/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/error.h"

#include "gitmind/result.h"
#include "gitmind/security/memory.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Helper to set error message with SSO */
static void set_error_message(gm_error_t *err, const char *fmt, va_list args) {
    /* Need to copy va_list to use it twice */
    va_list args_copy;
    va_copy(args_copy, args);

    /* First, measure the required length */
    int len = vsnprintf(NULL, 0, fmt, args);

    if (len < 0) {
        /* Formatting error - use a default message */
        strcpy(err->msg.small, "Error formatting failed");
        err->len = strlen(err->msg.small);
        err->heap_alloc = false;
        va_end(args_copy);
        return;
    }

    err->len = (uint16_t)len;

    if (len < GM_ERROR_SMALL_SIZE - 1) {
        /* Small string - store inline */
        vsnprintf(err->msg.small, GM_ERROR_SMALL_SIZE, fmt, args_copy);
        err->heap_alloc = false;
    } else {
        /* Large string - allocate exact size on heap */
        err->msg.heap = malloc(len + 1);
        if (err->msg.heap) {
            vsnprintf(err->msg.heap, len + 1, fmt, args_copy);
            err->heap_alloc = true;
        } else {
            /* Allocation failed - truncate to small buffer */
            vsnprintf(err->msg.small, GM_ERROR_SMALL_SIZE, fmt, args_copy);
            err->msg.small[GM_ERROR_SMALL_SIZE - 1] = '\0';
            err->len = GM_ERROR_SMALL_SIZE - 1;
            err->heap_alloc = false;
        }
    }

    va_end(args_copy);
}

/* Get message pointer (handles SSO) */
static const char *get_error_message(const gm_error_t *err) {
    return err->heap_alloc ? err->msg.heap : err->msg.small;
}

/* Create new error with formatted message */
gm_error_t *gm_error_new(int32_t code, const char *fmt, ...) {
    gm_error_t *err = malloc(sizeof(gm_error_t));
    if (!err) {
        return NULL; /* Can't allocate error to report allocation failure */
    }

    GM_MEMSET_SAFE(err, sizeof(gm_error_t), 0, sizeof(gm_error_t));
    err->code = code;

    /* Format message with SSO */
    va_list args;
    va_start(args, fmt);
    set_error_message(err, fmt, args);
    va_end(args);

    return err;
}

/* Create error with source location */
gm_error_t *gm_error_new_at(const char *file, int line, const char *func,
                            int32_t code, const char *fmt, ...) {
    gm_error_t *err = malloc(sizeof(gm_error_t));
    if (!err) {
        return NULL;
    }

    GM_MEMSET_SAFE(err, sizeof(gm_error_t), 0, sizeof(gm_error_t));
    err->code = code;
    err->file = file; /* Static string, no copy needed */
    err->line = line;
    err->func = func; /* Static string, no copy needed */

    /* Format message with SSO */
    va_list args;
    va_start(args, fmt);
    set_error_message(err, fmt, args);
    va_end(args);

    return err;
}

/* Add context to existing error */
gm_error_t *gm_error_wrap(gm_error_t *cause, int32_t code, const char *fmt,
                          ...) {
    gm_error_t *err = malloc(sizeof(gm_error_t));
    if (!err) {
        /* Can't wrap, at least don't leak the cause */
        gm_error_free(cause);
        return NULL;
    }

    GM_MEMSET_SAFE(err, sizeof(gm_error_t), 0, sizeof(gm_error_t));
    err->code = code;
    err->cause = cause; /* Take ownership */

    /* Format message with SSO */
    va_list args;
    va_start(args, fmt);
    set_error_message(err, fmt, args);
    va_end(args);

    return err;
}

/* Free error and its cause chain */
void gm_error_free(gm_error_t *error) {
    if (!error) {
        return;
    }

    /* Free heap-allocated message if present */
    if (error->heap_alloc && error->msg.heap) {
        free(error->msg.heap);
    }

    /* Free cause chain recursively */
    if (error->cause) {
        gm_error_free(error->cause);
    }

    /* Free any context data if present */
    if (error->context && error->context_free) {
        error->context_free(error->context);
    }

    free(error);
}

/* Format error chain as string */
char *gm_error_format(const gm_error_t *error) {
    if (!error) {
        return strdup("(no error)");
    }

    /* First pass: calculate exact size needed */
    size_t total_size = 1; /* for null terminator */
    const gm_error_t *err = error;
    while (err) {
        const char *msg = get_error_message(err);

        if (err->file && err->func) {
            /* Format: "[code] message (file:line in func)\n" */
            total_size +=
                snprintf(NULL, 0, "[%d] %s (%s:%d in %s)\n", err->code, msg,
                         err->file, err->line, err->func);
        } else {
            /* Format: "[code] message\n" */
            total_size += snprintf(NULL, 0, "[%d] %s\n", err->code, msg);
        }

        if (err->cause) {
            total_size += strlen("  caused by: ");
        }

        err = err->cause;
    }

    /* Allocate exact size */
    char *buffer = malloc(total_size);
    if (!buffer) {
        return strdup("(error formatting failed)");
    }

    /* Second pass: format into buffer */
    size_t offset = 0;
    err = error;
    while (err && offset < total_size - 1) {
        int written;

        const char *msg = get_error_message(err);

        if (err->file && err->func) {
            written = snprintf(buffer + offset, total_size - offset,
                               "[%d] %s (%s:%d in %s)\n", err->code, msg,
                               err->file, err->line, err->func);
        } else {
            written = snprintf(buffer + offset, total_size - offset,
                               "[%d] %s\n", err->code, msg);
        }

        if (written > 0 && (size_t)written < total_size - offset) {
            offset += written;
        } else {
            break;
        }

        err = err->cause;
        if (err && offset < total_size - 1) {
            written =
                snprintf(buffer + offset, total_size - offset, "  caused by: ");
            if (written > 0 && (size_t)written < total_size - offset) {
                offset += written;
            }
        }
    }

    buffer[total_size - 1] = '\0';
    return buffer;
}

/* Print error chain to stderr */
void gm_error_print(const gm_error_t *error) {
    char *formatted = gm_error_format(error);
    if (formatted) {
        fprintf(stderr, "%s", formatted);
        free(formatted);
    }
}