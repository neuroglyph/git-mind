# Specification: Domain Types and Strong Typedefs

> *"A type system is a tractable syntactic method for proving the absence of certain program behaviors by classifying phrases according to the kinds of values they compute."* - Benjamin Pierce

## Overview

This specification defines the core domain types for git-mind, using strong typedefs to prevent type confusion and make invalid states unrepresentable.

## 1. Identity Types

### 1.1 Base ID Type

```c
// core/include/gitmind/types/id.h

// 256-bit identifier (SHA-256 based, big-endian)
typedef struct gm_id {
    uint8_t bytes[32];  // Big-endian SHA-256 digest
} gm_id_t;

// ID operations
bool gm_id_equal(gm_id_t a, gm_id_t b);
int gm_id_compare(gm_id_t a, gm_id_t b);
uint32_t gm_id_hash(gm_id_t id);

// ID creation
gm_id_t gm_id_from_data(const void* data, size_t len);
gm_id_t gm_id_from_string(const char* str);
gm_id_t gm_id_generate(void);  // Random ID

// ID conversion
void gm_id_to_hex(gm_id_t id, char out[65]);
GM_RESULT(gm_id_t) gm_id_from_hex(const char* hex);
```

### 1.2 Typed IDs (Strong Typedefs)

```c
// Distinct types prevent mixing IDs
typedef struct { gm_id_t base; } gm_node_id_t;
typedef struct { gm_id_t base; } gm_edge_id_t;
typedef struct { gm_id_t base; } gm_graph_id_t;
typedef struct { gm_id_t base; } gm_session_id_t;
typedef struct { gm_id_t base; } gm_commit_id_t;

// Type-safe operations (compiler enforced!)
static inline bool gm_node_id_equal(gm_node_id_t a, gm_node_id_t b) {
    return gm_id_equal(a.base, b.base);
}

static inline bool gm_edge_id_equal(gm_edge_id_t a, gm_edge_id_t b) {
    return gm_id_equal(a.base, b.base);
}

// Won't compile - different types!
// bool same = gm_node_id_equal(node_id, edge_id);  // ERROR!
```

### 1.3 ID Generation Rules

```c
// Node ID: SHA256(path)
gm_node_id_t gm_node_id_from_path(gm_path_t path);

// Edge ID: SHA256(source || target || type)
gm_edge_id_t gm_edge_id_from_triple(
    gm_node_id_t source,
    gm_node_id_t target, 
    gm_edge_type_t type
);

// Session ID: Random UUID v4
gm_session_id_t gm_session_id_new(void);
```

## 2. String Types

### 2.1 Safe String Type

```c
// core/include/gitmind/types/string.h

// Owned string with explicit length
typedef struct gm_string {
    char* data;          // UTF-8 data (null-terminated)
    size_t length;       // Length in bytes (excluding null)
    size_t capacity;     // Allocated capacity
} gm_string_t;

// Non-owning string view (separate type for clarity)
typedef struct gm_string_view {
    const char* data;    // Points to existing string
    size_t length;       // Length in bytes
} gm_string_view_t;

// String creation (always owned)
GM_RESULT_DEF(gm_result_string, gm_string_t);
gm_result_string gm_string_new(const char* str);
gm_result_string gm_string_new_n(const char* str, size_t len);
gm_result_string gm_string_from_owned(char* str, size_t len, size_t capacity);

// String view creation (never owns)
gm_string_view_t gm_string_view(const char* str);
gm_string_view_t gm_string_view_n(const char* str, size_t len);

// String operations (all bounds-checked)
gm_result_string gm_string_copy(gm_string_t str);
gm_result_string gm_string_concat(gm_string_t a, gm_string_t b);
gm_result_string gm_string_substring(gm_string_t str, size_t start, size_t len);

// String validation
gm_result_void gm_string_validate_utf8(gm_string_t str);
bool gm_string_is_empty(gm_string_t str);

// String cleanup (only for owned strings)
void gm_string_free(gm_string_t str);
```

