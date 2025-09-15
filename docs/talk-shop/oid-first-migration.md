# OID‑First Migration: Future‑Proof by Design

Relying on raw 20‑byte SHA‑1 arrays is a dead end. git‑mind migrates interfaces to `git_oid` (our `gm_oid_t`) everywhere so the code is agnostic to the underlying hash algorithm. Equality, hashing, and cache sharding operate on OIDs, not legacy byte blobs. That shift eliminates subtle bugs (like mismatched byte sizes) and ensures we don’t care whether the repo is SHA‑1, SHA‑256, or something new.

We kept read‑compatibility: decoders backfill OIDs from legacy fields, and writers include OIDs alongside the old keys during a grace period. Meanwhile, caches, hooks, and equality semantics are OID‑first today. The payoff is simple—correctness now, and no migration panic later.

