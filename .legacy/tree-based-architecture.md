# Git-Mind Tree-Based Architecture

## The Revelation

Git-mind pointers are stored using Git's native tree and blob objects. No custom formats. No performance hacks. Just Git.

## Structure

```
.gitmind/
  graph/
    <source-sha>/           # Git tree object
      <relationship-type>/  # Git tree object
        <target-sha>        # Git blob object (metadata)
```

## Example

When you run:

```bash
git mind link parser.c grammar.md --type implements --confidence 0.9
```

It creates:

```
.gitmind/
  graph/
    abc123.../              # SHA of parser.c
      implements/           # relationship type
        def456...           # SHA of grammar.md (blob: {"confidence": 0.9})
```

## Why This Works

1. __Git trees ARE the graph structure__ - no separate graph database needed
2. __Natural sharding__ - filesystem limits avoided via SHA prefixing
3. __Merge-friendly__ - Git knows how to merge trees
4. __Query-efficient__ - `git ls-tree` traverses relationships
5. __Pack-friendly__ - Git delta-compresses similar structures
6. __No refs__ - just objects, no performance degradation

## Operations

### Create a link

```bash
# Get SHAs
source_sha=$(git hash-object parser.c)
target_sha=$(git hash-object grammar.md)

# Create metadata blob
echo '{"confidence": 0.9}' | git hash-object -w --stdin

# Update tree structure
git update-index --add --cacheinfo 100644 <metadata-sha> \
  ".gitmind/graph/$source_sha/implements/$target_sha"
```

### Query relationships

```bash
# What does parser.c implement?
git ls-tree .gitmind/graph/$(git hash-object parser.c)/implements/

# What implements grammar.md? (reverse lookup - need index)
git grep -l "def456" .gitmind/graph/*/implements/
```

### Traverse the graph

```bash
# Start from a file, follow relationships
function traverse() {
  local sha=$1
  git ls-tree -r .gitmind/graph/$sha/
}
```

## Metadata Format

Each pointer blob contains minimal JSON:

```json
{
  "confidence": 0.9,
  "timestamp": 1234567890
}
```

Author and timestamp come from the Git commit that created it.

## Future Extensions

Since everything is content-addressable:

- Pointers can point to pointers (meta-relationships)
- Pointers can point to commits (time-based links)
- Pointers can point to trees (module-level relationships)
- Git Notes can add mutable metadata (traversal counts)

## Performance Characteristics

- __Storage__: O(E) where E = number of edges
- __Query "from"__: O(1) - direct tree lookup
- __Query "to"__: O(E) - requires index or grep
- __Merge__: O(E) - Git's tree merge
- __Clone__: Proportional to graph size, but Git packs efficiently

## Migration Path

1. Start with tree-based storage
2. Build reverse index for efficient "incoming" queries
3. Add Git Notes for mutable metadata
4. Optimize with custom Git filters if needed

This isn't just storage - it's Git-native knowledge representation.