### 2.2 Path Type

```c
// core/include/gitmind/types/path.h

// Path with validation state
typedef struct gm_path {
    gm_string_t value;       // The path string
    char separator;          // Path separator ('/' or '\\')
    bool is_absolute;        // Absolute vs relative
    gm_path_state_t state;   // RAW or CANONICAL
    bool is_validated;       // Passed validation
    gm_path_type_t type;     // File, directory, etc.
} gm_path_t;

typedef enum {
    GM_PATH_STATE_RAW,       // As provided
    GM_PATH_STATE_CANONICAL  // Canonicalized
} gm_path_state_t;

typedef enum {
    GM_PATH_TYPE_UNKNOWN,
    GM_PATH_TYPE_FILE,
    GM_PATH_TYPE_DIRECTORY,
    GM_PATH_TYPE_SYMLINK,
    GM_PATH_TYPE_URL,
    GM_PATH_TYPE_IDENTIFIER
} gm_path_type_t;

// Define result type
GM_RESULT_DEF(gm_result_path, gm_path_t);

// Path creation with validation
gm_result_path gm_path_new(const char* str);
gm_result_path gm_path_from_string(gm_string_t str);

// Path operations
gm_result_path gm_path_join(gm_path_t base, gm_path_t relative);
gm_result_path gm_path_dirname(gm_path_t path);
gm_result_path gm_path_basename(gm_path_t path);
gm_result_path gm_path_canonicalize(gm_path_t path);

// Path validation
gm_result_void gm_path_validate(gm_path_t path, gm_path_rules_t* rules);
bool gm_path_is_safe(gm_path_t path);  // No traversal, etc.
```

## 3. Hash Types

### 3.1 Content Hash

```c
// core/include/gitmind/types/hash.h

typedef enum {
    GM_HASH_SHA256,
    GM_HASH_SHA1,     // For Git compatibility
    GM_HASH_BLAKE3    // Future
} gm_hash_algorithm_t;

// Generic hash type
typedef struct gm_hash {
    uint8_t bytes[32];              // Max size (SHA256)
    uint8_t length;                 // Actual length
    gm_hash_algorithm_t algorithm;
} gm_hash_t;

// Typed hashes for different purposes
typedef struct { gm_hash_t base; } gm_content_hash_t;
typedef struct { gm_hash_t base; } gm_commit_hash_t;
typedef struct { gm_hash_t base; } gm_tree_hash_t;

// Hash operations
gm_content_hash_t gm_content_hash_compute(const void* data, size_t len);
GM_RESULT(gm_content_hash_t) gm_content_hash_from_file(gm_path_t path);
```

## 4. Time Types

### 4.1 Timestamp

```c
// core/include/gitmind/types/time.h

// High-precision timestamp with timezone
typedef struct gm_timestamp {
    int64_t seconds_since_epoch;    // Unix timestamp
    uint32_t nanoseconds;           // Additional precision
    int16_t timezone_offset_minutes; // Timezone offset
} gm_timestamp_t;

// Time operations
gm_timestamp_t gm_timestamp_now(void);
gm_timestamp_t gm_timestamp_from_unix(time_t unix_time);
int64_t gm_timestamp_diff_seconds(gm_timestamp_t a, gm_timestamp_t b);

// Time formatting
GM_RESULT(gm_string_t) gm_timestamp_format_iso8601(gm_timestamp_t ts);
GM_RESULT(gm_timestamp_t) gm_timestamp_parse_iso8601(const char* str);
```

## 5. Domain Value Types

### 5.1 Confidence Score

```c
// core/include/gitmind/types/confidence.h

// Confidence score with explanation
typedef struct gm_confidence {
    double value;              // 0.0 to 1.0
    gm_string_t explanation;   // Why this confidence?
    gm_source_t source;        // Who assigned it?
} gm_confidence_t;

// Confidence constructors
gm_confidence_t gm_confidence_certain(void);      // 1.0
gm_confidence_t gm_confidence_high(void);         // 0.9
gm_confidence_t gm_confidence_medium(void);       // 0.7
gm_confidence_t gm_confidence_low(void);          // 0.5
gm_confidence_t gm_confidence_guess(void);        // 0.3

// Validation
GM_RESULT(gm_confidence_t) gm_confidence_new(double value, const char* reason);
```

