---
title: Milestones
description: Planning artifacts: roadmap, releases, sprints, milestones.
audience: [contributors]
domain: [project]
tags: [planning]
status: archive
last_updated: 2025-09-15
---

# Milestones

Table of Contents

- [Executive Summary](#executive-summary)
- [Milestone List with Deliverables](#milestone-list-with-deliverables)
- [Success Criteria and Dependencies](#success-criteria-and-dependencies)
- [Go/No-Go Decision Points](#gono-go-decision-points)

## Executive Summary

Concrete, verifiable checkpoints that align with the [Product Roadmap](Product_Roadmap.md) and [Release Plans](Release_Plans.md). Each milestone has clear outputs and dependency mapping to simplify resuming work later.

## Milestone List with Deliverables

M0: Repo Hygiene & Docs

- Deliverables: docs index, ADRs updated, planning/quality structure; pre-commit docs linter
- Artifacts: docs/README.md; tools/docs/check_docs.py

M1: Deterministic Semantics IDs

- Deliverables: `gm_sem_type_id`, `gm_sem_lane_id`, NFC utility; test vectors
- Artifacts: header + impl; `tests/test_sem_id.c`

M2: Edge CBOR Names Support

- Deliverables: Encoder/decoder for `type_name`, `lane_name` with tolerant reads
- Artifacts: updated `core/src/cbor/cbor.c`; golden CBOR blobs; docs update

M3: Journal & AUGMENTS

- Deliverables: journal writer/reader; post-commit hook; AUGMENTS edges
- Artifacts: CLI hook, tests, docs

M4: Cache/Query Name Routing

- Deliverables: name→ID routing in cache builder and list queries; CLI accepts names
- Artifacts: updated `core/src/cache/{builder,query}.c`; CLI help pages

M5: Cohesion Report (Basic)

- Deliverables: CLI report of scalar flips + set diffs across merge range
- Artifacts: `git mind cohesion-report`; JSON + table outputs; unit tests

M6: Advice Records + Merge Rules

- Deliverables: advice schema, parser, and hybrid CRDT merge; minimal application
- Artifacts: `core/src/advice/*`; tests for merge determinism

M7: CI C23 Gating

- Deliverables: CI updates to gate on gcc-14/clang-20; canary matrices
- Artifacts: workflow YAML; Dockerfiles; doc note in deployment

M8: Tidy Zero (cache/journal/cbor)

- Deliverables: zero clang-tidy warnings in target modules; safe wrappers
- Artifacts: tidy report in repo; CI check passes

M9: External Export (CSV/JSON)

- Deliverables: export scripts (CSV/JSON) with author defaults from git config
- Artifacts: `scripts/export-graph.*`; example graph snapshot

M10: Co‑Thought MCP (Optional)

- Deliverables: local MCP service for tools to read/write edges; lane conventions and review flags in CLI
- Artifacts: `apps/mcp/` (or equivalent), docs for attribution/lanes, review examples

## Success Criteria and Dependencies

| Milestone | Success Criteria | Depends On |
|-----------|------------------|------------|
| M1 | Hash helpers stable; tests green | — |
| M2 | CBOR round-trip with/without names | M1 |
| M3 | AUGMENTS edges created and visible in queries | M1 |
| M4 | Queries by name match ID results | M1,M2 |
| M5 | Report works on synthetic merge | M4 |
| M6 | Deterministic advice merges; bounded cost | M2 |
| M7 | CI fails when compilers < C23 | — |
| M8 | Zero warnings in scope | M4 |
| M9 | Exported graph opens in common tools | M4 |
| M10 | MCP service reads/writes edges; review flow works | M4,D1 |

## Go/No-Go Decision Points

- After M2: Proceed only if b/w compatibility validated via golden tests.
- After M3: Proceed if performance target (median < 10ms @100k edges) is met.
- After M5: Proceed if advice merge is deterministic and bounded.
