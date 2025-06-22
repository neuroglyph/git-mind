/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/error.h"
#include "gitmind/result.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Create new error with formatted message */
gm_error_t* gm_error_new(int32_t code, const char* fmt, ...) {
    gm_error_t* err = malloc(sizeof(gm_error_t));
    if (!err) {
        return NULL; /* Can't allocate error to report allocation failure */
    }
    
    memset(err, 0, sizeof(gm_error_t));
    err->code = code;
    
    /* Format message */
    va_list args;
    va_start(args, fmt);
    vsnprintf(err->message, sizeof(err->message), fmt, args);
    va_end(args);
    
    /* Ensure null termination */
    err->message[sizeof(err->message) - 1] = '\0';
    
    return err;
}

/* Create error with source location */
gm_error_t* gm_error_new_at(
    const char* file, 
    int line, 
    const char* func,
    int32_t code, 
    const char* fmt, 
    ...
) {
    gm_error_t* err = malloc(sizeof(gm_error_t));
    if (!err) {
        return NULL;
    }
    
    memset(err, 0, sizeof(gm_error_t));
    err->code = code;
    err->file = file; /* Static string, no copy needed */
    err->line = line;
    err->func = func; /* Static string, no copy needed */
    
    /* Format message */
    va_list args;
    va_start(args, fmt);
    vsnprintf(err->message, sizeof(err->message), fmt, args);
    va_end(args);
    
    err->message[sizeof(err->message) - 1] = '\0';
    
    return err;
}

/* Add context to existing error */
gm_error_t* gm_error_wrap(
    gm_error_t* cause,
    int32_t code,
    const char* fmt,
    ...
) {
    gm_error_t* err = malloc(sizeof(gm_error_t));
    if (!err) {
        /* Can't wrap, at least don't leak the cause */
        gm_error_free(cause);
        return NULL;
    }
    
    memset(err, 0, sizeof(gm_error_t));
    err->code = code;
    err->cause = cause; /* Take ownership */
    
    /* Format message */
    va_list args;
    va_start(args, fmt);
    vsnprintf(err->message, sizeof(err->message), fmt, args);
    va_end(args);
    
    err->message[sizeof(err->message) - 1] = '\0';
    
    return err;
}

/* Free error and its cause chain */
void gm_error_free(gm_error_t* error) {
    if (!error) {
        return;
    }
    
    /* Free cause chain recursively */
    if (error->cause) {
        gm_error_free(error->cause);
    }
    
    /* Free any context data if present */
    if (error->context) {
        free(error->context);
    }
    
    free(error);
}

/* Format error chain as string */
char* gm_error_format(const gm_error_t* error) {
    if (!error) {
        return strdup("(no error)");
    }
    
    /* Count errors in chain */
    size_t count = 0;
    const gm_error_t* e = error;
    while (e) {
        count++;
        e = e->cause;
    }
    
    /* Allocate buffer (generously) */
    size_t buf_size = count * 512;
    char* buffer = malloc(buf_size);
    if (!buffer) {
        return strdup("(error formatting failed)");
    }
    
    /* Format each error */
    size_t offset = 0;
    e = error;
    while (e && offset < buf_size - 1) {
        int written;
        
        if (e->file && e->func) {
            written = snprintf(
                buffer + offset, 
                buf_size - offset,
                "[%d] %s (%s:%d in %s)\n",
                e->code,
                e->message,
                e->file,
                e->line,
                e->func
            );
        } else {
            written = snprintf(
                buffer + offset, 
                buf_size - offset,
                "[%d] %s\n",
                e->code,
                e->message
            );
        }
        
        if (written > 0 && (size_t)written < buf_size - offset) {
            offset += written;
        } else {
            break;
        }
        
        e = e->cause;
        if (e && offset < buf_size - 1) {
            written = snprintf(
                buffer + offset,
                buf_size - offset,
                "  caused by: "
            );
            if (written > 0 && (size_t)written < buf_size - offset) {
                offset += written;
            }
        }
    }
    
    buffer[buf_size - 1] = '\0';
    return buffer;
}

/* Print error chain to stderr */
void gm_error_print(const gm_error_t* error) {
    char* formatted = gm_error_format(error);
    if (formatted) {
        fprintf(stderr, "%s", formatted);
        free(formatted);
    }
}