# Architecture Decision Records (ADRs)

This directory captures durable architecture decisions for Git Mind.

Use ADRs for decisions that are hard to reverse, cross-cut multiple subsystems, or define platform invariants.

---

## ADR Index

## [ADR-0002](./ADR-0002.md) — Worktree Independence and Materialization Architecture
**Status:** Accepted
**Date:** 2026-02-15

### What it establishes
- Git worktree state is **not** authoritative for Git Mind graph state.
- Graph truth (WARP causal graph + provenance) is canonical.
- Materialized artifacts are derived outputs, non-canonical by default.
- Context-sensitive operations must be explicit (`asOf`, observer, trust).
- Generated artifact hygiene requires defense-in-depth (not `.gitignore` alone).

### Why it matters
Defines the core separation model: **worktree-aware, never worktree-bound**.

---

## [ADR-0003](./ADR-0003.md) — Graph-Native Content, Deterministic Materialization, and Workspace Bridge
**Status:** Accepted
**Date:** 2026-02-15

### What it adds
- “Content-on-node” remains canonical, but editing must be file-native in UX.
- `SyncAdapter` projected workspace is the default compatibility bridge.
- `FuseAdapter` is optional advanced/performance mode.
- Graph→editor invalidation/signaling is required infra.
- Writeback is transactional, conflict-aware, deterministic, and receipt-backed.
- Workspace path policy: editable entities vs read-only derived outputs.

### Why it matters
Turns the separation model into an adoption-ready product path without breaking architecture laws.

---

## What changed from ADR-0002 to ADR-0003

1. **From principle to execution:**  
   ADR-0002 defined boundaries; ADR-0003 defines how users actually work within them.

2. **Editing UX became first-class:**  
   The project now explicitly treats editing ergonomics as a top adoption risk.

3. **Bridge strategy finalized:**  
   Universal **Sync Workspace Mode first**, FUSE later for performance.

4. **Live update correctness added:**  
   Graph subscription + path invalidation are required to prevent stale editor state.

5. **Writeback semantics hardened:**  
   Base frontier/hash checks + explicit conflict paths + receipts.

---

## ADR Authoring Guidelines

Create a new ADR when a decision is:
- hard to reverse,
- architecture-wide,
- policy-defining,
- or likely to be questioned later.

Recommended sections:
1. Context  
2. Decision  
3. Alternatives considered  
4. Consequences  
5. Risks & mitigations  
6. Acceptance criteria  
7. Implementation impact

---

## ADR Lifecycle

- **Proposed:** Draft under discussion.
- **Accepted:** Approved and active.
- **Superseded:** Replaced by a newer ADR (link both ways).
- **Deprecated:** No longer applied, retained for historical context.

---

## [ADR-0004](./ADR-0004.md) — Content Attachments Belong in git-warp
**Status:** Accepted
**Date:** 2026-02-20

### What it establishes
- Content-on-node (CAS-backed blob attachment) is a **git-warp** responsibility, not git-mind.
- git-warp should install `git-cas` and expose an API for attaching content-addressed blobs to nodes.
- git-mind provides the CLI/UX layer (`content set/show/edit`) on top.
- This aligns git-warp with Paper I's `(S, α, β)` formalism — nodes carrying `Atom(p)` payloads.

### Why it matters
Prevents git-mind from duplicating CRDT, time-travel, observer, and provenance guarantees that already exist in the substrate. Makes the attachment primitive available to any git-warp consumer, not just git-mind.

---

## Quick Contribution Rules

- Keep ADRs concise but specific.
- Avoid ambiguous wording (“usually”, “maybe”) in core invariants.
- Include explicit consequences and operational impact.
- Link milestone(s), contracts, and tooling gates affected by the decision.