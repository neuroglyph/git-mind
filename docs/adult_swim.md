<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->
<!-- © 2025 J. Kirby Ross / Neuroglyph Collective -->

# 🏊 Adult Swim: Deep End Memory Pool Opportunities

*"No floaties allowed - this is where we optimize HARD"*

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
**Pool Design**: Pre-allocate 1000 edge structs, reuse via free list
**Expected Win**: 10x faster edge creation in bulk operations

### 2. String Interning for Paths (edge.c)
```c
/* Current: Paths copied into every edge */
safe_string_copy(edge->src_path, src_path, GM_PATH_MAX);
safe_string_copy(edge->tgt_path, tgt_path, GM_PATH_MAX);

/* Observation: Same paths appear in MANY edges */
// "src/main.c" might be in 100+ edges
// "refs/heads/main" appears everywhere
```
**Pool Design**: Hash table of interned strings, edges store pointers
**Expected Win**: 80% memory reduction, faster string comparisons

### 3. Temporary Buffer Pool (edge.c)
```c
/* Current: Format buffer allocated by caller */
int gm_edge_format(const gm_edge_t *edge, char *buffer, size_t len);

/* Better: Thread-local format buffer pool */
// Avoid repeated stack allocations
// Reuse buffers for formatting operations
```
**Pool Design**: Per-thread circular buffer pool
**Expected Win**: Zero allocation for transient strings

### 4. SHA Pool (implicit in edge.c)
```c
/* Current: SHA arrays in every edge */
uint8_t src_sha[GM_SHA1_SIZE];
uint8_t tgt_sha[GM_SHA1_SIZE];

/* Observation: Same SHAs repeated across edges */
// If file has 50 relationships, SHA copied 50 times
```
**Pool Design**: SHA interning table, edges store indices
**Expected Win**: 20-byte SHA → 4-byte index

### 5. ULID Generation Pool (edge.c)
```c
/* Current: ULID generated per edge */
gm_ulid_generate(edge->ulid);

/* Observation: ULIDs generated in sequence */
// Could pre-generate batch during idle time
```
**Pool Design**: Background ULID generator with ring buffer
**Expected Win**: Amortize timestamp/random costs

## Implementation Priority

1. **String Interning** - Biggest memory win, simplest to implement
2. **Edge Object Pool** - High frequency allocation point  
3. **SHA Deduplication** - Significant memory savings
4. **Buffer Pools** - Eliminate transient allocations
5. **ULID Pregenerator** - Nice-to-have optimization

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

*"Come to the deep end... the water's fine!"* 🏊‍♂️