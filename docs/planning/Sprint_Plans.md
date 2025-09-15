---
title: Planning Doc
description: Planning artifacts: roadmap, releases, sprints, milestones.
audience: [contributors]
domain: [project]
tags: [planning]
status: archive
last_updated: 2025-09-15
---

# Sprint Plans

Table of Contents

- [Executive Summary](#executive-summary)
- [Sprint Overviews](#sprint-overviews)
- [User Stories with Acceptance Criteria](#user-stories-with-acceptance-criteria)
- [Task Breakdowns and Estimates](#task-breakdowns-and-estimates)

## Executive Summary

Iterations sized for a solo maintainer. Each sprint is a cohesive unit that can ship or be paused safely. No calendar dates; work proceeds in order. See [Milestones](Milestones.md) for gate criteria.

## Sprint Overviews

- Sprint 1: Journal writer/reader + AUGMENTS + minimal CLI
- Sprint 2: Cache builder + SHA-based query
- Sprint 3: Semantics ID helpers + CBOR names + name routing
- Sprint 4: Cohesion report + advice data model (parser + merge rules)
- Sprint 5: Advice minimal application + CI gating + tidy focus
- Sprint 6: Packaging + export (CSV/JSON) + docs polish
- Sprint 7 (Optional): Co‑Thought MCP + review lanes

## User Stories with Acceptance Criteria

Sprint 1

1) As a user, I can link/list edges stored in a journal.

- Acceptance: CBOR commits under refs/gitmind/edges/<branch>; CLI prints edges.
- Estimate: 8
- Tasks: journal writer/reader; CLI link/list minimal; tests.

2) As a maintainer, I can auto-create AUGMENTS edges on post-commit.

- Acceptance: Editing a tracked file generates AUGMENTS edges; visible via list.
- Estimate: 5
- Tasks: post-commit hook; recent-edge scan; edge creation; tests.

3) As a CLI user, I want helpful errors when paths are invalid.

- Acceptance: Clear error codes and messages; unit tests.
- Estimate: 3
- Tasks: path→blob resolution; error handling.

Sprint 2
4) As a user, I want SHA-based queries to be fast.

- Acceptance: Median query < 10ms on sample dataset; parity with journal scan.
- Estimate: 8
- Tasks: cache builder (forward/reverse); incremental rebuild; perf sample.

5) As a maintainer, I want combined filters (from/to) with cache.

- Acceptance: Combined bitmap operations return expected sets.
- Estimate: 5
- Tasks: query engine set ops; tests.

Sprint 3
6) As a maintainer, I want stable 64-bit IDs from names so cache keys are deterministic.

- Acceptance: Golden tests for Unicode NFC cases pass.
- Estimate: 5
- Tasks: FNV-1a-64; NFC normalize; tests.

7) As a user, I want edges to carry type/lane names so semantics time-travel with Git history.

- Acceptance: Encode/decode round-trip; tolerant decoders.
- Estimate: 8
- Tasks: CBOR updates; fixtures.

8) As a CLI user, I want to specify any type/lane name when linking.

- Acceptance: `git mind link` accepts strings; stored in journal.
- Estimate: 3
- Tasks: option parsing; plumb fields; help text.

Sprint 4
9) As a maintainer, I want a cohesion report to summarize semantic changes.

- Acceptance: JSON+table output; scalar flips and set diffs over a ref-range.
- Estimate: 5
- Tasks: scan journal; summarize; formatters; tests.

10) As a maintainer, I want advice records that merge deterministically.

- Acceptance: Merge unit tests (LWW + OR-Set) are green.
- Estimate: 8
- Tasks: advice schema; parser; merge engine; tests.

Sprint 5
11) As a user, I want minimal advice application (symmetric/implies) in list/report.

- Acceptance: Flags enable behavior; correctness tests.
- Estimate: 5
- Tasks: apply rules in query/report path; guardrails (caps).

12) As a maintainer, I want CI to enforce real C23 compilers.

- Acceptance: Pipeline fails on older compilers; canaries are non-blocking.
- Estimate: 3
- Tasks: workflow updates; Dockerfiles; doc.

13) As a maintainer, I want zero clang-tidy warnings in cache/journal/cbor.

- Acceptance: CI tidy step passes with zero warnings for these modules.
- Estimate: 8
- Tasks: safe wrappers; includes; complexity reductions.

Sprint 6
14) As a maintainer, I want packaged releases (CLI + library).

- Acceptance: Archives + checksums; version tags; changelog.
- Estimate: 5
- Tasks: scripts; documentation.

15) As an analyst, I want CSV/JSON export for graphs with authorship.

- Acceptance: Scripts produce edges/nodes; author defaults from git config.
- Estimate: 5
- Tasks: finalize scripts; sample output; doc.

Sprint 7 (Optional)
16) As a tool author, I want a local MCP service to read/write edges.

- Acceptance: MCP starts locally; CRUD for edges via a stable endpoint.
- Estimate: 8
- Tasks: scaffold service; auth model; tests; docs.

17) As a user, I want to review AI‑suggested edges before adopting them.

- Acceptance: `--lane suggested` + `--source ai` filters in CLI; promote to `verified`.
- Estimate: 5
- Tasks: filters; promote/demote flows; examples; tests.
