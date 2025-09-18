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

| Emoji | Feature | Area | Spec | Code | Milestone | KLoC (approx) | Status | Progress % | Bar | Current State | Tests | Remarks |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 🧱 | OID‑first Core | Core | — | core/include, core/src | v1.0.0 | 3.5 | Alpha | 85% | █████████░ | Equality OID‑first; OID fields in CBOR; cache/journal aligned. | Unit passing | Good foundation. |
| 🗄️ | Journal (CBOR) | Core | docs/architecture/journal-architecture-pivot.md | core/src/journal, core/src/cbor | Alpha | 1.2 | Alpha | 80% | ████████░░ | Writer/reader with base64; debug flag; attributed edges. | Unit passing | Backwards compat OK. |
| ⚡ | Cache (Bitmap) | Core | docs/architecture/bitmap-cache-design.md | core/src/cache | Alpha | 1.6 | In Progress | 75% | ███████░░░ | Rebuild + query; shard; metadata. | Unit partial | Added branch-limit + tree-size coverage. |
| 🔗 | Ref Utilities | Core | docs/architecture/Ref_Name_Validation.md | core/src/util | MVP | 0.5 | MVP | 75% | ███████░░░ | `gm_build_ref` with validation; tests. | Unit | Good coverage. |
| 🧰 | Safe String/Memory | Core | — | core/include/gitmind/util | v1.0.0 | 0.4 | V1 | 90% | █████████░ | Safe wrappers adopted in hot paths. | Unit | Security uplift. |
| 🕸️ | Edge Graph Engine | Core | docs/architecture/System_Architecture.md | core/src/edge, core/include/gitmind/edge | Alpha | 1.0 | Alpha | 75% | ████████░░ | ULID-ordered edges; journal append + cache hydration. | Unit + fuzz | Forms graph spine. |
| 🧬 | Semantics & Type System | Core | docs/architecture/System_Architecture.md | core/src/types, core/src/utf8, core/src/time | MVP | 2.7 | Alpha | 80% | ████████░░ | Stable hashed IDs; NFC normalization; time-safe stamping. | Unit | Ready for consumers. |
| 🪪 | Attribution Metadata | Core | docs/architecture/attribution-system.md | core/src/attribution | Beta | 0.2 | In Progress | 55% | ██████░░░░ | Capture author/time for edges; merge helpers in place. | Unit scaffolding | Needs UX polish. |
| 🗂️ | Sandbox Transactions | Core | docs/talk-shop/Sandbox_Transactions.md | core/src/journal (planned) | Alpha | 0.8 | Planned | 5% | █░░░░░░░░░ | Pending refs + CLI flow to stage/apply journal mutations safely. | Planned | Draft RFC merged. |

### CLI, Hooks & UX
<!-- group-progress:cli-hooks-ux:begin -->
```text
████████████████████████████░░░░░░░░░░░░ 69%
------------|-------------|------------|
           MVP          Alpha    v1.0.0 
features=7
```
<!-- group-progress:cli-hooks-ux:end -->

