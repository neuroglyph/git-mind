# How git-mind Works: The Complete Guide

## Table of Contents
1. [Overview](#overview)
2. [The Holy Grail Architecture](#the-holy-grail-architecture)
3. [Tombstones Explained](#tombstones-explained)
4. [Core Concepts](#core-concepts)
5. [Implementation Details](#implementation-details)
6. [API Reference](#api-reference)

## Overview

git-mind is a semantic link tracker that stores relationships between files directly in Git. Instead of using external databases or files in your working directory, it leverages Git's object model to create a parallel universe of knowledge.

```mermaid
graph TD
    A[Your Files] -->|git-mind link| B[Semantic Graph]
    B -->|Stored in| C[Git Objects]
    C -->|Referenced by| D[refs/gitmind/graph]
    
    style A fill:#e1f5fe
    style B fill:#fff3e0
    style C fill:#f3e5f5
    style D fill:#c8e6c9
```

## The Holy Grail Architecture

The "Holy Grail" refers to storing everything as pure Git objects - no files in your working directory, no merge conflicts, just trees and blobs.

```mermaid
gitGraph
    commit id: "main branch"
    commit id: "your code"
    commit id: "more code"
    
    branch refs/gitmind/graph
    checkout refs/gitmind/graph
    commit id: "link: A→B" type: HIGHLIGHT
    commit id: "link: B→C" type: HIGHLIGHT
    commit id: "unlink: A→B" type: REVERSE
```

### Why is this the Holy Grail?

1. **No Working Directory Pollution** - Your `.git` directory contains everything
2. **No Merge Conflicts** - Each edge has a unique path
3. **Git-Native Compression** - Automatic deduplication and compression
4. **History Preserved** - Every change is tracked
5. **Distributed by Design** - Works with all Git workflows

## Tombstones Explained

### What are Tombstones?

In traditional systems, when you delete something, it's gone. In git-mind, we use "tombstones" - markers that say "this link was removed" without actually deleting the history.

```mermaid
sequenceDiagram
    participant User
    participant git-mind
    participant Git Graph
    
    User->>git-mind: link A.md B.md
    git-mind->>Git Graph: Create edge (confidence: 1.0)
    Note over Git Graph: Edge exists, shows in list
    
    User->>git-mind: unlink A.md B.md
    git-mind->>Git Graph: Create tombstone (confidence: -1.0)
    Note over Git Graph: Both edges exist, but list skips negative
    
    User->>git-mind: list
    git-mind->>Git Graph: Walk tree, find edges
    Git Graph-->>git-mind: Skip negative confidence
    git-mind-->>User: Link not shown (appears deleted)
```

### Why Tombstones?

1. **History Preservation** - See when links were removed
2. **Resurrection** - Can "undelete" by adding a new positive edge
3. **Audit Trail** - Know who removed what and when
4. **Distributed Consistency** - Tombstones merge cleanly

### Tombstone Implementation

```mermaid
graph LR
    A[Regular Edge] -->|confidence: 1.0| B[Shows in list]
    C[Tombstone Edge] -->|confidence: -1.0| D[Hidden from list]
    
    style A fill:#90EE90
    style C fill:#FFB6C1
```

## Core Concepts

### 1. Content-Addressable Storage

Every file is identified by its content SHA, not its path:

```mermaid
graph TD
    F1[README.md] -->|SHA: abc123| S1[Source Node]
    F2[docs/api.md] -->|SHA: def456| S2[Target Node]
    S1 -->|Edge| S2
    
    F1 -->|git mv README.txt| F3[README.txt]
    F3 -->|Same SHA: abc123| S1
    
    Note1[Path changes, SHA stays same]
    Note2[Links follow content, not paths]
    
    style F1 fill:#ffebee
    style F3 fill:#e8f5e9
    style Note1 fill:#fff9c4
    style Note2 fill:#fff9c4
```

### 2. Double Fan-out Structure

To handle millions of edges efficiently:

```mermaid
graph TD
    Root[refs/gitmind/graph]
    Root --> F1[ab/]
    F1 --> F2[cd/]
    F2 --> SHA[abcdef.../]
    SHA --> Rel[implements/]
    Rel --> E1[01/]
    E1 --> E2[23/]
    E2 --> Edge1[ULID1.cbor]
    E2 --> Edge2[ULID2.cbor]
    
    style Root fill:#4CAF50
    style Edge1 fill:#FF9800
    style Edge2 fill:#FF9800
```

### 3. CBOR Edge Format

Each edge is a compact binary object:

```mermaid
graph TD
    CBOR[CBOR Edge Blob]
    CBOR --> Target[Target SHA<br/>20 bytes]
    CBOR --> Conf[Confidence<br/>2 bytes float]
    CBOR --> Time[Timestamp<br/>8 bytes]
    CBOR --> Extra[Extra Data<br/>optional]
    
    Size[Total: ~30 bytes]
    
    style CBOR fill:#E3F2FD
    style Size fill:#C5E1A5
```

### 4. Edge Types

Edges are grouped by relationship type:

```mermaid
graph LR
    Source[parser.c] --> Impl[implements/] --> Grammar[grammar.md]
    Source --> Ref[references/] --> RFC[rfc1234.txt]
    Source --> Test[tests/] --> TestFile[test_parser.c]
    
    style Impl fill:#FFCDD2
    style Ref fill:#F8BBD0
    style Test fill:#E1BEE7
```

## Implementation Details

### Creating a Link

```mermaid
sequenceDiagram
    participant CLI
    participant Core
    participant Backend
    participant Git
    
    CLI->>Core: gm_link_create(ctx, "A.md", "B.md", "implements")
    Core->>Core: Validate paths
    Core->>Core: Check files exist
    Core->>Backend: hash_object("A.md")
    Backend->>Git: git_odb_hash()
    Git-->>Backend: SHA: abc123
    Backend-->>Core: SHA: abc123
    
    Core->>Core: Generate ULID
    Core->>Core: Encode CBOR edge
    Core->>Backend: hash_object(cbor_data)
    Backend->>Git: git_odb_write()
    Git-->>Backend: Edge SHA
    
    Core->>Core: Build tree path
    Note over Core: ab/cd/abc123/implements/01/23/ULID
    
    Core->>Backend: Merge into tree
    Core->>Git: Update refs/gitmind/graph
    Git-->>CLI: Success
```

### Listing Links

```mermaid
sequenceDiagram
    participant CLI
    participant Core
    participant TreeWalker
    participant CBOR
    
    CLI->>Core: gm_link_list(ctx, &links, source, target)
    Core->>TreeWalker: walk_tree_for_links()
    
    loop For each tree entry
        TreeWalker->>TreeWalker: Is it 26 chars? (ULID)
        alt Is ULID
            TreeWalker->>CBOR: Decode edge
            CBOR-->>TreeWalker: target, confidence, timestamp
            
            alt Confidence > 0
                TreeWalker->>TreeWalker: Add to results
            else Confidence < 0 (Tombstone)
                TreeWalker->>TreeWalker: Skip
            end
        end
    end
    
    TreeWalker-->>Core: Link set
    Core-->>CLI: Links (tombstones filtered out)
```

### Traversing the Graph

```mermaid
graph TD
    Start[README.md] -->|BFS| Queue[Queue]
    Queue --> Process[Process Node]
    Process --> GetLinks[Get all links from node]
    GetLinks --> Check{Already visited?}
    Check -->|No| AddQueue[Add to queue]
    Check -->|Yes| Skip[Skip]
    AddQueue --> Queue
    
    Note[Breadth-first prevents infinite loops]
    
    style Start fill:#4CAF50
    style Note fill:#FFF9C4
```

## API Reference

### Context Management

#### `gm_create_context()`
Creates a new git-mind context for all operations.

```c
gm_context_t* ctx = gm_create_context();
if (!ctx) {
    // Handle error
}
```

**Returns**: Pointer to context or NULL on failure

#### `gm_destroy_context()`
Frees a context and all associated resources.

```c
gm_destroy_context(ctx);
```

### Repository Operations

#### `gm_init()`
Initializes git-mind in a repository by creating refs/gitmind/graph.

```c
int ret = gm_init(ctx);
if (ret != GM_OK) {
    fprintf(stderr, "Error: %s\n", gm_last_error(ctx));
}
```

**Returns**: 
- `GM_OK` - Success
- `GM_ERR_NOT_REPO` - Not in a Git repository
- `GM_ERR_ALREADY_EXISTS` - Already initialized

### Link Operations

#### `gm_link_create()`
Creates a semantic link between two files.

```c
int ret = gm_link_create(ctx, "src/main.c", "docs/api.md", "implements");
```

**Parameters**:
- `ctx` - The context
- `source` - Source file path (must exist)
- `target` - Target file path (must exist)
- `type` - Relationship type (e.g., "implements", "references", "tests")

**Returns**:
- `GM_OK` - Success
- `GM_ERR_NOT_FOUND` - File not found
- `GM_ERR_INVALID_ARG` - Invalid path or type

#### `gm_link_list()`
Lists all links, optionally filtered by source/target.

```c
gm_link_set_t* links = NULL;
int ret = gm_link_list(ctx, &links, "src/main.c", NULL);
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

**Parameters**:
- `ctx` - The context
- `out_set` - Output parameter for link set
- `filter_source` - Optional source filter (NULL for all)
- `filter_target` - Optional target filter (NULL for all)

**Note**: Tombstoned links are automatically filtered out.

#### `gm_link_unlink()`
Removes a link by creating a tombstone.

```c
int ret = gm_link_unlink(ctx, "src/main.c", "docs/api.md");
```

**Behavior**: 
- Creates a tombstone edge with negative confidence
- The link will no longer appear in listings
- History is preserved - the original edge still exists

### Graph Operations

#### `gm_traverse()`
Traverses the graph from a starting point.

```c
void print_callback(const gm_link_t* link, int level, void* userdata) {
    for (int i = 0; i < level; i++) printf("  ");
    printf("└─ %s -> %s [%s]\n", link->source, link->target, link->type);
}

int ret = gm_traverse(ctx, "README.md", 3, print_callback, NULL);
```

**Parameters**:
- `ctx` - The context
- `start_file` - Starting file for traversal
- `depth` - Maximum depth (default: 3, max: 10)
- `callback` - Function called for each discovered link
- `userdata` - Passed to callback

**Algorithm**: Breadth-first search with cycle detection

### Status Operations

#### `gm_status()`
Shows repository status including link counts.

```c
int total_links = 0;
int ret = gm_get_status(ctx, &total_links);
printf("Total links: %d\n", total_links);
```

### Error Handling

#### `gm_last_error()`
Gets the last error message as a string.

```c
const char* error = gm_last_error(ctx);
fprintf(stderr, "Error: %s\n", error);
```

### Output Modes

#### `gm_set_output_mode()`
Controls output verbosity.

```c
gm_set_output_mode(ctx, GM_OUTPUT_VERBOSE);  // Detailed output
gm_set_output_mode(ctx, GM_OUTPUT_SILENT);   // No output (default)
gm_set_output_mode(ctx, GM_OUTPUT_PORCELAIN); // Machine-readable
```

## Advanced Topics

### Confidence Decay (Future)
Links could weaken over time if not reinforced:

```mermaid
graph LR
    New[New Link<br/>confidence: 1.0] -->|6 months| Old[Old Link<br/>confidence: 0.7]
    Old -->|1 year| Weak[Weak Link<br/>confidence: 0.3]
    Weak -->|2 years| Gone[Hidden<br/>confidence: 0.1]
    
    style New fill:#4CAF50
    style Old fill:#FFC107
    style Weak fill:#FF9800
    style Gone fill:#F44336
```

### Bidirectional Links (Future)
Currently links are unidirectional. Future versions could add reverse indexes:

```mermaid
graph TD
    Forward[Source → Target] -->|Also creates| Reverse[Target ← Source]
    Reverse -->|Enables| Query[What links TO this file?]
    
    style Forward fill:#2196F3
    style Reverse fill:#9C27B0
```

## Summary

git-mind achieves its goals through:

1. **Pure Git Storage** - Everything is trees and blobs
2. **Content Addressing** - Links follow content, not paths
3. **Tombstone Deletion** - History preserved, behavior correct
4. **Efficient Structure** - Double fan-out for O(1) operations
5. **Clean Architecture** - Dependency injection throughout

The result is a semantic graph that:
- Lives entirely in Git
- Scales to millions of edges
- Preserves complete history
- Merges without conflicts
- Works offline
- Follows Git's distributed model

*The graph breathes. The edges live. Understanding accumulates.*