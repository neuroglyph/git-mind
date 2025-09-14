<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->
<!-- © 2025 J. Kirby Ross / Neuroglyph Collective -->

# 🏊 Adult Swim: Deep End Memory Pool Opportunities

_"No floaties allowed - this is where we optimize HARD"_

## Memory Pool Opportunities Discovered During Migration

### 1. Edge Creation Pool (edge.c)

```c
/* Current: Every edge creation allocates */
int gm_edge_create(...) {
    // Currently stack allocated, but consider:
    // - Edge objects are fixed size (sizeof(gm_edge_t))
    // - Created frequently during link operations
    // - Often created in batches (augment operations)
}
```

__Pool Design__: Pre-allocate 1000 edge structs, reuse via free list
__Expected Win__: 10x faster edge creation in bulk operations

### 2. String Interning for Paths (edge.c)

```c
/* Current: Paths copied into every edge */
safe_string_copy(edge->src_path, src_path, GM_PATH_MAX);
safe_string_copy(edge->tgt_path, tgt_path, GM_PATH_MAX);

/* Observation: Same paths appear in MANY edges */
// "src/main.c" might be in 100+ edges
// "refs/heads/main" appears everywhere
```

__Pool Design__: Hash table of interned strings, edges store pointers
__Expected Win__: 80% memory reduction, faster string comparisons

### 3. Temporary Buffer Pool (edge.c)

```c
/* Current: Format buffer allocated by caller */
int gm_edge_format(const gm_edge_t *edge, char *buffer, size_t len);

/* Better: Thread-local format buffer pool */
// Avoid repeated stack allocations
// Reuse buffers for formatting operations
```

__Pool Design__: Per-thread circular buffer pool
__Expected Win__: Zero allocation for transient strings

### 4. SHA Pool (implicit in edge.c)

```c
/* Current: SHA arrays in every edge */
uint8_t src_sha[GM_SHA1_SIZE];
uint8_t tgt_sha[GM_SHA1_SIZE];

/* Observation: Same SHAs repeated across edges */
// If file has 50 relationships, SHA copied 50 times
```

__Pool Design__: SHA interning table, edges store indices
__Expected Win__: 20-byte SHA → 4-byte index

### 5. ULID Generation Pool (edge.c)

```c
/* Current: ULID generated per edge */
gm_ulid_generate(edge->ulid);

/* Observation: ULIDs generated in sequence */
// Could pre-generate batch during idle time
```

__Pool Design__: Background ULID generator with ring buffer
__Expected Win__: Amortize timestamp/random costs

## Implementation Priority

1. __String Interning__ - Biggest memory win, simplest to implement
2. __Edge Object Pool__ - High frequency allocation point  
3. __SHA Deduplication__ - Significant memory savings
4. __Buffer Pools__ - Eliminate transient allocations
5. __ULID Pregenerator__ - Nice-to-have optimization

## Architecture Notes

- Pools should be per-context (`gm_context_t`)
- Must support single-threaded and multi-threaded modes
- Graceful degradation if pools exhausted
- Debugging mode to track pool efficiency

## Memory Budget Targets

For a repository with 100k edges:

- Current: ~50MB heap allocation
- With pools: ~10MB heap allocation
- Cache efficiency: 5x improvement
- Allocation overhead: 90% reduction

---

_"Come to the deep end... the water's fine!"_ 🏊‍♂️