| Emoji | Feature | Area | Spec | Code | Milestone | KLoC (approx) | Status | Progress % | Bar | Current State | Tests | Remarks |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 🪝 | Post‑commit Hook (AUGMENTS) | Hooks | docs/hooks | core/src/hooks | Alpha | 0.5 | In Progress | 50% | █████░░░░░ | Process changed files; AUGMENTS edge. | Unit | Wire to journal read. |
| 🔒 | CLI Safety Guard | CLI | docs/operations/Environment_Variables.md | apps/cli/cli_runtime.c | MVP | 0.6 | MVP | 70% | ███████░░░ | Official repo guard; env override; safe install workflow. | Unit | Solid baseline. |
| 🛠️ | `git mind` CLI Commands | CLI | apps/cli/main.md | apps/cli | MVP | 1.2 | MVP | 65% | ██████░░░░ | `link`, `list`, cache rebuild; shared runtime utilities. | Unit + manual | Ready for daily use. |
| 🧰 | Review Seeding & Replies | DX | docs/tools | tools/review, workflows | Alpha | 0.4 | MVP | 80% | ████████░░ | Seed + auto‑reply; artifact fallback; reply templates rendered cleanly. | CI + unit | High leverage. |
| 📜 | Docs Tooling | Docs | docs/tools | tools/docs | MVP | 0.4 | MVP | 70% | ███████░░░ | Frontmatter/TOC/title checks; labeler. | Scripted | Good base. |
| 🧭 | Curated AGENTS.md | DX | docs | tools/agents | MVP | 0.2 | MVP | 80% | ████████░░ | TL;DR + YAML; curator; archives. | Scripted | Fast onboarding. |
| 🧪 | Worksheet Gates | DX | docs/tools | tools/review | Alpha | 0.3 | V1 | 90% | █████████░ | Pre‑push checks for placeholders + decisions. | Scripted | Keeps quality high. |

### Observability & Analytics
<!-- group-progress:observability-analytics:begin -->
```text
█████████████████████▓░░░░░░░░░░░░░░░░░░ 53%
------------|-------------|------------|
           MVP          Alpha    v1.0.0 
features=3
```
<!-- group-progress:observability-analytics:end -->

| Emoji | Feature | Area | Spec | Code | Milestone | KLoC (approx) | Status | Progress % | Bar | Current State | Tests | Remarks |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 🔎 | CBOR Debug Tracing | Core/Obs | docs/operations | core/src/journal | Alpha | 0.2 | MVP | 60% | ██████░░░░ | Env flag for verbose decode. | Unit | Helpful during migration. |
| 📈 | Progress Ledgers | DX/Obs | docs/features | scripts | MVP | 0.3 | MVP | 70% | ███████░░░ | Ledger + updater script; KLoC-weighted bars + README sync. | Scripted | Improves planning. |
| 📊 | CI Trend Baselines | Obs | docs/ci | tools | v1.0.0 | 0.2 | Planned | 20% | ██░░░░░░░░ | Not started. | Planned | Future. |

## Tasklist

### Core & Platform

<a id="gmv1t001"></a>
- [ ] (GM.V1.T001) Finalize strict equality; docs complete.
> [!INFO]- Step-by-step Instructions
> - [ ] Audit equality helpers in cache/journal and ensure OID-first comparisons never fall back when OIDs exist.
> - [ ] Add regression tests covering mixed OID/string paths.
> - [ ] Update `docs/TECHNICAL.md` and related ADRs to document the strict behavior.
> - [ ] Sweep references to legacy SHA fallbacks in docs/examples.

<a id="gmalpha001"></a>
- [ ] (GM.ALPHA.T001) Finalize ordering; add tests for CBOR journal reader.
> [!INFO]- Step-by-step Instructions
> - [ ] Write table-driven tests for journal iteration order (newest-first, branch boundaries).
> - [ ] Exercise attributed vs. plain edges in CBOR fixtures.
> - [ ] Patch reader docs to reflect ordering guarantees.
> - [ ] Wire tests into `meson.build` and ensure they run in CI.

<a id="gmalpha002"></a>
- [ ] (GM.ALPHA.T002) Perf tuning + shard benchmarks.
> [!INFO]- Step-by-step Instructions
> - [ ] Capture baseline timings for cache rebuild/query across representative repos.
> - [ ] Profile shard fan-out hotspots and prototype optimizations.
> - [ ] Automate a lightweight benchmark harness (Docker-safe) to guard regressions.
> - [ ] Document results in `docs/architecture/bitmap-cache-design.md`.

<a id="gmmvp001"></a>
- [ ] (GM.MVP.T001) Add negative tests and docs for ref utilities.
> [!INFO]- Step-by-step Instructions
> - [ ] Extend `test_ref_utils` to cover invalid prefixes, length overflows, and reserved refs.
> - [ ] Ensure errors propagate with meaningful `GM_ERR_*` codes.
> - [ ] Update ref utilities docstrings and the Narrative in `docs/architecture/Ref_Name_Validation.md`.
> - [ ] Mention failure scenarios in README/tutorial examples.

