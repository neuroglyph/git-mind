---
title: Cache Edge Map
description: Domain data structure backing forward/reverse edge indexing.
audience: [contributors]
status: draft
---

# Cache Edge Map

## Table of Contents

- [Purpose](#purpose)
- [Structure](#structure)
- [Public API](#public-api)
- [Hashing Scheme](#hashing-scheme)
- [Consumers](#consumers)
- [Testing](#testing)
- [Future Enhancements](#future-enhancements)

## Purpose

`gm_edge_map` is a deterministic hash table that records which cache edge IDs are associated with a given Git object ID. The cache rebuild service uses two instances (forward and reverse) to produce sharded roaring bitmaps for quick lookup.

## Structure

Implementation lives in `core/src/domain/cache/edge_map.c`; the contract is declared in `core/include/gitmind/cache/internal/edge_map.h`.

- Buckets allocate arrays of `struct gm_edge_map_entry`, each holding a `gm_oid_t`, its bitmap, and a pointer to the next entry.
- All allocations are bounded; failures are reported via `gm_result_void_t` with `GM_ERR_OUT_OF_MEMORY`.

## Public API

```c
GM_NODISCARD gm_result_void_t gm_edge_map_create(size_t bucket_count,
                                                 gm_edge_map_t **out_map);
void gm_edge_map_destroy(gm_edge_map_t *map);
GM_NODISCARD gm_result_void_t gm_edge_map_add(gm_edge_map_t *map,
                                              const gm_oid_t *oid,
                                              uint32_t edge_id);
GM_NODISCARD gm_result_void_t gm_edge_map_visit(const gm_edge_map_t *map,
                                                gm_edge_map_visit_cb callback,
                                                void *userdata);
```

The visitor callback receives immutable access to the bitmap so consumers can write out sharded files without exposing mutation.

## Hashing Scheme

- Mixes each byte of the raw OID using constants derived from the golden ratio and Murmur-inspired multipliers.
- Applies avalanche steps (`>> 16`, `>> 15`) to reduce clustering.
- Hash function returns `0` when `bucket_count == 0` to avoid UB.

## Consumers

- `gm_cache_rebuild_execute` builds forward and reverse maps, then visits them to write `*.forward` and `*.reverse` bitmaps.
- Future consumers (e.g., incremental invalidation) can reuse the visitor callback to process OID buckets.

## Testing

`core/tests/unit/test_cache_edge_map.c` covers:

- Creation, insertion, and visitor traversal for multiple OIDs.
- Error propagation when the visitor intentionally returns a non-`GM_OK` result.

## Future Enhancements

- Expose iterator helpers (e.g., begin/next) if the visitor pattern becomes too constraining for incremental updates.
- Consider configurable load factors and rehashing if cache sizes grow beyond the current bucket count.
