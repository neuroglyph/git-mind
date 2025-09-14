# Product Roadmap

Table of Contents

- [Executive Summary](#executive-summary)
- [Vision and Principles](#vision-and-principles)
- [Quarterly Themes and Initiatives](#quarterly-themes-and-initiatives)
- [Priority Ranking and Dependencies](#priority-ranking-and-dependencies)
- [Review and Update Cadence](#review-and-update-cadence)

## Executive Summary

git-mind evolves into a Git-native, time-travel-safe semantics system with fast, local queries and optional graph interoperability. This roadmap aligns with [ADR 0001](../adr/0001-first-class-semantics.md) and the [Semantics PRD](../PRDs/PRD-git-mind-semantics-time-travel-prototype.md) and organizes work into pragmatic, hobby-friendly chunks without fixed dates.

## Vision and Principles

- Names-as-truth: semantics defined by strings in history; derived IDs only for speed.
- Git-native: journals and caches follow branches; merges converge without conflicts.
- Fast and lean: roaring bitmap queries in milliseconds; rebuildable caches.
- Single-maintainer friendly: small increments, defensible milestones, clear stopping points.

## Themes and Initiatives (Full Project Scope)

Theme A: Core Truth Layer (Journal)

- A1: Journal writer/reader (CBOR edge commits under refs/gitmind/edges/<branch>)
- A2: AUGMENTS pipeline and post-commit hook
- A3: Journal scan APIs with pagination and filters

Theme B: Performance Layer (Cache)

- B1: Roaring Bitmap cache builder (forward/reverse indices)
- B2: Incremental rebuilds + versioned cache metadata
- B3: Query engine using bitmaps for fan-in/fan-out and combined filters

Theme C: CLI and UX

- C1: `git mind link/list/unlink/status/cache-rebuild` with helpful output modes (human/JSON)
- C2: Robust path→blob resolution and error messaging
- C3: Cohesion report (merge-aware semantic summary)

Theme D: Semantics & Advice

- D1: Names-as-truth: `type_name`/`lane_name` persisted; deterministic ID helpers
- D2: Advice data model and merge rules (hybrid CRDT)
- D3: Minimal advice application (symmetric, implies) behind flags

Theme E: Architecture & Modularity

- E1: Modular restructure toward single-header `gitmind.h`
- E2: Clear public/private boundaries in headers
- E3: Internal utilities for safe memory and string ops

Theme F: Quality & CI

- F1: CI hard-gate on C23 (gcc-14/clang-20); canary on older compilers
- F2: Clang-tidy zero-warning target in cache/journal/cbor; track regressions
- F3: Deterministic builds in Docker; cleanup tooling

Theme G: Packaging & Distribution

- G1: Build scripts for static/shared library + CLI
- G2: Release artifacts (archives, checksums)
- G3: Versioning policy (semver-ish) and changelogs

Theme H: Docs & Developer Experience

- H1: CLI manpages and examples; architecture deep dives
- H2: Contributor guides; pre-commit tooling (lint, docs checks)
- H3: “Pause and resume” playbooks and issue templates

Theme I: Interop (Optional)

- I1: Export/import helpers (CSV/JSON) for external analysis
- I2: Dataset generator for perf and demos

Theme J: Human + AI Co‑Thought (Optional)

- J1: Attribution lanes and filters for AI suggestions (suggested/verified)
- J2: MCP service for local tools to read/write edges via a stable interface
- J3: Review flows: accept/reject AI edges; promote lanes; audit trail

## Priority Ranking and Dependencies

1. A1 → A2 (AUGMENTS needs journal) → A3
2. B1 depends on A1; B2 depends on B1; B3 depends on B2
3. C1 depends on A1; C2 depends on robust path/ID helpers; C3 depends on D2
4. D1 depends on A1; D2 depends on A1; D3 depends on D2
5. E-series parallel after A1; F-series runs throughout; G-series near stabilization
6. I/J-series independent but benefit from A/B/C maturity; never gate core releases

## Review and Update Cadence

- Roadmap review at the end of each sprint block (no fixed dates).
- Keep Product Roadmap aligned with [Release Plans](Release_Plans.md); update when any initiative scope changes.