<a id="gmv1t002"></a>
- [ ] (GM.V1.T002) Sweep remaining safe-string/memory stragglers and docs.
> [!INFO]- Step-by-step Instructions
> - [ ] Grep for raw `strcpy`/`sprintf` usage outside approved wrappers.
> - [ ] Replace with `gm_snprintf`/`gm_strcpy_safe` equivalents.
> - [ ] Update Safety section in `docs/enforcer/gameplans/03-observability-built-in.md`.
> - [ ] Add unit coverage where new wrappers are introduced.

<a id="gmalpha003"></a>
- [ ] (GM.ALPHA.T003) Edge diff coverage; multi-branch stress harness.
> [!INFO]- Step-by-step Instructions
> - [ ] Create fixture branches with divergent edge histories to prove diff stability.
> - [ ] Add integration tests comparing fan-in/out across merge commits.
> - [ ] Ensure cache/journal diff helpers handle branch fan-out.
> - [ ] Document the harness in `docs/architecture/System_Architecture.md`.

<a id="gmmvp002"></a>
- [ ] (GM.MVP.T002) Expose helper API; document naming rules.
> [!INFO]- Step-by-step Instructions
> - [ ] Promote internal semantics helpers to public headers (review include hygiene).
> - [ ] Add examples demonstrating NFC normalization to docs/tutorials.
> - [ ] Write API reference entries with usage notes.
> - [ ] Cover helper functions with unit tests.

<a id="gmbeta001"></a>
- [ ] (GM.BETA.T001) Extend multi-author rollups; surface in CLI.
> [!INFO]- Step-by-step Instructions
> - [ ] Extend attribution data structure to track multiple authors/time windows.
> - [ ] Update cache/journal serialization to carry additional metadata.
> - [ ] Add CLI flags to display attribution columns.
> - [ ] Document workflow and examples in attribution architecture doc.

<a id="gmalpha004"></a>
- [ ] (GM.ALPHA.T004) Implement sandbox txn start/apply/abort; hook integration.
> [!INFO]- Step-by-step Instructions
> - [ ] Implement `gm_txn_start/apply/abort` helpers manipulating pending refs.
> - [ ] Build CLI plumbing (`git mind txn ...`) with dry-run/apply flows.
> - [ ] Add local/server hook snippets to enforce provenance.
> - [ ] Ship documentation updates (RFC, roadmap, tutorial snippet).

### CLI, Hooks & UX

<a id="gmalpha005"></a>
- [ ] (GM.ALPHA.T005) Path truncation handling follow-ups; wire to journal read.
> [!INFO]- Step-by-step Instructions
> - [ ] Add failing test covering truncated paths in augment hook.
> - [ ] Ensure journal read/write propagates truncation errors cleanly.
> - [ ] Update hook documentation with failure guidance.
> - [ ] Verify behavior through integration test in container.

<a id="gmmvp003"></a>
- [ ] (GM.MVP.T003) Extend host override tests; document upgrade path.
> [!INFO]- Step-by-step Instructions
> - [ ] Expand CLI safety tests to cover new env overrides.
> - [ ] Document the upgrade path in `docs/operations/Environment_Variables.md`.
> - [ ] Announce safe override usage in README/CLI help.
> - [ ] Confirm behavior in Docker CI harness.

<a id="gmmvp004"></a>
- [ ] (GM.MVP.T004) Add query subcommands; richer diagnostics.
> [!INFO]- Step-by-step Instructions
> - [ ] Design subcommand syntax and update CLI help.
> - [ ] Implement parsing + output formatting for new queries.
> - [ ] Write unit/integration tests for diagnostics.
> - [ ] Update tutorial examples.

