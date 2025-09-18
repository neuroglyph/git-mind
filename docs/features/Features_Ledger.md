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

Progress is milestone-weighted overall, milestone-specific by KLoC, and feature-weighted within each group. Update via `python3 scripts/update_progress.py` or `make features-update` after edits.


Here's how we calculate the completion percentage of each milestone:

${formula where features are weighted based on their estimated complexity}$

And here's how we calculate the completion percentage of the overall project:

${overall progress formula}$

### Progress Bars

```

| Milestone | % | Remarks |
|-----------|---|------------|
| MVP       | `██████████████████████████████░░░░░░░░░░ 74%` | Min. Viable Product |
| Alpha     | `█████████████████████████████▓░░░░░░░░░░ 73%` | Comes before Beta |
| Beta      | `██████████████████████░░░░░░░░░░░░░░░░░░ 55%` | Half way |
| v1.0.0    | `█████████████████████████████████░░░░░░░ 82%` | Lol what? |
| --- |
| Total   | `█████████████████████████████░░░░░░░░░░░ 72%` | Math don't be mathin' |

<!-- progress-overall:begin -->
```text

MVP 74% | ALPHA 73% | BETA 55% | V1.0.0 82%
```
<!-- progress-overall:end -->

### MVP Progress

**What “MVP” means:** Core linking/journal flows usable end-to-end for maintainers and early adopters; docs and safety rails in place for guided experiments. See `docs/planning/Milestone_Synthesis.md#mvpmmdash-show-hn-ready`.

<!-- progress-mvp:begin -->
```text
██████████████████████████████░░░░░░░░░░ 74%
```
<!-- progress-mvp:end -->

> [!INFO]- **Outstanding Tasks**
> <!-- tasks-mvp:begin -->
> _All tracked tasks complete_
> <!-- tasks-mvp:end -->

### Alpha Progress

**What “Alpha” means:** Broader team-ready; cache rebuilds reliable, augmented hooks online, telemetry/observability guardrails in place. See `docs/planning/Milestone_Synthesis.md#alpha-mdash-team-productivity`.

<!-- progress-alpha:begin -->
```text
█████████████████████████████▓░░░░░░░░░░ 73%
```
<!-- progress-alpha:end -->

> [!INFO]- **Outstanding Tasks**
> <!-- tasks-alpha:begin -->
> - [ ] Finalize bitmap cache conformance tests
> - [ ] Edge graph multi-branch coverage
> - [ ] Post‑commit AUGMENTS wiring
> - [ ] GraphQL auto‑resolve for accepted PR feedback
> - [ ] Validate worksheet reply workflow on a fresh PR
> <!-- tasks-alpha:end -->

### Beta Progress

**What “Beta” means:** External contributors onboard smoothly; scaling telemetry captured; advanced review automation hardened. See `docs/planning/Milestone_Synthesis.md#beta-mdash-community-onboarding`.

<!-- progress-beta:begin -->
```text
██████████████████████░░░░░░░░░░░░░░░░░░ 55%
```
<!-- progress-beta:end -->

> [!INFO]- **Outstanding Tasks**
> <!-- tasks-beta:begin -->
> - [ ] Surface attribution metadata in CLI output
> - [ ] Ledger snapshot automation
> <!-- tasks-beta:end -->

### v1.0.0 Progress

**What “v1.0.0” means:** Production-grade stability with complete semantics, attribution, and CI observability; ready for wider release and partner integrations. See `docs/planning/Milestone_Synthesis.md#v100-mdash-production-grade`.

<!-- progress-v1:begin -->
```text
█████████████████████████████████░░░░░░░ 82%
```
<!-- progress-v1:end -->

> [!INFO]- **Outstanding Tasks**
> <!-- tasks-v1:begin -->
> - [ ] Adopt OID‑first equality everywhere (strict, no SHA fallback when OIDs present)
> - [ ] CI trend baselines collection
> <!-- tasks-v1:end -->

