# git-mind Technical Architecture

This document covers the implementation details of git-mind. For user documentation, see the [README](README.md).

---

## System Overview

git-mind stores semantic edges as Git objects, enabling:
- **Serverless operation** — Git is the database
- **Branch-scoped semantics** — Edges exist per branch
- **Deterministic merging** — Edges merge like code
- **Time-travel queries** — Checkout = point-in-time graph

---

## Storage Model

### Journal (Source of Truth)

Each edge is stored as a Git commit under `refs/gitmind/edges/<branch>`:

refs/gitmind/edges/main/ ├── 01HF3MG7K8XQZJ2NCFHPV3BQTA # ULID-based edge ID ├── 01HF3MG9ZTPQWJ7KFCGXN9YSMD └── 01HF3MGCB4XQZJ2NCFHPV3BQTB

Edge format (CBOR):

```json
{
  "id": "01HF3MG7K8XQZJ2NCFHPV3BQTA",
  "from": "sha256:abc123...",
  "to": "sha256:def456...",
  "type": "implements",
  "lane": "verified",
  "author": "human:user@example.com",
  "timestamp": 1699459200,
  "metadata": {}
}
````

### Cache (Performance Layer)

Optional Roaring Bitmap indices under `refs/gitmind/cache/<branch>`:

```
refs/gitmind/cache/main/
├── index.bitmap     # Main index
├── type-implements.bitmap
└── lane-verified.bitmap
```

Properties:

- Rebuildable from journal
- Never merged (branch-local)
- Safe to delete
- O(log N) set operations

---

## Query Engine

### Query Pipeline

1. **Parse** — Convert natural language or structured query
2. **Plan** — Choose between cache (fast) or journal scan (complete)
3. **Execute** — Bitmap operations or journal traversal
4. **Filter** — Apply attribution, lane, time-range filters
5. **Rank** — Sort by relevance, recency, or trust

### Query Examples

```c
// Direct lookup
edges_from("src/auth.c")

// Pattern matching  
edges_matching(type="implements", lane="verified")

// Graph traversal
transitive_closure("src/main.c", max_depth=3)

// Temporal
edges_at_commit("abc123def")
```

---

## Performance

### Benchmarks

|Operation|Journal Only|With Cache|
|---|---|---|
|edges_from (1 file)|45ms|0.3ms|
|pattern match (1000 edges)|200ms|2ms|
|transitive closure (depth=3)|500ms|8ms|
|cache rebuild (10k edges)|-|1.2s|

### Optimization Strategies

1. **Lazy cache building** — Build on first query
2. **Incremental updates** — Only process new edges
3. **Bloom filters** — Quick negative lookups
4. **Memory mapping** — Zero-copy bitmap access

---

## Core Concepts

**Edge** — Directed link (source → target) between any two Git blobs with metadata

**Names-as-truth** — Type/lane names stored as strings on the edge; IDs derived only for performance

**Attribution** — Who/what created the edge (human/AI), with author/session metadata

**AUGMENTS** — Evolution links (old blob → new blob) created on edits to preserve meaning through change

**Advice (optional)** — Data-driven semantics (e.g., symmetry, implies) that merge deterministically

---

## Development

### Build Requirements

```bash
# Core requirements
gcc-14 or clang-20  # C23 support
meson >= 1.0
ninja >= 1.10

# Optional
python >= 3.10      # For bindings
rust >= 1.70        # For future extensions
```

### Build & Test

```bash
make                 # meson+ninja build in ./build
make test           # runs unit tests
make test-integration  # integration tests
make bench          # performance benchmarks
make fuzz           # fuzzing (requires AFL++)
```

### Architecture

```
git-mind/
├── libgitmind/          # Core library
│   ├── core/            # Types, crypto, I/O
│   │   ├── ulid.h       # ULID generation
│   │   ├── cbor.h       # CBOR encoding
│   │   └── hash.h       # Content addressing
│   ├── graph/           # Graph operations
│   │   ├── edge.h       # Edge definitions
│   │   ├── query.h      # Query engine
│   │   └── merge.h      # Merge strategies
│   └── storage/         # Persistence
│       ├── journal.h    # Append-only log
│       ├── cache.h      # Bitmap cache
│       └── git.h        # Git integration
├── apps/
│   ├── cli/             # Command-line tool
│   ├── hooks/           # Git hooks
│   └── mcp/             # Model Context Protocol
└── bindings/
    ├── python/          # Python bindings
    └── wasm/            # WebAssembly
```

---

## Key Design Decisions

1. **Names as truth** — Types/lanes are strings, not IDs
2. **ULID for edges** — Time-sortable, globally unique
3. **CBOR encoding** — Compact, schema-evolution friendly
4. **Roaring Bitmaps** — Best-in-class compressed bitsets
5. **C23 core** — Modern C with _BitInt, typeof, auto

---

## Detailed Specifications

- [Edge Format Specification](https://claude.ai/chat/docs/specs/edge-format.md)
- [Query Language Grammar](https://claude.ai/chat/docs/specs/query-grammar.md)
- [Cache Format](https://claude.ai/chat/docs/specs/cache-format.md)
- [Merge Algorithm](https://claude.ai/chat/docs/specs/merge-algorithm.md)
- [Attribution System](https://claude.ai/chat/docs/specs/attribution.md)

---

## Contributing

See [CONTRIBUTING.md](https://claude.ai/chat/CONTRIBUTING.md) for:

- Code style guide
- Commit conventions
- Review process
- Architecture decisions (ADRs)
