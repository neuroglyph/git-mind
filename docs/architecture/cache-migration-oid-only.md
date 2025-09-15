# Cache Migration: OID-Only Format and Stable Refs

Table of Contents

- [Summary](#summary)
- [Migration Behavior](#migration-behavior)
- [Impact](#impact)
- [Recommended Steps](#recommended-steps)
- [Notes](#notes)

Date: 2025-09-14

## Summary

We finalized the move toward OID‑first semantics across cache and journal.

- Cache refs are now written to a stable ref: `refs/gitmind/cache/<branch>`.
- Older timestamped cache refs (`refs/gitmind/cache/<branch>/<ts>`) remain readable; loaders fall back to the most recent legacy ref if the stable ref is missing.
- Cache index contents are keyed by `gm_oid_t` (aka `git_oid`), removing SHA‑1 assumptions in hashing and comparisons.

## Migration Behavior

On load, `gm_cache_load_meta()` resolves:
1. `refs/gitmind/cache/<branch>` (preferred)
2. If missing, scans `refs/gitmind/cache/<branch>/*` and picks the latest commit

No in‑place modification of legacy refs occurs; new rebuilds update the stable ref.

## Impact

- Existing caches are still discoverable (fallback).
- New caches use stable naming and OID‑first internals.
- A full rebuild is recommended if CI signals stale cache.

## Recommended Steps

1. Rebuild caches on active branches:
   ```bash
   git mind cache-rebuild
   ```
2. Verify with stats:
   ```bash
   git mind cache-stats
   ```

## Notes

- Hash function uses raw `git_oid` bytes; naming clarified via `OID_HASH_MULTIPLIER`.
- This is a forward‑compatible change; future hash algorithms continue to work via libgit2’s `git_oid` abstraction.
