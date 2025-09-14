# Specification: Error Handling and Result Types

> _"Make illegal states unrepresentable. Make success and failure explicit."_

## Overview

This specification defines the foundational error handling system for git-mind, including the `GM_RESULT(T)` type, error propagation patterns, and core error codes.

## 1. Result Type Definition

### 1.1 Core Result Type

```c
// core/include/gitmind/result.h

// Define named result types (avoids anonymous struct issues)
#define GM_RESULT_DEF(name, T) \
    typedef struct { \
        bool ok; \
        union { \
            T val; \
            gm_error_t* err; \
        } u; \
    } name

// Define standard result types
GM_RESULT_DEF(gm_result_void, int);      // int is dummy for void case
GM_RESULT_DEF(gm_result_ptr, void*);
GM_RESULT_DEF(gm_result_int, int);
GM_RESULT_DEF(gm_result_bool, bool);
GM_RESULT_DEF(gm_result_size, size_t);

// Legacy name for compatibility
typedef gm_result_ptr gm_result_t;
```

### 1.2 Result Constructors

```c
// Success constructors (type-safe inline functions)
static inline gm_result_void gm_ok_void(void) {
    return (gm_result_void){ .ok = true, .u.val = 0 };
}

static inline gm_result_ptr gm_ok_ptr(void* v) {
    return (gm_result_ptr){ .ok = true, .u.val = v };
}

static inline gm_result_int gm_ok_int(int v) {
    return (gm_result_int){ .ok = true, .u.val = v };
}

static inline gm_result_size gm_ok_size(size_t v) {
    return (gm_result_size){ .ok = true, .u.val = v };
}

// Error constructors
static inline gm_result_void gm_err_void(gm_error_t* e) {
    return (gm_result_void){ .ok = false, .u.err = e };
}

static inline gm_result_ptr gm_err_ptr(gm_error_t* e) {
    return (gm_result_ptr){ .ok = false, .u.err = e };
}

// Generic macros for convenience (work with any result type)
#define GM_OK(result_type, val) \
    ((result_type){ .ok = true, .u.val = (val) })

#define GM_ERR(result_type, err) \
    ((result_type){ .ok = false, .u.err = (err) })
```

### 1.3 Result Inspection

```c
// Generic inspection macros (work with any result type)
#define GM_IS_OK(r) ((r).ok)
#define GM_IS_ERR(r) (!(r).ok)

// Extract value (caller must check ok first!)
#define GM_UNWRAP(r) ((r).u.val)

// Extract error (caller must check !ok first!)
#define GM_UNWRAP_ERR(r) ((r).u.err)

// Legacy names for compatibility
#define gm_is_ok(r) GM_IS_OK(r)
#define gm_is_err(r) GM_IS_ERR(r)
#define gm_unwrap(r) GM_UNWRAP(r)
#define gm_unwrap_err(r) GM_UNWRAP_ERR(r)
```

## 2. Error Type Definition

### 2.1 Error Structure

```c
// core/include/gitmind/error.h

typedef struct gm_error {
    int32_t code;                // Error code (not enum for extensibility)
    char message[256];           // Human-readable message
    const char* file;            // Source file (static string)
    int line;                    // Line number
    const char* func;            // Function name (static string)
    struct gm_error* cause;      // Previous error in chain (owned)
    void* context;               // Optional context data
} gm_error_t;
```

### 2.2 Error Codes

```c
typedef enum {
    GM_OK = 0,                   // Not an error
    
    // Generic errors (1-99)
    GM_ERR_UNKNOWN = 1,          // Unknown error
    GM_ERR_NOT_IMPLEMENTED = 2,  // Feature not implemented
    GM_ERR_INVALID_ARGUMENT = 3, // Invalid function argument
    GM_ERR_INVALID_STATE = 4,    // Invalid state for operation
    GM_ERR_NOT_FOUND = 5,        // Resource not found
    GM_ERR_ALREADY_EXISTS = 6,   // Resource already exists
    GM_ERR_PERMISSION_DENIED = 7,// Permission denied
    GM_ERR_TIMEOUT = 8,          // Operation timed out
    GM_ERR_CANCELLED = 9,        // Operation cancelled
    
    // Memory errors (100-199)
    GM_ERR_OUT_OF_MEMORY = 100,  // Memory allocation failed
    GM_ERR_BUFFER_TOO_SMALL = 101,// Buffer too small
    GM_ERR_MEMORY_CORRUPTED = 102,// Memory corruption detected
    
    // I/O errors (200-299)
    GM_ERR_IO_FAILED = 200,      // Generic I/O failure
    GM_ERR_FILE_NOT_FOUND = 201, // File not found
    GM_ERR_PATH_TOO_LONG = 202,  // Path exceeds limits
    GM_ERR_DISK_FULL = 203,      // No space left
    GM_ERR_READ_ONLY = 204,      // Read-only filesystem
    
    // Validation errors (300-399)
    GM_ERR_INVALID_FORMAT = 300, // Invalid data format
    GM_ERR_INVALID_UTF8 = 301,   // Invalid UTF-8 encoding
    GM_ERR_INVALID_PATH = 302,   // Invalid path (traversal, etc)
    GM_ERR_INVALID_LENGTH = 303, // Length constraint violated
    GM_ERR_INVALID_TYPE = 304,   // Type mismatch
    
    // Git-mind specific (1000+)
    GM_ERR_INVALID_NODE_ID = 1000,
    GM_ERR_INVALID_EDGE_TYPE = 1001,
    GM_ERR_CYCLE_DETECTED = 1002,
    GM_ERR_CORRUPT_STORAGE = 1003,
} gm_error_code_t;
```