<a id="gmalpha006"></a>
- [ ] (GM.ALPHA.T006) Gate GraphQL auto-resolve behind flag; document validation; GH App roadmap.
> [!INFO]- Step-by-step Instructions
> - [ ] Introduce feature flag controlling auto-resolve behavior.
> - [ ] Document validation workflow with screenshots in Review Seeding docs.
> - [ ] Outline GH App roadmap section clarifying future automation.
> - [ ] Add regression tests around new flag default.

<a id="gmmvp005"></a>
- [ ] (GM.MVP.T005) Extend templates; more guides.
> [!INFO]- Step-by-step Instructions
> - [ ] Audit existing docs templates for gaps.
> - [ ] Add missing sections (TOC hints, status guidance).
> - [ ] Update docs tooling lint to confirm compliance.
> - [ ] Announce templates in contributor guide.

<a id="gmmvp006"></a>
- [ ] (GM.MVP.T006) Tune AGENTS summaries; monthly rollup.
> [!INFO]- Step-by-step Instructions
> - [ ] Define monthly rollup process for AGENTS highlights.
> - [ ] Automate summary generation or provide script.
> - [ ] Update AGENTS.md with new cadence instructions.
> - [ ] Verify pre-commit hook enforces formatting.

<a id="gmv1t003"></a>
- [ ] (GM.V1.T003) Optional heuristics for worksheet gates.
> [!INFO]- Step-by-step Instructions
> - [ ] Define heuristic rules (placeholder detection, decision coverage).
> - [ ] Implement optional flag to enable heuristics in gate tooling.
> - [ ] Document opt-in usage and backward compatibility.
> - [ ] Add tests covering heuristic outcomes.

### Observability & Analytics

<a id="gmalpha007"></a>
- [ ] (GM.ALPHA.T007) Structured logging; sampling options.
> [!INFO]- Step-by-step Instructions
> - [ ] Design logging schema for CBOR debug tracing.
> - [ ] Implement sampling controls (env/config toggles).
> - [ ] Update ops docs with usage examples.
> - [ ] Add tests ensuring logs redact sensitive content.

<a id="gmmvp007"></a>
- [ ] (GM.MVP.T007) Expose CLI summary; automate weekly snapshots.
> [!INFO]- Step-by-step Instructions
> - [ ] Implement CLI summary command reading ledger progress.
> - [ ] Automate snapshot job (cron/workflow) updating docs.
> - [ ] Document snapshot cadence for contributors.
> - [ ] Add regression test verifying README sync.

<a id="gmv1t004"></a>
- [ ] (GM.V1.T004) Collect tidy counts and failures; render trends.
> [!INFO]- Step-by-step Instructions
> - [ ] Instrument CI to capture tidy counts and unit failures.
> - [ ] Store trend data (JSON/CSV) for dashboards.
> - [ ] Render trend visualization in docs/dashboard.
> - [ ] Document maintenance workflow for trend data.

## Completeness

| Task | Started? | Finished? | Tested? | Shipped? |
|------|----------|-----------|---------|----------|
| [GM.V1.T001](#gmv1t001) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.ALPHA.T001](#gmalpha001) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.ALPHA.T002](#gmalpha002) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.MVP.T001](#gmmvp001) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.V1.T002](#gmv1t002) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.ALPHA.T003](#gmalpha003) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.MVP.T002](#gmmvp002) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.BETA.T001](#gmbeta001) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.ALPHA.T004](#gmalpha004) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.ALPHA.T005](#gmalpha005) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.MVP.T003](#gmmvp003) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.MVP.T004](#gmmvp004) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.ALPHA.T006](#gmalpha006) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.MVP.T005](#gmmvp005) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.MVP.T006](#gmmvp006) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.V1.T003](#gmv1t003) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.ALPHA.T007](#gmalpha007) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.MVP.T007](#gmmvp007) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.V1.T004](#gmv1t004) | 🚫 | 🚫 | 🚫 | 🚫 |

