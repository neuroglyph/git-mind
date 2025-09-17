---
title: Specification: Security Primitives
description: Historical notes and enforcement-era artifacts.
audience: [contributors]
domain: [quality]
tags: [enforcer]
status: archive
last_updated: 2025-09-15
---

# Specification: Security Primitives

> _"Security isn't something you add later. It's something you build in from the start."_

## Overview

This specification defines the foundational security primitives for git-mind, focusing on input validation, path safety, and secure string operations that prevent common vulnerabilities.

## 1. Input Validation Framework

### 1.1 Validation Result Type

```c
// core/include/gitmind/security/validate.h

// Validation error details
typedef struct gm_validation_error {
    gm_validation_error_code_t code;
    const char* field;          // Which field failed
    const char* reason;         // Human-readable reason
    size_t position;            // Position in input (if applicable)
    char invalid_char;          // The problematic character
} gm_validation_error_t;

typedef enum {
    GM_VALIDATION_OK = 0,
    GM_VALIDATION_ERR_EMPTY,
    GM_VALIDATION_ERR_TOO_LONG,
    GM_VALIDATION_ERR_TOO_SHORT,
    GM_VALIDATION_ERR_INVALID_CHAR,
    GM_VALIDATION_ERR_INVALID_UTF8,
    GM_VALIDATION_ERR_PATH_TRAVERSAL,
    GM_VALIDATION_ERR_ABSOLUTE_PATH,
    GM_VALIDATION_ERR_RESERVED_NAME,
    GM_VALIDATION_ERR_INVALID_FORMAT
} gm_validation_error_code_t;
```

### 1.2 UTF-8 Validation

```c
// Validate UTF-8 encoding
gm_result_void gm_validate_utf8(
    const char* str, 
    size_t len,
    gm_validation_error_t* error
);

// Validate and measure UTF-8 string
gm_result_size gm_validate_utf8_length(
    const char* str,
    size_t max_bytes,
    size_t* char_count  // Output: number of UTF-8 characters
);

// Check if character is valid for identifiers
bool gm_is_valid_identifier_char(char c, bool first_char);
```

### 1.3 Length Validation

```c
// Length constraints
#define GM_MAX_PATH_LENGTH      4096
#define GM_MAX_NAME_LENGTH      255
#define GM_MAX_STRING_LENGTH    65536
#define GM_MAX_IDENTIFIER_LENGTH 128
#define GM_MAX_URL_LENGTH       2048

// Validate length constraints
gm_result_void gm_validate_length(
    const char* str,
    size_t min_len,
    size_t max_len,
    const char* field_name
);
```

## 2. Path Security

### 2.1 Path Validation Rules

```c
// core/include/gitmind/security/path.h

// Path validation rules
typedef struct gm_path_rules {
    bool allow_absolute;        // Allow absolute paths
    bool allow_relative;        // Allow relative paths  
    bool allow_symlinks;        // Allow symbolic links
    bool allow_hidden;          // Allow .hidden files
    bool allow_special;         // Allow special chars
    size_t max_depth;          // Max directory depth
    size_t max_length;         // Max total length
    const char** allowed_prefixes;  // Whitelist prefixes
    size_t prefix_count;
    const char** forbidden_patterns; // Blacklist patterns
    size_t forbidden_count;
} gm_path_rules_t;

// Predefined rule sets
extern const gm_path_rules_t GM_PATH_RULES_STRICT;    // Most restrictive
extern const gm_path_rules_t GM_PATH_RULES_REPO;      // For repo paths
extern const gm_path_rules_t GM_PATH_RULES_USER;      // For user input
extern const gm_path_rules_t GM_PATH_RULES_SYSTEM;    // For system paths
```

### 2.2 Path Validation Functions

```c
// Validate path against rules
gm_result_void gm_validate_path(
    const char* path,
    const gm_path_rules_t* rules,
    gm_validation_error_t* error
);

// Validate and canonicalize path
gm_result_string gm_path_canonicalize_safe(
    const char* path,
    const gm_path_rules_t* rules
);

// Check for path traversal attempts
bool gm_path_has_traversal(const char* path);

// Check if path is within boundary
bool gm_path_is_within(const char* path, const char* boundary);

// Validate filename (no directory separators)
gm_result_void gm_validate_filename(
    const char* filename,
    gm_validation_error_t* error
);
```

### 2.3 Path Traversal Prevention

