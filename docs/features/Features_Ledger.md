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

1. **Capture (Planned)** — When a new idea lands (roadmap review, retro, support signal), add a row under the appropriate group with a short name, owning area, and initial KLoC estimate. Link to any discovery notes/spec drafts in the “Spec” column and file a task in the `## Tasklist

![Task Dependency Map](images/task_dag.svg)

### MVP

#### Core

<a id="gmmvpt001"></a>
- [ ] (GM.MVP.T001) Finalize strict equality
> [!INFO]- Step-by-step Instructions
> - [ ] Audit equality helpers in cache and journal to ensure OID-first comparisons never fall back when OIDs exist.
> - [ ] Add regression tests covering mixed OID/string paths across cache and journal utilities.
> - [ ] Update `docs/TECHNICAL.md` and any relevant ADRs to document strict equality behavior.
> - [ ] Sweep references to legacy SHA fallbacks in docs and CLI examples.
> [!NOTE]- Dependencies
> - (none)

<a id="gmmvpt002"></a>
- [ ] (GM.MVP.T002) Negative tests for ref utils
> [!INFO]- Step-by-step Instructions
> - [ ] Extend `test_ref_utils` to cover invalid prefixes, length overflows, and reserved refs.
> - [ ] Ensure errors propagate with meaningful `GM_ERR_*` codes.
> - [ ] Update `docs/architecture/Ref_Name_Validation.md` with new failure scenarios.
> - [ ] Mention failure scenarios in README/tutorial examples.
> [!NOTE]- Dependencies
> - **Hard:** [GM.MVP.T001](#gmmvpt001)

#### CLI

<a id="gmmvpt003"></a>
- [ ] (GM.MVP.T003) Extend CLI safety tests
> [!INFO]- Step-by-step Instructions
> - [ ] Expand CLI safety guard tests to cover host override permutations.
> - [ ] Document the override matrix in `docs/operations/Environment_Variables.md`.
> - [ ] Highlight safe override usage in README/CLI help.
> - [ ] Verify behavior in Docker CI harness.
> [!NOTE]- Dependencies
> - (none)

<a id="gmmvpt004"></a>
- [ ] (GM.MVP.T004) Query subcommands & diagnostics
> [!INFO]- Step-by-step Instructions
> - [ ] Design CLI syntax and update command help output for new query modes.
> - [ ] Implement parsing and output formatting for diagnostic flags.
> - [ ] Add unit/integration tests for diagnostic output.
> - [ ] Update tutorial examples with new query flows.
> [!NOTE]- Dependencies
> - **Hard:** [GM.MVP.T003](#gmmvpt003)

#### Docs

<a id="gmmvpt005"></a>
- [ ] (GM.MVP.T005) Extend docs templates
> [!INFO]- Step-by-step Instructions
> - [ ] Audit existing document templates for missing sections (status, TOC guidance, review notes).
> - [ ] Add missing sections and refresh template examples.
> - [ ] Update docs tooling lint to enforce new expectations.
> - [ ] Announce template changes in the contributor guide.
> [!NOTE]- Dependencies
> - (none)

<a id="gmmvpt006"></a>
- [ ] (GM.MVP.T006) Tune AGENTS monthly rollup
> [!INFO]- Step-by-step Instructions
> - [ ] Define the monthly rollup cadence for AGENTS summaries.
> - [ ] Provide a script or automation to generate rollups.
> - [ ] Update `AGENTS.md` with cadence instructions.
> - [ ] Verify the pre-commit hook enforces the new format.
> [!NOTE]- Dependencies
> - **Soft:** [GM.MVP.T005](#gmmvpt005)

#### Observability

<a id="gmmvpt007"></a>
- [ ] (GM.MVP.T007) CLI summary & snapshots
> [!INFO]- Step-by-step Instructions
> - [ ] Implement CLI summary command reading ledger progress.
> - [ ] Automate a cron/workflow to publish weekly snapshots.
> - [ ] Document the snapshot cadence and expectations.
> - [ ] Add regression test verifying README/ledger sync.
> [!NOTE]- Dependencies
> - **Soft:** [GM.MVP.T005](#gmmvpt005)

### Alpha

#### Core

<a id="gmalphat001"></a>
- [ ] (GM.ALPHA.T001) Journal ordering tests
> [!INFO]- Step-by-step Instructions
> - [ ] Write table-driven tests exercising journal iteration order across branches.
> - [ ] Cover both plain and attributed edges in CBOR fixtures.
> - [ ] Document ordering guarantees in journal reader docs.
> - [ ] Wire new tests into `meson.build` and CI.
> [!NOTE]- Dependencies
> - **Hard:** [GM.MVP.T001](#gmmvpt001)

<a id="gmalphat002"></a>
- [ ] (GM.ALPHA.T002) Cache perf tuning & benchmarks
> [!INFO]- Step-by-step Instructions
> - [ ] Capture baseline rebuild/query timings across representative repos.
> - [ ] Profile shard fan-out hotspots and prototype optimizations.
> - [ ] Automate a Docker-safe benchmark harness for regressions.
> - [ ] Document findings in `docs/architecture/bitmap-cache-design.md`.
> [!NOTE]- Dependencies
> - **Hard:** [GM.ALPHA.T001](#gmalphat001)

<a id="gmalphat003"></a>
- [ ] (GM.ALPHA.T003) Edge diff coverage
> [!INFO]- Step-by-step Instructions
> - [ ] Create fixture branches with divergent edge histories to stress diffs.
> - [ ] Add integration tests comparing fan-in/out across merge commits.
> - [ ] Ensure cache/journal diff helpers handle branch fan-out scenarios.
> - [ ] Document the harness in `docs/architecture/System_Architecture.md`.
> [!NOTE]- Dependencies
> - **Hard:** [GM.ALPHA.T002](#gmalphat002)

<a id="gmalphat004"></a>
- [ ] (GM.ALPHA.T004) Sandbox transactions
> [!INFO]- Step-by-step Instructions
> - [ ] Implement `gm_txn_start/apply/abort` helpers manipulating pending refs.
> - [ ] Build CLI plumbing (`git mind txn ...`) with dry-run/apply flows.
> - [ ] Add local/server hook snippets to enforce provenance.
> - [ ] Ship documentation updates (RFC, roadmap, tutorial snippet).
> [!NOTE]- Dependencies
> - **Hard:** [GM.ALPHA.T003](#gmalphat003)
> - **Hard:** [GM.MVP.T002](#gmmvpt002)

#### CLI

<a id="gmalphat005"></a>
- [ ] (GM.ALPHA.T005) Post-commit AUGMENTS wiring
> [!INFO]- Step-by-step Instructions
> - [ ] Finish journal read integration and handle path truncation errors.
> - [ ] Add end-to-end test demonstrating AUGMENTS edges emitted via hooks.
> - [ ] Document error/fallback behavior in hook guides.
> - [ ] Verify containerized tests cover the new path.
> [!NOTE]- Dependencies
> - **Hard:** [GM.ALPHA.T004](#gmalphat004)

<a id="gmalphat006"></a>
- [ ] (GM.ALPHA.T006) GraphQL auto-resolve flag
> [!INFO]- Step-by-step Instructions
> - [ ] Introduce a feature flag controlling auto-resolve behavior.
> - [ ] Document validation workflow with screenshots in Review Seeding docs.
> - [ ] Outline GH App roadmap clarifying future automation.
> - [ ] Add regression tests around the new flag default.
> [!NOTE]- Dependencies
> - **Hard:** [GM.MVP.T004](#gmmvpt004)

#### Observability

<a id="gmalphat007"></a>
- [ ] (GM.ALPHA.T007) Structured logging & sampling
> [!INFO]- Step-by-step Instructions
> - [ ] Design logging schema for CBOR debug tracing.
> - [ ] Implement sampling controls (environment/config toggles).
> - [ ] Update operations docs with usage examples.
> - [ ] Add tests ensuring logs redact sensitive content.
> [!NOTE]- Dependencies
> - **Hard:** [GM.ALPHA.T004](#gmalphat004)

### Beta

#### Core

<a id="gmbetat001"></a>
- [ ] (GM.BETA.T001) Multi-author attribution rollups
> [!INFO]- Step-by-step Instructions
> - [ ] Extend attribution data structure to track multiple authors and time windows.
> - [ ] Update cache/journal serialization to carry new metadata.
> - [ ] Add CLI flags to display attribution columns.
> - [ ] Document workflow and examples in attribution docs.
> [!NOTE]- Dependencies
> - **Hard:** [GM.ALPHA.T004](#gmalphat004)
> - **Soft:** [GM.MVP.T002](#gmmvpt002)

### v1.0.0

#### Core

<a id="gmv1t001"></a>
- [ ] (GM.V1.T001) Strict equality enforcement
> [!INFO]- Step-by-step Instructions
> - [ ] Harmonize equality helpers across cache/journal/util modules.
> - [ ] Add regression tests covering strict equality edges-to-cache interactions.
> - [ ] Document long-term guarantees in architecture docs.
> - [ ] Ensure release notes highlight the enforcement.
> [!NOTE]- Dependencies
> - **Hard:** [GM.ALPHA.T001](#gmalphat001)
> - **Hard:** [GM.MVP.T001](#gmmvpt001)

<a id="gmv1t002"></a>
- [ ] (GM.V1.T002) Safe-string sweep
> [!INFO]- Step-by-step Instructions
> - [ ] Grep for raw `strcpy`/`sprintf` usage outside approved wrappers.
> - [ ] Replace with `gm_snprintf`/`gm_strcpy_safe` equivalents.
> - [ ] Update safety documentation and examples.
> - [ ] Add unit coverage where new wrappers are introduced.
> [!NOTE]- Dependencies
> - **Hard:** [GM.MVP.T001](#gmmvpt001)

#### CLI

<a id="gmv1t003"></a>
- [ ] (GM.V1.T003) Worksheet heuristics
> [!INFO]- Step-by-step Instructions
> - [ ] Define heuristic rules (placeholder detection, decision coverage).
> - [ ] Implement optional flag enabling heuristics in gate tooling.
> - [ ] Document opt-in usage and backward compatibility.
> - [ ] Add tests covering heuristic outcomes.
> [!NOTE]- Dependencies
> - **Hard:** [GM.MVP.T005](#gmmvpt005)
> - **Soft:** [GM.MVP.T006](#gmmvpt006)

#### Observability

<a id="gmv1t004"></a>
- [ ] (GM.V1.T004) CI trend baselines
> [!INFO]- Step-by-step Instructions
> - [ ] Instrument CI to capture tidy counts and unit failures.
> - [ ] Store trend data (JSON/CSV) for dashboards.
> - [ ] Render trend visualization in docs/dashboard.
> - [ ] Document maintenance workflow for trend data.
> [!NOTE]- Dependencies
> - **Hard:** [GM.ALPHA.T007](#gmalphat007)
> - **Soft:** [GM.MVP.T007](#gmmvpt007)

## Completeness

| Task | Started? | Finished? | Tested? | Shipped? |
|------|----------|-----------|---------|----------|
| [GM.MVP.T001](#gmmvpt001) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.MVP.T002](#gmmvpt002) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.MVP.T003](#gmmvpt003) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.MVP.T004](#gmmvpt004) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.MVP.T005](#gmmvpt005) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.MVP.T006](#gmmvpt006) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.MVP.T007](#gmmvpt007) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.ALPHA.T001](#gmalphat001) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.ALPHA.T002](#gmalphat002) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.ALPHA.T003](#gmalphat003) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.ALPHA.T004](#gmalphat004) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.ALPHA.T005](#gmalphat005) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.ALPHA.T006](#gmalphat006) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.ALPHA.T007](#gmalphat007) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.BETA.T001](#gmbetat001) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.V1.T001](#gmv1t001) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.V1.T002](#gmv1t002) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.V1.T003](#gmv1t003) | 🚫 | 🚫 | 🚫 | 🚫 |
| [GM.V1.T004](#gmv1t004) | 🚫 | 🚫 | 🚫 | 🚫 |