## Status Model

- Planned → In Progress → MVP → Alpha → Beta → v1.0.0
- Stage names are canonical; “Shipped” is implied by milestone tags ≥ the current release.

## Feature Lifecycle & Workflow

Use the ledger as the single system of record from ideation through release:

1. **Capture (Planned)** — When a new idea lands (roadmap review, retro, support signal), add a row under the appropriate group with a short name, owning area, and initial KLoC estimate. Link to any discovery notes/spec drafts in the “Spec” column and file a task in the `## Tasklist` section if further framing is needed.
2. **Framing (Planned → In Progress)** — Once scope and acceptance criteria are clear, mark status `In Progress`, ensure the “Current State” block describes the baseline, and list outstanding engineering/design work in the “Todo (Tasks)” column. Keep KLoC estimates realistic as tickets are broken down.
3. **Build & Land (In Progress)** — Engineers ship code behind feature flags/guards. Update progress % weekly (or when a significant chunk lands) and log validation tasks (tests, docs, follow-up stories). Reference the primary implementation branches/paths in the “Code” column.
4. **Validation (MVP)** — When the feature is usable end-to-end for the target persona, flip status to `MVP`. Document rollout steps, known gaps, and add stabilization tasks (dogfooding, bug bashes) to the ledger’s task list.
5. **Hardening (Alpha/Beta)** — Scale to broader audiences, tighten reliability/perf, and ensure telemetry/observability is live. Integrate feedback into Todo, tag follow-up tasks with `[Alpha]` or `[Beta]` as appropriate.
6. **General Availability (v1.0.0)** — All quality bars met, docs/tests complete, downstream consumers notified. Continue to attach post-GA follow-ups (deprecation notices, future enhancements) either as new ledger entries or subtasks in the Notes section.

Operational rules:

- Every feature row must have: owning area, milestone tag, spec/code pointers, KLoC estimate, status, progress %, and actionable Todo/Triage notes. Update KLoC when scope materially changes.
- The `## Tasklist` checklist is the authoritative backlog for cross-cutting work. Sync it whenever Todo items are resolved or re-scoped.
- README’s “📊 Status” reflects the overall milestone-weighted bar automatically; run `make features-update` after edits.
- For net-new features, create discovery docs in `docs/` and link them before moving past Planned.
- Retire legacy items by marking status `Deprecated` (optional column note) and migrating remaining tasks to successors.

---

### Core & Platform
<!-- group-progress:core-platform:begin -->
```text
████████████████████████████████░░░░░░░░ 79%
------------|-------------|------------|
           MVP          Alpha    v1.0.0 
features=8
```
<!-- group-progress:core-platform:end -->

