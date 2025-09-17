---
title: Milestone Synthesis & Scope Map
description: Consolidated view of MVP, Alpha, Beta, and v1.0.0 goals with cross-references to existing plans.
audience: [contributors]
domain: [planning]
tags: [milestones, roadmap]
status: draft
last_updated: 2025-09-16
---

# Milestone Synthesis & Scope Map

This guide aligns the many planning artifacts (HN demo plan, roadmap snapshots, milestone lists, release plans, brainstorms) into a single, milestone-oriented view. Use it when updating the [Features Ledger](../features/Features_Ledger.md) or when onboarding new contributors.

## Table of Contents

- [Quick Reference Table](#quick-reference-table)
- [Milestone Details](#milestone-details)
  - [MVP — “Show HN” Ready](#mvp--show-hn-ready)
  - [Alpha — Team Productivity](#alpha--team-productivity)
  - [Beta — Community Onboarding](#beta--community-onboarding)
  - [v1.0.0 — Production-Grade](#v100--production-grade)
- [How to Use This Doc](#how-to-use-this-doc)

## Quick Reference Table

| Milestone | Focus | Key Deliverables | Primary Sources |
|-----------|-------|------------------|-----------------|
| MVP | Prove the Git-native “mental model time-travel” loop; demo-ready flows. | Journal + AUGMENTS edges, core CLI (`link/list/cache`), deterministic semantics IDs, baseline cache rebuild + query, docs for demo. | `.legacy/HN Demo Plan.md`, `docs/planning/Milestones.md` (M0–M4), `docs/planning/Release_Plans.md` (v0.4–v0.6).
| Alpha | Team-ready tooling with reliable rebuilds and collaboration helpers. | Cohesion report basics, worksheet/review automation, cache perf guardrails, observability hooks, expanded CLI diagnostics. | `docs/planning/Milestones.md` (M5), `docs/planning/Release_Plans.md` (v0.7–v0.8), `docs/roadmap.md` (near/medium term).
| Beta | External contributor onboarding, advanced automation, packaging polish. | Advice merge + plugin hook stubs, CI hardening, packaging pipeline, attribution surfaced in CLI, telemetry snapshots. | `docs/planning/Milestones.md` (M6–M9), `docs/planning/Release_Plans.md` (v0.9–v0.10), `docs/wish-list-features/brain-storms/plugins.md`.
| v1.0.0 | Production-grade stability with complete semantics + attribution story. | OID-first equality everywhere, CI trend baselines, packaging/release hygiene, documentation freeze, optional MCP service (if landed). | `docs/planning/Release_Plans.md` (v0.10 + v0.11 optional), `docs/planning/Product_Roadmap.md`, `docs/planning/Milestones.md`.

## Milestone Details

### MVP — “Show HN” Ready

- **Experience goal:** Deliver the 30-second hook from `.legacy/HN Demo Plan.md` — initialize, link, time-travel, list, and traverse understanding history.
- **Scope highlights:**
  - Journal append + AUGMENTS hook (Milestones M3), deterministic semantics IDs (M1), CBOR names (M2).
  - CLI basics (`link`, `list`, `cache-rebuild`) with helpful errors (Release v0.4–v0.6).
  - Baseline cache to support `git mind list` across history (Product Roadmap Themes A/B/C).
  - Docs + scripts to reproduce the demo.
- **Exit criteria:** A maintainer can run the HN demo on a fresh repo, share reproducible docs, and recover after cache/journal reset.

### Alpha — Team Productivity

- **Experience goal:** Internal team uses git-mind daily with confidence in rebuilds and collaboration flows.
- **Scope highlights:**
  - Cohesion report + branch-aware queries (Milestones M5, Release v0.7–v0.8).
  - Worksheet automation + reply workflows (Features Ledger “Review Seeding & Replies”).
  - Cache performance guardrails (<10ms median per Release Plan), telemetry knobs (Roadmap “Medium Term”).
  - Observability: CBOR debug tracing, ledger snapshot tasks.
- **Exit criteria:** Cache rebuilds complete without manual babysitting; collaboration loops (worksheet, review replies) exercised on real PR; alpha docs outline known gaps.

### Beta — Community Onboarding

- **Experience goal:** External contributors can install, review, and extend git-mind without maintainer hand-holding.
- **Scope highlights:**
  - Advice merge CRDT + plugin hook scaffolding (Milestones M6–M9, Release v0.9–v0.10).
  - Attribution surfaced in CLI output; ledger snapshots automated for transparency.
  - Packaging + CI hardening; tidy-zero guard for core modules.
  - Plugin vision seeds (wishlist brainstorm) inform optional experiments.
- **Exit criteria:** Nightly builds + packaging succeed, documentation covers install/update flows, advanced review automation stable, plugin hooks gated but demonstrable.

### v1.0.0 — Production-Grade

- **Experience goal:** git-mind installs cleanly, passes strict CI, and offers a complete semantics + attribution story suitable for partners.
- **Scope highlights:**
  - OID-first equality everywhere; safe wrappers swept (ledger tasks, Release Plan).
  - CI trend baselines & observability dashboards.
  - Packaging artifacts with checksums + version policy (Release v0.10) plus optional MCP service (v0.11) if ready.
  - Documentation freeze + API stability commitments (Roadmap long-term).
- **Exit criteria:** All v0.x acceptance criteria met, outstanding ledger tasks closed or re-scoped, README/Features Ledger alignment signed off.

## How to Use This Doc

1. **Feature Ledger updates:** Pick the milestone whose exit criteria match the feature’s ambition; update the `Milestone` column accordingly.
2. **Task tagging:** Prefix checklist items with `[MVP]`, `[Alpha]`, `[Beta]`, or `[v1.0.0]` to feed the ledger callouts.
3. **Planning syncs:** Reference the Quick Reference table during roadmap discussions to ensure new ideas map to an existing milestone (or justify a new one).
4. **Docs hygiene:** When adding new planning material, include a “Milestone Alignment” note linking back here to keep the taxonomy coherent.

Keeping these milestones aligned across docs prevents drift and keeps our progress bars meaningful.
