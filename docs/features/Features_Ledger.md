---
title: Features Ledger
description: Canonical, grouped snapshot of features — shipped, in‑progress, and planned — including progress, tasks, tests, and remarks.
audience: [contributors]
domain: [planning]
tags: [features, progress]
status: draft
last_updated: 2025-09-16
---

# Features Ledger

This is the canonical, grouped snapshot of features — shipped, in‑progress, and planned — including progress, tasks, tests, and remarks. CLI/TUI and other feature tasks live here (not in AGENTS.md).

## Progress

<!-- progress:begin -->
```text
░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ 0%
------------|-------------|------------|
           MVP          Alpha    v1.0.0 
```
<!-- progress:end -->

Weighted by feature size (simple average for now). Updated by `python3 scripts/update_progress.py` or `make features-update`.

## Status Model

- Planned → In Progress → MVP → Alpha → V1
- Stage names are canonical; “Shipped” is implied by MVP/Alpha/V1.

---

### Core & Platform
<!-- group-progress:core-platform:begin -->
```text
░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ 0%
------------|-------------|------------|
           MVP          Alpha    v1.0.0 
features=?
```
<!-- group-progress:core-platform:end -->

| Emoji | Feature | Area | Spec | Code | Status | Progress % | Bar | Current State | Todo (Tasks) | Tests | Remarks |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 🧱 | OID‑first Core | Core | — | core/include, core/src | Alpha | 85 | █████████░ | Equality OID‑first; OID fields in CBOR; cache/journal aligned. | Finalize strict equality; docs complete. | Unit passing | Good foundation. |
| 🗄️ | Journal (CBOR) | Core | docs/architecture | core/src/journal | Alpha | 80 | ████████░░ | Writer/reader with base64; debug flag; attributed edges. | Finalize ordering; more tests. | Unit passing | Backwards compat OK. |
| ⚡ | Cache (Bitmap) | Core | docs/architecture | core/src/cache | In Progress | 70 | ███████░░░ | Rebuild + query; shard; metadata. | Conformance tests; size calc. | Unit partial | Performance tuning next. |
| 🔒 | CLI Safety Guard | CLI | docs/operations | apps/cli | MVP | 70 | ███████░░░ | Official repo guard; env override. | E2E extension; docs polish. | Unit | Solid. |
| 🔗 | Ref Utilities | Core | docs/architecture | core/src/util | MVP | 75 | ███████░░░ | `gm_build_ref` with validation; tests. | More negative tests; docs link. | Unit | Good coverage. |
| 🧰 | Safe String/Memory | Core | — | core/include/gitmind/util | V1 | 90 | █████████░ | Safe wrappers adopted in hot paths. | Sweep stragglers; docs. | Unit | Security uplift. |

### CLI, Hooks & UX
<!-- group-progress:cli-hooks-ux:begin -->
```text
░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ 0%
------------|-------------|------------|
           MVP          Alpha    v1.0.0 
features=?
```
<!-- group-progress:cli-hooks-ux:end -->

| Emoji | Feature | Area | Spec | Code | Status | Progress % | Bar | Current State | Todo (Tasks) | Tests | Remarks |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 🪝 | Post‑commit Hook (AUGMENTS) | Hooks | docs/hooks | core/src/hooks | In Progress | 50 | █████░░░░░ | Process changed files; AUGMENTS edge. | Path truncation handling (done); more cases. | Unit | Wire to journal read. |
| 🧰 | Review Seeding & Replies | DX | docs/tools | tools/review, workflows | MVP | 80 | ████████░░ | Seed + auto‑reply; artifact fallback. | GraphQL auto‑resolve; GH App. | CI + unit | High leverage. |
| 📜 | Docs Tooling | Docs | docs/tools | tools/docs | MVP | 70 | ███████░░░ | Frontmatter/TOC/title checks; labeler. | Extend templates; more guides. | Scripted | Good base. |
| 🧭 | Curated AGENTS.md | DX | docs | tools/agents | MVP | 80 | ████████░░ | TL;DR + YAML; curator; archives. | Tune summaries; monthly rollup. | Scripted | Fast onboarding. |
| 🧪 | Worksheet Gates | DX | docs/tools | tools/review | V1 | 90 | █████████░ | Pre‑push checks for placeholders + decisions. | Heuristics later (optional). | Scripted | Keeps quality high. |

### Observability & Analytics
<!-- group-progress:observability-analytics:begin -->
```text
░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░ 0%
------------|-------------|------------|
           MVP          Alpha    v1.0.0 
features=?
```
<!-- group-progress:observability-analytics:end -->

| Emoji | Feature | Area | Spec | Code | Status | Progress % | Bar | Current State | Todo (Tasks) | Tests | Remarks |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 🔎 | CBOR Debug Tracing | Core/Obs | docs/operations | core/src/journal | MVP | 60 | ██████░░░░ | Env flag for verbose decode. | Structured logging; sampling. | Unit | Helpful during migration. |
| 📈 | Progress Ledgers | DX/Obs | docs/features | scripts | MVP | 60 | ██████░░░░ | Ledger + updater script. | Weighting; KLoC metrics. | Scripted | Improves planning. |
| 📊 | CI Trend Baselines | Obs | docs/ci | tools | Planned | 20 | ██░░░░░░░░ | Not started. | Collect tidy counts; failures. | — | Future. |

---

Notes
- Update via `python3 scripts/update_progress.py` (or `make features-update`).
- Progress bars are averaged for now; weighting and KLoC are future work.

