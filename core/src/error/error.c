/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/security/memory.h"
#include "gitmind/security/string.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Error formatting constants */
static const char *const GM_ERR_NO_ERROR = "(no error)";
static const char *const GM_ERR_FORMAT_FAILED = "(error formatting failed)";
static const char *const GM_ERR_CAUSED_BY = "  caused by: ";
static const char *const GM_ERR_FORMAT_ERROR_MSG = "Error formatting failed";

/* Set error message for formatting failure */
static void set_format_error(gm_error_t *err) {
    /* Safe copy - we know GM_ERR_FORMAT_ERROR_MSG fits in small buffer */
    size_t msg_len = strlen(GM_ERR_FORMAT_ERROR_MSG);
    gm_memcpy_safe(err->msg.small, GM_ERROR_SMALL_SIZE, GM_ERR_FORMAT_ERROR_MSG, msg_len + 1);
    err->len = (uint16_t)msg_len;
    err->heap_alloc = false;
}

/* Store error message in small buffer */
static void store_small_message(gm_error_t *err, const char *fmt, va_list args) {
    int ret = gm_vsnprintf(err->msg.small, GM_ERROR_SMALL_SIZE, fmt, args);
    if (ret < 0) {
        set_format_error(err);
        return;
    }
    err->len =
        (ret >= GM_ERROR_SMALL_SIZE) ? GM_ERROR_SMALL_SIZE - 1 : (uint16_t)ret;
    err->heap_alloc = false;
}

/* Store error message on heap */
static void store_heap_message(gm_error_t *err, const char *fmt, va_list args, int len) {
    err->msg.heap = malloc(len + 1);
    if (err->msg.heap) {
        int ret = gm_vsnprintf(err->msg.heap, len + 1, fmt, args);
        if (ret < 0 || ret > len) {
            free(err->msg.heap);
            set_format_error(err);
            return;
        }
        err->len = (uint16_t)ret;
        err->heap_alloc = true;
    } else {
        /* Allocation failed - truncate to small buffer */
        int ret = gm_vsnprintf(err->msg.small, GM_ERROR_SMALL_SIZE, fmt, args);
        if (ret < 0) {
            set_format_error(err);
            return;
        }
        err->msg.small[GM_ERROR_SMALL_SIZE - 1] = '\0';
        err->len = GM_ERROR_SMALL_SIZE - 1;
        err->heap_alloc = false;
    }
}

/* Helper to set error message with SSO */
static void set_error_message(gm_error_t *err, const char *fmt, va_list args) {
    va_list args_copy;
    va_copy(args_copy, args);

    /* Measure the required length */
    int len = gm_vsnprintf(nullptr, 0, fmt, args);
    if (len < 0) {
        set_format_error(err);
        va_end(args_copy);
        return;
    }

    err->len = (uint16_t)len;
    if (len < GM_ERROR_SMALL_SIZE - 1) {
        store_small_message(err, fmt, args_copy);
    } else {
        store_heap_message(err, fmt, args_copy, len);
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
        return nullptr; /* Can't allocate error to report allocation failure */
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
        return nullptr;
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
        return nullptr;
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

/* Calculate size needed for single error */
static size_t calc_error_size(const gm_error_t *err) {
    const char *msg = get_error_message(err);
    
    if (err->file && err->func) {
        return gm_snprintf(nullptr, 0, "[%d] %s (%s:%d in %s)\n", 
                       err->code, msg, err->file, err->line, err->func);
    } else {
        return gm_snprintf(nullptr, 0, "[%d] %s\n", err->code, msg);
    }
}

/* Calculate total size for error chain */
static size_t calc_error_chain_size(const gm_error_t *error) {
    size_t total_size = 1; /* null terminator */
    const gm_error_t *err = error;
    
    while (err) {
        total_size += calc_error_size(err);
        if (err->cause) {
            total_size += strlen(GM_ERR_CAUSED_BY);
        }
        err = err->cause;
    }
    
    return total_size;
}

/* Format single error into buffer */
static size_t format_single_error(char *buffer, size_t size, const gm_error_t *err) {
    const char *msg = get_error_message(err);
    int written;
    
    if (err->file && err->func) {
        written = gm_snprintf(buffer, size, "[%d] %s (%s:%d in %s)\n",
                          err->code, msg, err->file, err->line, err->func);
    } else {
        written = gm_snprintf(buffer, size, "[%d] %s\n", err->code, msg);
    }
    
    return (written > 0 && (size_t)written < size) ? written : 0;
}

/* Append caused by prefix */
static size_t append_caused_by(char *buffer, size_t size) {
    int written = gm_snprintf(buffer, size, "%s", GM_ERR_CAUSED_BY);
    return (written > 0 && (size_t)written < size) ? written : 0;
}

/* Format error chain into buffer */
static void format_error_chain(char *buffer, size_t size, const gm_error_t *error) {
    size_t offset = 0;
    const gm_error_t *err = error;
    
    while (err && offset < size - 1) {
        size_t written = format_single_error(buffer + offset, size - offset, err);
        if (written == 0) {
            break;
        }
        offset += written;
        
        err = err->cause;
        if (err && offset < size - 1) {
            offset += append_caused_by(buffer + offset, size - offset);
        }
    }
    
    buffer[size - 1] = '\0';
}

/* Format error chain as string */
char *gm_error_format(const gm_error_t *error) {
    if (!error) {
        return strdup(GM_ERR_NO_ERROR);
    }

    size_t total_size = calc_error_chain_size(error);
    char *buffer = malloc(total_size);
    if (!buffer) {
        return strdup(GM_ERR_FORMAT_FAILED);
    }

    format_error_chain(buffer, total_size, error);
    return buffer;
}

/* Print error chain to stderr */
void gm_error_print(const gm_error_t *error) {
    char *formatted = gm_error_format(error);
    if (formatted) {
        (void)gm_fprintf_stderr("%s", formatted);
        free(formatted);
    }
}