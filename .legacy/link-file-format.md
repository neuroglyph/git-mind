# Git-Mind Link File Format Specification

## Overview

Git-Mind stores semantic relationships between files using link files with the `.gml` (Git-Mind Link) extension. Each link file represents ALL relationships between a specific pair of nodes, supporting multiple edges with different types, authors, and metadata.

## File Naming

Link files are named using a SHA-256 hash of the sorted source and target blob SHAs:

```
SHA256(sort(source_blob_sha, target_blob_sha)).gml
```

This ensures:
- One file per unique node pair (regardless of direction)
- Consistent naming across repositories
- Content-addressed storage

## File Format

Link files use a simple line-based format optimized for performance:

```
GMv1
abc123def456|789012345678|src/parser.c|docs/grammar.md
+implements|alice@example.com|1234567890|0.9
+references|bob@example.com|1234567891|0.7
-documents|alice@example.com|1234567892|0.0|Moved to architecture.md
```

### Format Structure

1. **Line 1**: Magic header `GMv1` (format version)
2. **Line 2**: Node info: `out_sha|in_sha|out_path|in_path`
3. **Lines 3+**: Edges, one per line
   - `+` prefix = active edge
   - `-` prefix = tombstoned edge
   - Fields separated by `|`

### Edge Format

**Active edge**: `+type|author|timestamp|confidence`
**Tombstone**: `-type|author|timestamp|0.0|reason`

## Field Definitions

### Line 1: Format Version
- `GMv1`: Magic string identifying format version

### Line 2: Node Information  
- `out_sha`: Git blob SHA of source/outgoing node
- `in_sha`: Git blob SHA of target/incoming node
- `out_path`: Path of source (for human reference)
- `in_path`: Path of target (for human reference)

### Lines 3+: Edges
- **Prefix**: `+` for active, `-` for tombstone
- **Field 1**: Edge type (implements, references, etc.)
- **Field 2**: Author (from git config)
- **Field 3**: Unix timestamp
- **Field 4**: Confidence (0.0-1.0)
- **Field 5**: (tombstones only) Deletion reason

## Semantics

### Multiple Edges

- Multiple edges of the same type can exist if created by different authors
- The same author updating an existing edge type replaces their previous assertion
- All edges are preserved in the file, creating a full history

### Tombstones

- Edges are never deleted, only marked as tombstoned
- Tombstones preserve history and allow for edge revival
- A tombstoned edge is effectively "removed" from the active graph

### Merge Strategy

When merging `.gml` files:
1. Union all edges from both versions
2. Remove exact duplicates (same type, author, timestamp)
3. Sort edges by timestamp for consistent ordering
4. No merge conflicts - all perspectives are preserved

## Git Integration

### Filter Drivers

Add to `.gitattributes`:
```
*.gml filter=gitmind diff=gitmind merge=gitmind
```

This enables:
- Custom packing for efficient storage
- Semantic diffs showing edge changes
- Conflict-free merging

### Performance Considerations

- One file per node pair (not per edge) reduces Git objects by 10-100x
- YAML format is human-readable but can be compressed by Git
- Filter drivers can optimize storage format while preserving readability

## Example Usage

### Creating a link between parser.c and grammar.md:

```bash
git mind link parser.c grammar.md --type implements
```

This creates or updates a `.gml` file containing:
- Blob SHAs for both files
- An edge with type "implements"
- Current author and timestamp

### Viewing relationships:

```bash
git mind list parser.c
parser.c ←→ grammar.md [3 edges]
  → implements (alice, 0.9)
  → references (bob, 0.7)
  ← documents (charlie, 0.85)
```

## Version History

- v1.0 (2025-06-15): Initial format with multi-edge support