/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind.h"

#include "gitmind/constants_internal.h"

#include <stdarg.h>
#include <stdio.h>

/* Log level constants */
#define LOG_LEVEL_ERROR_ONLY 0

/* Error messages */
static const char *error_messages[] = {[0] "Success",
                                       [-GM_ERROR] "General error",
                                       [-GM_NOT_FOUND] "Not found",
                                       [-GM_INVALID_ARG] "Invalid argument",
                                       [-GM_NO_MEMORY] "Out of memory",
                                       [-GM_IO_ERROR] "I/O error"};

/* Get error string */
const char *gm_error_string(int error_code) {
    int index = -error_code;
    int max_index = sizeof(error_messages) / sizeof(error_messages[0]) - 1;

    if (index < 0 || index > max_index) {
        return "Unknown error";
    }

    return error_messages[index];
}

/* Default log function */
void gm_log_default(int level, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    if (level > LOG_LEVEL_ERROR_ONLY) { /* Only log errors by default */
        vfprintf(stderr, fmt, args);
        fprintf(stderr, "\n");
    }

    va_end(args);
}