```c
// Common dangerous patterns
static const char* DANGEROUS_PATTERNS[] = {
    "../",           // Parent directory
    "..\\",          // Windows parent
    "./",            // Current directory (can be abused)
    ".\\",           // Windows current
    "~/",            // Home directory expansion
    "${",            // Variable expansion
    "$(", 
    "`",             // Command substitution
    "|",             // Pipe
    ";",             // Command separator
    "&",             // Background
    ">",             // Redirect
    "<",
    "\n",            // Newline injection
    "\r",
    "\0",            // Null byte injection
    "%2e%2e/",       // URL-encoded parent
    "%2e%2e%2f",     // URL-encoded parent (alt)
    "..%2f",         // Mixed encoding
    "..%252f",       // Double-encoded
};

// Detect dangerous patterns (handles URL encoding)
bool gm_path_has_dangerous_pattern(const char* path);

// Decode URL-encoded path before validation
gm_result_string gm_path_url_decode(const char* encoded);
```

## 3. String Sanitization

### 3.1 Filename Sanitization

```c
// core/include/gitmind/security/sanitize.h

// Sanitize filename for safe filesystem use
gm_result_string gm_sanitize_filename(
    const char* input,
    gm_sanitize_options_t options
);

typedef enum {
    GM_SANITIZE_NONE = 0,
    GM_SANITIZE_SPACES = 1 << 0,      // Replace spaces with _
    GM_SANITIZE_SPECIAL = 1 << 1,     // Remove special chars
    GM_SANITIZE_UNICODE = 1 << 2,     // ASCII-only (only if filesystem requires)
    GM_SANITIZE_LOWERCASE = 1 << 3,   // Force lowercase
    GM_SANITIZE_TRUNCATE = 1 << 4,    // Truncate to max length
    GM_SANITIZE_PRESERVE_UNICODE = 1 << 5,  // Default: keep Unicode intact
} gm_sanitize_options_t;

// Reserved filenames (Windows)
static const char* RESERVED_NAMES[] = {
    "CON", "PRN", "AUX", "NUL",
    "COM1", "COM2", "COM3", "COM4", "COM5", 
    "COM6", "COM7", "COM8", "COM9",
    "LPT1", "LPT2", "LPT3", "LPT4", "LPT5",
    "LPT6", "LPT7", "LPT8", "LPT9"
};
```

### 3.2 Shell Argument Sanitization

```c
// Sanitize for shell execution (if ever needed)
gm_result_string gm_sanitize_shell_arg(
    const char* input
);

// Quote for shell safety
gm_result_string gm_shell_quote(
    const char* input
);

// Escape special characters
gm_result_string gm_escape_special(
    const char* input,
    const char* special_chars
);
```

### 3.3 Identifier Validation

```c
// Validate identifier (variable names, etc)
gm_result_void gm_validate_identifier(
    const char* id,
    gm_identifier_rules_t* rules
);

typedef struct gm_identifier_rules {
    bool allow_unicode;         // Allow non-ASCII
    bool allow_dash;           // Allow hyphens
    bool allow_underscore;     // Allow underscores
    bool allow_dot;            // Allow dots
    size_t min_length;
    size_t max_length;
    const char* reserved_words[]; // Reserved identifiers
    size_t reserved_count;
} gm_identifier_rules_t;

// Common identifier patterns
extern const gm_identifier_rules_t GM_ID_RULES_C;      // C-style
extern const gm_identifier_rules_t GM_ID_RULES_URL;    // URL-safe
extern const gm_identifier_rules_t GM_ID_RULES_FILE;   // Filename-safe
```

## 4. Secure Memory Operations

### 4.1 Bounds-Checked Operations

```c
// core/include/gitmind/security/memory.h

// Safe string copy with bounds checking
gm_result_size gm_strlcpy(
    char* dst,
    const char* src,
    size_t dst_size
);

// Safe string concatenation
gm_result_size gm_strlcat(
    char* dst,
    const char* src,
    size_t dst_size
);

// Safe memory comparison (constant time)
bool gm_mem_equal_secure(
    const void* a,
    const void* b,
    size_t len
);

// Clear sensitive memory
void gm_mem_clear_secure(
    void* ptr,
    size_t len
);
```

### 4.2 Buffer Size Validation

```c
// Validate buffer sizes before operations
gm_result_void gm_validate_buffer_size(
    size_t required,
    size_t available
);