| Emoji | Feature | Area | Spec | Code | Milestone | KLoC (approx) | Status | Progress % | Bar | Current State | Todo (Tasks) | Tests | Remarks |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 🧱 | OID‑first Core | Core | — | core/include, core/src | v1.0.0 | 3.5 | Alpha | 85% | █████████░ | Equality OID‑first; OID fields in CBOR; cache/journal aligned. | Finalize strict equality; docs complete. | Unit passing | Good foundation. |
| 🗄️ | Journal (CBOR) | Core | docs/architecture/journal-architecture-pivot.md | core/src/journal, core/src/cbor | Alpha | 1.2 | Alpha | 80% | ████████░░ | Writer/reader with base64; debug flag; attributed edges. | Finalize ordering; more tests. | Unit passing | Backwards compat OK. |
| ⚡ | Cache (Bitmap) | Core | docs/architecture/bitmap-cache-design.md | core/src/cache | Alpha | 1.6 | In Progress | 70% | ███████░░░ | Rebuild + query; shard; metadata. | Conformance tests; size calc. | Unit partial | Performance tuning next. |
| 🔗 | Ref Utilities | Core | docs/architecture/Ref_Name_Validation.md | core/src/util | MVP | 0.5 | MVP | 75% | ███████░░░ | `gm_build_ref` with validation; tests. | More negative tests; docs link. | Unit | Good coverage. |
| 🧰 | Safe String/Memory | Core | — | core/include/gitmind/util | v1.0.0 | 0.4 | V1 | 90% | █████████░ | Safe wrappers adopted in hot paths. | Sweep stragglers; docs. | Unit | Security uplift. |
| 🕸️ | Edge Graph Engine | Core | docs/architecture/System_Architecture.md | core/src/edge, core/include/gitmind/edge | Alpha | 1.0 | Alpha | 75% | ████████░░ | ULID-ordered edges; journal append + cache hydration. | Edge diff coverage; multi-branch stress. | Unit + fuzz | Forms graph spine. |
| 🧬 | Semantics & Type System | Core | docs/architecture/System_Architecture.md | core/src/types, core/src/utf8, core/src/time | MVP | 2.7 | Alpha | 80% | ████████░░ | Stable hashed IDs; NFC normalization; time-safe stamping. | Expose helper API; document name rules. | Unit | Ready for consumers. |
| 🪪 | Attribution Metadata | Core | docs/architecture/attribution-system.md | core/src/attribution | Beta | 0.2 | In Progress | 55% | ██████░░░░ | Capture author/time for edges; merge helpers in place. | Extend multi-author rollups; CLI surfacing. | Unit scaffolding | Needs UX polish. |

### CLI, Hooks & UX
<!-- group-progress:cli-hooks-ux:begin -->
```text
████████████████████████████░░░░░░░░░░░░ 69%
------------|-------------|------------|
           MVP          Alpha    v1.0.0 
features=7
```
<!-- group-progress:cli-hooks-ux:end -->

| Emoji | Feature | Area | Spec | Code | Milestone | KLoC (approx) | Status | Progress % | Bar | Current State | Todo (Tasks) | Tests | Remarks |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 🪝 | Post‑commit Hook (AUGMENTS) | Hooks | docs/hooks | core/src/hooks | Alpha | 0.5 | In Progress | 50% | █████░░░░░ | Process changed files; AUGMENTS edge. | Path truncation handling (done); more cases. | Unit | Wire to journal read. |
| 🔒 | CLI Safety Guard | CLI | docs/operations/Environment_Variables.md | apps/cli/cli_runtime.c | MVP | 0.6 | MVP | 70% | ███████░░░ | Official repo guard; env override; safe install workflow. | Extend host override tests; document upgrade path. | Unit | Solid baseline. |
| 🛠️ | `git mind` CLI Commands | CLI | apps/cli/main.md | apps/cli | MVP | 1.2 | MVP | 65% | ██████░░░░ | `link`, `list`, cache rebuild; shared runtime utilities. | Add query subcommands; richer diagnostics. | Unit + manual | Ready for daily use. |
| 🧰 | Review Seeding & Replies | DX | docs/tools | tools/review, workflows | Alpha | 0.4 | MVP | 80% | ████████░░ | Seed + auto‑reply; artifact fallback; reply templates rendered cleanly. | Gate GraphQL auto-resolve behind flag; document live PR validation; GH App roadmap. | CI + unit | High leverage. |
| 📜 | Docs Tooling | Docs | docs/tools | tools/docs | MVP | 0.4 | MVP | 70% | ███████░░░ | Frontmatter/TOC/title checks; labeler. | Extend templates; more guides. | Scripted | Good base. |
| 🧭 | Curated AGENTS.md | DX | docs | tools/agents | MVP | 0.2 | MVP | 80% | ████████░░ | TL;DR + YAML; curator; archives. | Tune summaries; monthly rollup. | Scripted | Fast onboarding. |
| 🧪 | Worksheet Gates | DX | docs/tools | tools/review | Alpha | 0.3 | V1 | 90% | █████████░ | Pre‑push checks for placeholders + decisions. | Heuristics later (optional). | Scripted | Keeps quality high. |

