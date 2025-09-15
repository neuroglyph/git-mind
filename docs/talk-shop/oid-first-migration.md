# OID‑First Migration: Future‑Proof by Design

Relying on raw 20‑byte SHA‑1 arrays is a dead end. git‑mind migrates interfaces to `git_oid` (our `gm_oid_t`) across the core, so the code is agnostic to the underlying hash algorithm. Equality, hashing, and cache sharding all operate on OIDs, not legacy byte blobs. That shift eliminates subtle bugs (like mismatched sizes or comparisons) and ensures we don’t care whether the repo is SHA‑1, SHA‑256, or what comes next.

We kept read‑compatibility during the transition: decoders backfill OIDs from legacy SHA fields when needed, and writers include both OID and legacy keys during a grace period. Meanwhile, caches, hooks, and equality semantics are OID‑first today. The payoff is simple—correctness now, and no migration panic later.

Phases of the migration:

```mermaid
graph TD
  A[Legacy Only
  sha[20] fields] --> B[Dual Mode
  OID + Legacy]
  B --> C[OID‑Only
  Legacy fields dropped]

  subgraph Compatibility
    A -->|read| B
    B -->|write OID| B
    B -->|read legacy| C
  end
```

We designed the migration to be boring: no big‑bang rewrites, no flag days—just steady evolution toward an interface that won’t age out.