// Safe integer arithmetic (detect overflow)
gm_result_size gm_safe_add(size_t a, size_t b);
gm_result_size gm_safe_mul(size_t a, size_t b);
gm_result_int gm_safe_cast_size_to_int(size_t value);
```

## 5. Security Macros and Helpers

### 5.1 Validation Macros

```c
// Early return on validation failure
#define GM_VALIDATE(expr) \
    do { \
        gm_result_void _result = (expr); \
        if (GM_IS_ERR(_result)) { \
            return (typeof(_result)){ .ok = false, .u.err = GM_UNWRAP_ERR(_result) }; \
        } \
    } while(0)

// Validate with custom error
#define GM_VALIDATE_OR(expr, error_code, msg) \
    do { \
        if (!(expr)) { \
            return GM_ERR(GM_ERROR(error_code, msg)); \
        } \
    } while(0)

// Common validations
#define GM_VALIDATE_NOT_NULL(ptr) \
    GM_VALIDATE_OR((ptr) != NULL, GM_ERR_INVALID_ARGUMENT, \
                   #ptr " cannot be NULL")

#define GM_VALIDATE_STRING_NOT_EMPTY(str) \
    GM_VALIDATE_OR((str) && *(str), GM_ERR_INVALID_ARGUMENT, \
                   "String cannot be empty")

#define GM_VALIDATE_BUFFER_SIZE(required, available) \
    GM_VALIDATE_OR((required) <= (available), GM_ERR_BUFFER_TOO_SMALL, \
                   "Buffer too small")
```

### 5.2 Security Assertions

```c
// Security assertions (always enabled)
#define GM_SECURITY_ASSERT(expr) \
    do { \
        if (!(expr)) { \
            gm_security_violation(__FILE__, __LINE__, #expr); \
        } \
    } while(0)

// Log security violation and abort
void gm_security_violation(
    const char* file,
    int line,
    const char* expr
);
```

## 6. Usage Examples

### 6.1 Validating User Input

```c
gm_result_node_ptr create_node_from_input(const char* user_path) {
    // Validate UTF-8
    gm_validation_error_t error;
    GM_VALIDATE(gm_validate_utf8(user_path, strlen(user_path), &error));
    
    // Validate path
    GM_VALIDATE(gm_validate_path(user_path, &GM_PATH_RULES_USER, &error));
    
    // Canonicalize safely
    gm_result_string safe_path = gm_path_canonicalize_safe(
        user_path, &GM_PATH_RULES_USER
    );
    GM_TRY(safe_path);
    
    // Now safe to use
    return create_node_internal(GM_UNWRAP(safe_path));
}
```

### 6.2 Safe String Operations

```c
void format_message_safe(char* buffer, size_t size, 
                        const char* name, int count) {
    // Validate inputs
    GM_SECURITY_ASSERT(buffer != NULL);
    GM_SECURITY_ASSERT(size > 0);
    
    // Use bounds-checked formatting
    int written = snprintf(buffer, size, 
                          "Processing %s (%d items)", name, count);
    
    // Check for truncation
    if (written >= size) {
        // Handle truncation safely
        buffer[size - 1] = '\0';
        gm_log_warn("Message truncated");
    }
}
```

### 6.3 Path Traversal Prevention

```c
gm_result_void save_to_file(const char* filename, const void* data, size_t len) {
    // Validate filename (no paths allowed)
    gm_validation_error_t error;
    GM_VALIDATE(gm_validate_filename(filename, &error));
    
    // Build safe path
    char safe_path[GM_MAX_PATH_LENGTH];
    gm_result_size result = gm_strlcpy(safe_path, ".gitmind/", sizeof(safe_path));
    GM_TRY(result);
    
    gm_result_size result2 = gm_strlcat(safe_path, filename, sizeof(safe_path));
    GM_TRY(result2);
    
    // Verify still within boundary
    if (!gm_path_is_within(safe_path, ".gitmind")) {
        return GM_ERR(GM_ERROR(GM_ERR_INVALID_PATH, 
                              "Path escapes boundary"));
    }
    
    // Safe to write
    return write_file_internal(safe_path, data, len);
}
```

## 7. Security Checklist

When implementing any function that handles external input:

- [ ] Validate UTF-8 encoding
- [ ] Check length constraints  
- [ ] Prevent path traversal
- [ ] Sanitize for target context
- [ ] Use bounds-checked operations
- [ ] Handle errors explicitly
- [ ] Log security-relevant events
- [ ] Clear sensitive data

## 8. Performance Considerations

- Validation is fast (single pass where possible)
- Sanitization may allocate (acceptable for security)
- Constant-time comparison for sensitive data
- Cache validation results where safe

---

_"The best security bug is the one that can't exist in the first place."_
