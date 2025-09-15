---
title: git-mind C API Reference
description: Reference for public C API types and functions.
audience: [developers]
domain: [api]
tags: [api, c]
status: draft
last_updated: 2025-09-15
---

# git-mind C API Reference

Complete API documentation for the git-mind library (gitmind_lib.h).

## Table of Contents

- [Types and Structures](#types-and-structures)
- [Context Management](#context-management)
- [Repository Operations](#repository-operations)
- [Link Operations](#link-operations)
- [Graph Operations](#graph-operations)
- [Status and Maintenance](#status-and-maintenance)
- [Error Handling](#error-handling)
- [Link Set Management](#link-set-management)
- [Backend Interface](#backend-interface)

## Types and Structures

### Error Codes

```c
typedef enum {
    GM_OK = 0,                  // Success
    GM_ERR_NOT_REPO = -1,      // Not in a Git repository
    GM_ERR_NOT_FOUND = -2,     // Resource not found
    GM_ERR_IO = -3,            // I/O error
    GM_ERR_GIT = -4,           // Git operation failed
    GM_ERR_MEMORY = -5,        // Memory allocation failed
    GM_ERR_INVALID_ARG = -6,   // Invalid argument
    GM_ERR_PATH_TOO_LONG = -7, // Path exceeds maximum length
    GM_ERR_ALREADY_EXISTS = -8 // Resource already exists
} gm_error_t;
```

### Output Modes

```c
typedef enum {
    GM_OUTPUT_SILENT = 0,    // No output (default)
    GM_OUTPUT_VERBOSE = 1,   // Detailed human-readable output
    GM_OUTPUT_PORCELAIN = 2  // Machine-readable output
} gm_output_mode_t;
```

### Link Structure

```c
typedef struct gm_link {
    char source[4096];      // Source file path
    char target[4096];      // Target file path
    char type[64];          // Relationship type
    time_t timestamp;       // Creation timestamp
} gm_link_t;
```

### Link Set Structure

```c
typedef struct gm_link_set {
    gm_link_t* links;       // Array of links
    size_t count;           // Number of links
    size_t capacity;        // Allocated capacity
} gm_link_set_t;
```

## Context Management

### gm_create_context

```c
gm_context_t* gm_create_context(void);
```

Creates a new git-mind context. All operations require a context.

__Returns:__

- Valid pointer on success
- NULL on failure (out of memory)

__Example:__

```c
gm_context_t* ctx = gm_create_context();
if (!ctx) {
    fprintf(stderr, "Failed to create context\n");
    return 1;
}
```

### gm_destroy_context

```c
void gm_destroy_context(gm_context_t* ctx);
```

Frees a context and all associated resources.

__Parameters:__

- `ctx` - Context to destroy (can be NULL)

__Example:__

```c
gm_destroy_context(ctx);
ctx = NULL;  // Good practice
```

### gm_set_output_mode

```c
void gm_set_output_mode(gm_context_t* ctx, gm_output_mode_t mode);
```

Sets the output mode for operations.

__Parameters:__

- `ctx` - The context
- `mode` - Output mode (silent, verbose, or porcelain)

__Example:__

```c
gm_set_output_mode(ctx, GM_OUTPUT_VERBOSE);  // Enable detailed output
```

## Repository Operations

### gm_init

```c
int gm_init(gm_context_t* ctx);
```

Initializes git-mind in the current repository by creating the orphan ref `refs/gitmind/graph`.

__Parameters:__

- `ctx` - The context

__Returns:__

- `GM_OK` - Successfully initialized
- `GM_ERR_NOT_REPO` - Not in a Git repository
- `GM_ERR_ALREADY_EXISTS` - Already initialized
- `GM_ERR_GIT` - Git operation failed

__Example:__

```c
int ret = gm_init(ctx);
if (ret == GM_ERR_ALREADY_EXISTS) {
    printf("Already initialized\n");
} else if (ret != GM_OK) {
    fprintf(stderr, "Failed: %s\n", gm_last_error(ctx));
}
```

## Link Operations

### gm_link_create

```c
int gm_link_create(gm_context_t* ctx, const char* source, 
                   const char* target, const char* type);
```

Creates a semantic link between two files.

__Parameters:__

- `ctx` - The context
- `source` - Path to source file (must exist)
- `target` - Path to target file (must exist)
- `type` - Relationship type (e.g., "implements", "references", "tests")

__Returns:__

- `GM_OK` - Link created successfully
- `GM_ERR_NOT_REPO` - Not initialized
- `GM_ERR_NOT_FOUND` - Source or target file not found
- `GM_ERR_INVALID_ARG` - Invalid path or type
- `GM_ERR_GIT` - Git operation failed

__Notes:__

- Files must exist in the working directory
- Paths must be relative (no absolute paths)
- Type is case-insensitive and normalized internally
- Creates a CBOR edge in the Git object database

__Example:__

```c
int ret = gm_link_create(ctx, "src/main.c", "docs/api.md", "implements");
if (ret != GM_OK) {
    fprintf(stderr, "Failed to create link: %s\n", gm_last_error(ctx));
}
```

### gm_link_list

```c
int gm_link_list(gm_context_t* ctx, gm_link_set_t** out_set,
                 const char* filter_source, const char* filter_target);
```

Lists links, optionally filtered by source and/or target.

__Parameters:__

- `ctx` - The context
- `out_set` - Output parameter for the link set (must be freed)
- `filter_source` - Optional source filter (NULL for all)
- `filter_target` - Optional target filter (NULL for all)

__Returns:__

- `GM_OK` - Success (even if no links found)
- `GM_ERR_NOT_REPO` - Not initialized
- `GM_ERR_MEMORY` - Memory allocation failed
- `GM_ERR_GIT` - Git operation failed

__Notes:__

- Tombstoned links (negative confidence) are automatically filtered
- Caller must free the link set with `gm_link_set_free()`
- Empty repository returns empty set, not an error

__Example:__

```c
gm_link_set_t* links = NULL;

// List all links
int ret = gm_link_list(ctx, &links, NULL, NULL);

// List links from specific source
ret = gm_link_list(ctx, &links, "README.md", NULL);

// List links between specific files
ret = gm_link_list(ctx, &links, "src/main.c", "docs/api.md");

if (ret == GM_OK) {
    for (size_t i = 0; i < links->count; i++) {
        printf("%s -> %s [%s]\n",
               links->links[i].source,
               links->links[i].target,
               links->links[i].type);
    }
    gm_link_set_free(links);
}
```

### gm_link_unlink

```c
int gm_link_unlink(gm_context_t* ctx, const char* source, const char* target);
```

Removes a link between two files by creating a tombstone.

__Parameters:__

- `ctx` - The context
- `source` - Source file path
- `target` - Target file path

__Returns:__

- `GM_OK` - Link removed (tombstone created)
- `GM_ERR_NOT_REPO` - Not initialized
- `GM_ERR_NOT_FOUND` - Link doesn't exist
- `GM_ERR_GIT` - Git operation failed

__Notes:__

- Creates a tombstone edge with confidence = -1.0
- Original edge is preserved for history
- Link will no longer appear in listings
- Can be "resurrected" by creating a new positive edge

__Example:__

```c
int ret = gm_link_unlink(ctx, "README.md", "docs/setup.md");
if (ret == GM_ERR_NOT_FOUND) {
    printf("Link not found\n");
} else if (ret != GM_OK) {
    fprintf(stderr, "Failed: %s\n", gm_last_error(ctx));
}
```

## Graph Operations

### gm_traverse

```c
typedef void (*gm_traverse_callback_t)(const gm_link_t* link, int level, void* userdata);

int gm_traverse(gm_context_t* ctx, const char* start_file, int depth,
                gm_traverse_callback_t callback, void* userdata);
```

Traverses the link graph from a starting point using breadth-first search.

__Parameters:__

- `ctx` - The context
- `start_file` - Starting file for traversal
- `depth` - Maximum depth (0 = use default of 3, max = 10)
- `callback` - Function called for each discovered link (can be NULL)
- `userdata` - Passed to callback

__Returns:__

- `GM_OK` - Traversal completed
- `GM_ERR_NOT_REPO` - Not initialized
- `GM_ERR_INVALID_ARG` - Invalid arguments or depth > 10
- `GM_ERR_MEMORY` - Memory allocation failed

__Callback Parameters:__

- `link` - The discovered link
- `level` - Depth level (1 = direct link, 2 = second level, etc.)
- `userdata` - User-provided data

__Notes:__

- Uses breadth-first search with cycle detection
- Visits each node at most once
- Tombstoned links are automatically skipped
- Callback is not called for the starting node

__Example:__

```c
void print_tree(const gm_link_t* link, int level, void* userdata) {
    for (int i = 0; i < level; i++) {
        printf("  ");
    }
    printf("└─ %s -> %s [%s]\n", link->source, link->target, link->type);
}

int ret = gm_traverse(ctx, "README.md", 3, print_tree, NULL);
```

## Status and Maintenance

### gm_get_status

```c
int gm_get_status(gm_context_t* ctx, int* out_total_links);
```

Gets repository status information.

__Parameters:__

- `ctx` - The context
- `out_total_links` - Output parameter for total link count (can be NULL)

__Returns:__

- `GM_OK` - Success
- `GM_ERR_NOT_REPO` - Not initialized

__Example:__

```c
int total_links = 0;
if (gm_get_status(ctx, &total_links) == GM_OK) {
    printf("Total links: %d\n", total_links);
}
```

### gm_check

```c
int gm_check(gm_context_t* ctx, int fix);
```

Checks repository integrity and finds broken links.

__Parameters:__

- `ctx` - The context
- `fix` - If non-zero, attempt to fix issues (not fully implemented)

__Returns:__

- `GM_OK` - No issues found
- `GM_ERR_NOT_REPO` - Not initialized
- Other - Number of issues found

__Notes:__

- Identifies links to non-existent files
- Fix mode is not fully implemented in current version
- Reports issues based on output mode setting

__Example:__

```c
gm_set_output_mode(ctx, GM_OUTPUT_VERBOSE);
int issues = gm_check(ctx, 0);
if (issues > 0) {
    printf("Found %d broken links\n", issues);
}
```

## Error Handling

### gm_last_error

```c
const char* gm_last_error(gm_context_t* ctx);
```

Gets the last error message as a human-readable string.

__Parameters:__

- `ctx` - The context

__Returns:__

- Error message string (never NULL)
- "Unknown error" if no context

__Notes:__

- String is owned by context, do not free
- Valid until next operation on context

__Example:__

```c
if (gm_init(ctx) != GM_OK) {
    fprintf(stderr, "Error: %s\n", gm_last_error(ctx));
}
```

## Link Set Management

### gm_link_set_new

```c
gm_link_set_t* gm_link_set_new(void);
```

Creates a new empty link set.

__Returns:__

- Valid pointer on success
- NULL on memory allocation failure

### gm_link_set_free

```c
void gm_link_set_free(gm_link_set_t* set);
```

Frees a link set and all its links.

__Parameters:__

- `set` - Link set to free (can be NULL)

### gm_link_set_add

```c
int gm_link_set_add(gm_link_set_t* set, const gm_link_t* link);
```

Adds a link to a link set.

__Parameters:__

- `set` - The link set
- `link` - Link to add (copied)

__Returns:__

- `GM_OK` - Added successfully
- `GM_ERR_MEMORY` - Memory allocation failed

## Backend Interface

The backend interface allows different storage implementations. Currently, libgit2 is the primary backend.

### Backend Operations Structure

```c
typedef struct gm_backend_ops {
    // Repository operations
    int (*open_repo)(void* backend_data, const char* path, void** out_handle);
    void (*close_repo)(void* backend_data, void* handle);
    
    // Object operations
    int (*hash_object)(void* backend_data, const void* data, size_t len, 
                       const char* type, char* out_sha);
    int (*read_object)(void* backend_data, const char* sha, void* out_data, 
                       size_t max_size, size_t* actual_size);
    
    // Tree operations
    int (*read_tree)(void* backend_data, const char* tree_sha, char* out_entries);
    int (*write_tree)(void* backend_data, const char* entries, char* out_sha);
    
    // Reference operations
    int (*read_ref)(void* backend_data, const char* ref_name, char* out_sha);
    int (*write_ref)(void* backend_data, const char* ref_name, 
                     const char* sha, const char* message);
    
    // Notes operations
    int (*read_note)(void* backend_data, const char* notes_ref, 
                     const char* object_sha, char* out_note, size_t max_size);
    int (*write_note)(void* backend_data, const char* notes_ref, 
                      const char* object_sha, const char* note);
} gm_backend_ops_t;
```

### Available Backends

1. __libgit2__ - Production backend using libgit2
2. __test__ - Mock backend for unit testing

## Best Practices

1. __Always check return values__ - Every function can fail
2. __Free resources__ - Use `gm_link_set_free()` and `gm_destroy_context()`
3. __Use filters wisely__ - Filter at query time, not after
4. __Handle tombstones__ - Unlinked items won't appear in lists
5. __Respect depth limits__ - Traverse depth is capped at 10
6. __Use appropriate output modes__ - Verbose for humans, porcelain for scripts

## Thread Safety

git-mind is NOT thread-safe. Each thread should have its own context.

## Memory Management

- Contexts own their error messages
- Link sets must be freed by caller
- All strings are copied - originals can be freed
- Backend manages Git object lifetime

## Version Information

```c
const char* gm_version_string(void);  // Returns "0.1.0"
```

---

_For implementation details and architecture, see [Architecture Overview](../architecture/README.md)_