### 2.3 Error Creation

```c
// Create new error with message
gm_error_t* gm_error_new(gm_error_code_t code, const char* fmt, ...);

// Create with source location (usually via macro)
gm_error_t* gm_error_new_at(
    const char* file, 
    int line, 
    const char* func,
    gm_error_code_t code, 
    const char* fmt, 
    ...
);

// Convenience macro that captures location
#define GM_ERROR(code, ...) \
    gm_error_new_at(__FILE__, __LINE__, __func__, code, __VA_ARGS__)
```

### 2.4 Error Propagation

```c
// Add context to existing error
gm_error_t* gm_error_wrap(
    gm_error_t* cause,
    gm_error_code_t code,
    const char* fmt,
    ...
);

// Propagate with additional context
#define GM_ERROR_PROPAGATE(cause, code, ...) \
    gm_error_wrap(cause, code, __VA_ARGS__)

// Common propagation pattern (generic)
#define GM_TRY(expr) \
    do { \
        __auto_type _result = (expr); \
        if (GM_IS_ERR(_result)) { \
            return (typeof(_result)){ .ok = false, .u.err = GM_UNWRAP_ERR(_result) }; \
        } \
    } while(0)

// Try with context (generic)
#define GM_TRY_CTX(expr, code, ...) \
    do { \
        __auto_type _result = (expr); \
        if (GM_IS_ERR(_result)) { \
            gm_error_t* _err = GM_UNWRAP_ERR(_result); \
            return (typeof(_result)){ .ok = false, .u.err = GM_ERROR_PROPAGATE(_err, code, __VA_ARGS__) }; \
        } \
    } while(0)
```

### 2.5 Error Cleanup

```c
// Free error and its cause chain
void gm_error_free(gm_error_t* error);

// Format error chain as string (caller must free)
char* gm_error_format(const gm_error_t* error);

// Print error chain to stderr
void gm_error_print(const gm_error_t* error);
```

## 3. Usage Patterns

### 3.1 Basic Function Pattern

```c
// First define the result type
GM_RESULT_DEF(gm_result_node_ptr, gm_node_t*);

gm_result_node_ptr gm_node_create(const char* path) {
    // Validate input
    if (!path || !*path) {
        return gm_err_ptr(GM_ERROR(GM_ERR_INVALID_ARGUMENT, 
                                   "Path cannot be empty"));
    }
    
    // Allocate node
    gm_node_t* node = malloc(sizeof(gm_node_t));
    if (!node) {
        return gm_err_ptr(GM_ERROR(GM_ERR_OUT_OF_MEMORY, 
                                   "Failed to allocate node"));
    }
    
    // Success
    return gm_ok_ptr(node);
}
```

### 3.2 Error Propagation Pattern

```c
GM_RESULT_DEF(gm_result_edge_ptr, gm_edge_t*);

gm_result_edge_ptr create_edge(const char* from, const char* to) {
    // Try operations, propagating errors
    gm_result_node_ptr from_result = gm_node_find(from);
    GM_TRY_CTX(from_result, GM_ERR_INVALID_ARGUMENT, 
               "Source node '%s' not found", from);
    
    gm_result_node_ptr to_result = gm_node_find(to);
    GM_TRY_CTX(to_result, GM_ERR_INVALID_ARGUMENT,
               "Target node '%s' not found", to);
    
    // Use unwrapped values
    gm_node_t* from_node = GM_UNWRAP(from_result);
    gm_node_t* to_node = GM_UNWRAP(to_result);
    
    return create_edge_internal(from_node, to_node);
}
```

### 3.3 Caller Pattern

```c
void process_file(const char* path) {
    gm_result_node_ptr result = gm_node_create(path);
    
    if (GM_IS_ERR(result)) {
        gm_error_t* err = GM_UNWRAP_ERR(result);
        gm_error_print(err);
        gm_error_free(err);
        return;
    }
    
    gm_node_t* node = GM_UNWRAP(result);
    // Use node...
    gm_node_free(node);
}
```

## 4. Implementation Notes

### 4.1 Memory Management

- Errors are always heap-allocated
- Errors own their cause chain
- Caller must free errors they receive
- Success values follow normal ownership rules

### 4.2 Thread Safety

- Error creation is thread-safe
- Error objects are immutable after creation
- Result types are value types (no sharing)

### 4.3 Performance Considerations

- Success path has minimal overhead (just a bool check)
- Error path allocates memory (acceptable for error cases)
- Consider arena allocation for temporary errors

## 5. Future Extensions

### 5.1 Result Combinators (Future)

```c
// Map over success value
GM_RESULT(int) string_to_int(const char* str);
GM_RESULT(bool) is_even(int n);

// Chain operations
GM_RESULT(bool) result = gm_result_and_then(
    string_to_int("42"),
    is_even
);
```

### 5.2 Async Results (Future)

```c
typedef struct {
    bool is_ready;
    GM_RESULT(T) result;
} gm_future_t;
```

## 6. Comparison with Other Systems

| Feature | git-mind | Rust | Go | C++ Expected |
|---------|----------|------|-----|--------------|
| Zero-cost success | ✅ | ✅ | ✅ | ✅ |
| Typed errors | ✅ | ✅ | ❌ | ✅ |
| Error chaining | ✅ | ✅ | ✅ | ❌ |
| Must handle | ❌* | ✅ | ✅ | ❌ |
| Combinators | Future | ✅ | ❌ | ✅ |

*Enforced by convention and code review

---

_"Make the right thing easy and the wrong thing obvious."_