### 5.2 Version

```c
// core/include/gitmind/types/version.h

// Semantic version
typedef struct gm_version {
    uint32_t major;
    uint32_t minor;
    uint32_t patch;
    gm_string_t prerelease;  // Optional
    gm_string_t build;       // Optional
} gm_version_t;

// Version operations
int gm_version_compare(gm_version_t a, gm_version_t b);
GM_RESULT(gm_version_t) gm_version_parse(const char* str);
GM_RESULT(gm_string_t) gm_version_format(gm_version_t v);
```

## 6. Collection Types

### 6.1 Type-Safe Lists

```c
// core/include/gitmind/types/collections.h

// Generic list macro
#define GM_LIST_TYPE(T) \
    struct { \
        T* items; \
        size_t count; \
        size_t capacity; \
    }

// Common list types
typedef GM_LIST_TYPE(gm_node_id_t) gm_node_id_list_t;
typedef GM_LIST_TYPE(gm_edge_id_t) gm_edge_id_list_t;
typedef GM_LIST_TYPE(gm_path_t) gm_path_list_t;
typedef GM_LIST_TYPE(gm_string_t) gm_string_list_t;

// Type-safe operations
GM_RESULT(void) gm_node_id_list_append(gm_node_id_list_t* list, gm_node_id_t id);
GM_RESULT(gm_node_id_t) gm_node_id_list_get(gm_node_id_list_t* list, size_t index);
void gm_node_id_list_free(gm_node_id_list_t* list);
```

## 7. Usage Examples

### 7.1 Creating a Node

```c
GM_RESULT_DEF(gm_result_node_ptr, gm_node_t*);

gm_result_node_ptr create_file_node(const char* file_path) {
    // Parse and validate path
    gm_result_path path_result = gm_path_new(file_path);
    GM_TRY(path_result);
    gm_path_t path = GM_UNWRAP(path_result);
    
    // Generate ID from path
    gm_node_id_t id = gm_node_id_from_path(path);
    
    // Create node with typed fields
    gm_node_t* node = malloc(sizeof(gm_node_t));
    node->id = id;
    node->path = path;
    node->type = GM_NODE_TYPE_FILE;
    node->created_at = gm_timestamp_now();
    
    return gm_ok_ptr(node);
}
```

### 7.2 Type Safety in Action

```c
void process_graph(void) {
    gm_node_id_t node_id = /* ... */;
    gm_edge_id_t edge_id = /* ... */;
    
    // This compiles
    bool same_node = gm_node_id_equal(node_id, node_id);
    
    // This won't compile - type mismatch!
    // bool mixed = gm_node_id_equal(node_id, edge_id);  // ERROR!
    
    // Must explicitly convert if needed
    bool same_base = gm_id_equal(node_id.base, edge_id.base);
}
```

## 8. Design Principles

### 8.1 Zero-Cost Abstractions
- Types compile to same assembly as raw primitives
- No runtime overhead for type safety
- Inline functions for common operations

### 8.2 Make Invalid States Unrepresentable
- Can't create invalid IDs
- Can't mix different ID types  
- Paths are always validated
- Strings track their length

### 8.3 Explicit Over Implicit
- Ownership is explicit (owned flag)
- Validation state tracked
- No hidden allocations
- Clear error propagation

## 9. Memory Management Rules

1. **Creators own**: Functions that create types own the memory
2. **Explicit transfer**: Ownership transfer must be documented
3. **Views don't own**: `gm_string_view_t` never frees memory
4. **Lists own items**: Collections own their contents
5. **Paths own strings**: Composite types own their parts
6. **No ambiguity**: Separate owned vs view types prevents confusion

---

*"Strong types are like seat belts - slightly annoying until they save your life."*