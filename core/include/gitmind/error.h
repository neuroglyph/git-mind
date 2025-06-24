/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_ERROR_H
#define GITMIND_ERROR_H

#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>

/* Error message size constants */
#define GM_ERROR_SMALL_SIZE 48   /* Size for small string optimization */
#define GM_ERROR_MSG_SIZE 256    /* Default message buffer size (deprecated) */

/**
 * @brief Error structure with chaining support and SSO
 * 
 * Errors are heap-allocated and own their cause chain.
 * The caller must free errors they receive.
 * 
 * Uses small string optimization (SSO) for messages:
 * - Messages <= 48 bytes stored inline
 * - Longer messages allocated on heap
 */
typedef struct gm_error {
    int32_t code;                /* Error code (extensible) */
    union {
        char small[GM_ERROR_SMALL_SIZE];  /* Inline storage for small messages */
        char* heap;                       /* Heap pointer for large messages */
    } msg;
    uint16_t len;                /* Message length */
    bool heap_alloc;             /* True if message is heap allocated */
    const char* file;            /* Source file (static string) */
    int line;                    /* Line number */
    const char* func;            /* Function name (static string) */
    struct gm_error* cause;      /* Previous error in chain (owned) */
    void* context;               /* Optional context data */
    void (*context_free)(void*); /* Optional context destructor */
} gm_error_t;

/**
 * @brief Standard error codes
 * 
 * Using plain int32_t instead of enum for extensibility
 */
typedef enum {
    GM_OK = 0,                   /* Not an error */
    
    /* Generic errors (1-99) */
    GM_ERR_UNKNOWN = 1,          /* Unknown error */
    GM_ERR_NOT_IMPLEMENTED = 2,  /* Feature not implemented */
    GM_ERR_INVALID_ARGUMENT = 3, /* Invalid function argument */
    GM_ERR_INVALID_STATE = 4,    /* Invalid state for operation */
    GM_ERR_NOT_FOUND = 5,        /* Resource not found */
    GM_ERR_ALREADY_EXISTS = 6,   /* Resource already exists */
    GM_ERR_PERMISSION_DENIED = 7,/* Permission denied */
    GM_ERR_TIMEOUT = 8,          /* Operation timed out */
    GM_ERR_CANCELLED = 9,        /* Operation cancelled */
    
    /* Memory errors (100-199) */
    GM_ERR_OUT_OF_MEMORY = 100,  /* Memory allocation failed */
    GM_ERR_BUFFER_TOO_SMALL = 101,/* Buffer too small */
    GM_ERR_MEMORY_CORRUPTED = 102,/* Memory corruption detected */
    
    /* I/O errors (200-299) */
    GM_ERR_IO_FAILED = 200,      /* Generic I/O failure */
    GM_ERR_FILE_NOT_FOUND = 201, /* File not found */
    GM_ERR_PATH_TOO_LONG = 202,  /* Path exceeds limits */
    GM_ERR_DISK_FULL = 203,      /* No space left */
    GM_ERR_READ_ONLY = 204,      /* Read-only filesystem */
    
    /* Validation errors (300-399) */
    GM_ERR_INVALID_FORMAT = 300, /* Invalid data format */
    GM_ERR_INVALID_UTF8 = 301,   /* Invalid UTF-8 encoding */
    GM_ERR_INVALID_PATH = 302,   /* Invalid path (traversal, etc) */
    GM_ERR_INVALID_LENGTH = 303, /* Length constraint violated */
    GM_ERR_INVALID_TYPE = 304,   /* Type mismatch */
    
    /* Git-mind specific (1000+) */
    GM_ERR_INVALID_NODE_ID = 1000,
    GM_ERR_INVALID_EDGE_TYPE = 1001,
    GM_ERR_CYCLE_DETECTED = 1002,
    GM_ERR_CORRUPT_STORAGE = 1003,
} gm_error_code_t;

/**
 * @brief Create new error with formatted message
 * 
 * @param code Error code
 * @param fmt Printf-style format string
 * @param ... Format arguments
 * @return Heap-allocated error (caller must free)
 */
gm_error_t* gm_error_new(int32_t code, const char* fmt, ...);

/**
 * @brief Create error with source location
 * 
 * Usually called via GM_ERROR macro
 */
gm_error_t* gm_error_new_at(
    const char* file, 
    int line, 
    const char* func,
    int32_t code, 
    const char* fmt, 
    ...
);

/**
 * @brief Convenience macro that captures source location
 */
#define GM_ERROR(code, ...) \
    gm_error_new_at(__FILE__, __LINE__, __func__, code, __VA_ARGS__)

/**
 * @brief Add context to existing error
 * 
 * Takes ownership of cause
 */
gm_error_t* gm_error_wrap(
    gm_error_t* cause,
    int32_t code,
    const char* fmt,
    ...
);

/**
 * @brief Propagate error with additional context
 */
#define GM_ERROR_PROPAGATE(cause, code, ...) \
    gm_error_wrap(cause, code, __VA_ARGS__)

/**
 * @brief Try with context - add context on error
 */
#define GM_TRY_CTX(expr, code, ...) \
    do { \
        __auto_type _result = (expr); \
        if (GM_IS_ERR(_result)) { \
            gm_error_t* _err = GM_UNWRAP_ERR(_result); \
            return (typeof(_result)){ \
                .ok = false, \
                .u.err = GM_ERROR_PROPAGATE(_err, code, __VA_ARGS__) \
            }; \
        } \
    } while(0)

/**
 * @brief Free error and its cause chain
 */
void gm_error_free(gm_error_t* error);

/**
 * @brief Format error chain as string
 * 
 * @return Heap-allocated string (caller must free)
 */
char* gm_error_format(const gm_error_t* error);

/**
 * @brief Print error chain to stderr
 */
void gm_error_print(const gm_error_t* error);

#endif /* GITMIND_ERROR_H */