### Observability & Analytics
<!-- group-progress:observability-analytics:begin -->
```text
█████████████████████▓░░░░░░░░░░░░░░░░░░ 53%
------------|-------------|------------|
           MVP          Alpha    v1.0.0 
features=3
```
<!-- group-progress:observability-analytics:end -->

| Emoji | Feature | Area | Spec | Code | Milestone | KLoC (approx) | Status | Progress % | Bar | Current State | Todo (Tasks) | Tests | Remarks |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 🔎 | CBOR Debug Tracing | Core/Obs | docs/operations | core/src/journal | Alpha | 0.2 | MVP | 60% | ██████░░░░ | Env flag for verbose decode. | Structured logging; sampling. | Unit | Helpful during migration. |
| 📈 | Progress Ledgers | DX/Obs | docs/features | scripts | MVP | 0.3 | MVP | 70% | ███████░░░ | Ledger + updater script; KLoC-weighted bars + README sync. | Expose CLI summary; automate weekly snapshots. | Scripted | Improves planning. |
| 📊 | CI Trend Baselines | Obs | docs/ci | tools | v1.0.0 | 0.2 | Planned | 20% | ██░░░░░░░░ | Not started. | Collect tidy counts; failures. | — | Future. |

## Tasklist

### Core & Platform

- [ ] [v1.0.0] Adopt OID‑first equality everywhere (strict, no SHA fallback when OIDs present)
> [!INFO]-
> Update comparisons and tests; align cache/journal; document in TECHNICAL.md.

- [ ] [Alpha] Finalize bitmap cache conformance tests
> [!INFO]-
> Build synthetic repos; verify shard distribution and size calculations; add docs.

- [ ] [Alpha] Edge graph multi-branch coverage
> [!INFO]-
> Add regression tests for branch fan-out, ULID ordering guarantees, and edge diff helpers; capture findings in `docs/architecture/System_Architecture.md`.

- [ ] [Beta] Surface attribution metadata in CLI output
> [!INFO]-
> Thread author/time data through cache rebuild + `git mind list`; document flag to toggle extended columns.

- [x] [Alpha] Retire legacy `src/` tree
> [!INFO]-
> Move remaining markdown artifacts under `src/` into `docs/legacy/src/`, remove empty scaffolding, and update references so core/ is the sole code root.

- [x] [Alpha] Sweep includes and docs for `src/` references
> [!INFO]-
> Replace lingering `src/` include or doc paths with `core/` equivalents and ensure tooling/tests reflect the finalized layout.

### CLI, Hooks & UX

- [ ] [Alpha] Post‑commit AUGMENTS wiring
> [!INFO]-
> Finish journal read integration; handle path truncation errors; add E2E.

- [ ] [Alpha] GraphQL auto‑resolve for accepted PR feedback
> [!INFO]-
> Add a feature-flagged path in `apply-feedback` to call `discussion.resolveReviewThread`; map worksheet IDs to GraphQL node IDs and dry-run before enabling by default.

- [ ] [Alpha] Validate worksheet reply workflow on a fresh PR
> [!INFO]-
> Run `make apply-feedback` against a new test PR, capture rendered replies, and embed screenshots/notes in `docs/tools/Review_Seeding.md`.

### Observability & Analytics

- [ ] [v1.0.0] CI trend baselines collection
> [!INFO]-
> Record tidy counts and unit results; render sparklines; add dashboards.

- [ ] [Beta] Ledger snapshot automation
> [!INFO]-
> Wire `make features-update` into a scheduled workflow and add regression tests to confirm README + ledger stay in sync.


---

Notes
- Update via `python3 scripts/update_progress.py` (or `make features-update`).
- Progress bars use KLoC weighting; keep the KLoC column fresh to avoid drift.
