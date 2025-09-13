
# ADR 0001: First‑Class, Time‑Travel‑Safe Semantics (Names as Truth)

Table of Contents
- [Status](#status)
- [Date](#date)
- [Related](#related)
- [Context](#context)
- [Decision](#decision)
- [Rationale](#rationale)
- [Consequences](#consequences)
- [Alternatives Considered](#alternatives-considered)
- [Implementation Notes](#implementation-notes)
- [Rollout](#rollout)

- Status: Proposed
- Date: 2025‑09‑12
- Related: docs/PRDs/PRD-git-mind-semantics-time-travel-prototype.md

## Context
- git‑mind stores semantic edges as Git commits under `refs/gitmind/edges/<branch>` and maintains optional bitmap caches under `refs/gitmind/cache/<branch>`.
- Users need to define arbitrary relationship types and lanes (e.g., `CALLS`, `TESTS`, `verified`, `draft`) while preserving Git’s guarantees: branch isolation, time‑travel, conflict‑free merges.
- Prior designs with enums or global registries cause drift and merge conflicts, breaking time‑travel.

## Decision
- Treat `type_name` and `lane_name` (UTF‑8 strings) as the ground truth on every edge. Do not compress user types into a generic/custom bucket.
- Derive stable 64‑bit IDs from the normalized names (e.g., `fnv1a64("gm_type:" + nfc(type_name))`) only for cache/index performance. Names remain with edges; IDs are derived, never authoritative.
- Optional “Semantics Advice” is stored as append‑only records in the journal to express behavior (e.g., aliases, symmetry, transitivity, implies, colors). Advice is branch/commit scoped and time‑travel safe.
- Merging semantics advice uses a hybrid CRDT rule set:
  - Scalars (bool/enums/numbers/colors): Last‑Write‑Wins (LWW) with deterministic tiebreak (clock||timestamp, actor_id, ulid).
  - Collections (aliases, implies, tags): Observed‑Remove Set (OR‑Set) with tombstones.
  - Maps: OR‑Set of keys + per‑key LWW for values.
- Caches remain derived per (branch, commit) shard; never merged; rebuild on demand.

## Rationale
- Names‑as‑truth ensures semantics time‑travel and merge like code; there is no global registry to drift or conflict.
- Derived IDs preserve roaring bitmap performance without sacrificing user vocabulary.
- Advice‑as‑data allows richer semantics without baking policy into core; it’s auditable and branch‑scoped.
- Hybrid CRDT rules deliver deterministic merges while minimizing loss of information.

## Consequences
- Pros
  - First‑class, user‑defined semantics with no “custom” fallback.
  - Deterministic behavior across clones/branches; time‑travel correct.
  - Fast queries via 64‑bit keys; caches are stable and ephemeral.
  - Extensible via advice and safe plugin hooks.
- Cons
  - Slightly larger edge payloads (two extra strings).
  - Advice parsing required for advanced behaviors; must cap expansion and detect cycles.

## Alternatives Considered
- Global registry of types/lanes with IDs
  - Rejected: non‑Git state, merge conflicts, time‑travel breakage.
- Hard‑coded enums
  - Rejected: eliminates user freedom; brittle; version lock‑in.
- Only names, no IDs
  - Rejected: slower caches and queries at scale.

## Implementation Notes
- Extend edge CBOR with `type_name` and `lane_name` (NFC normalized). Readers tolerate missing fields for backward compatibility.
- Add helper functions `gm_sem_type_id(const char*)` and `gm_sem_lane_id(const char*)` (documented hash).
- Define advice object format; implement minimal reader; apply only when present.
- Provide a “cohesion‑report” CLI to summarize advice/semantics changes after merges.

## Rollout
1. Implement deterministic ID helpers + normalization.
2. Extend CBOR encoder/decoder; migrate writers.
3. Update cache/query to accept names → IDs.
4. Implement minimal advice reader (optional in v1).
5. Add cohesion report and tests; document limits.
