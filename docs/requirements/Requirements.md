# Requirements

Table of Contents
- [Executive Summary](#executive-summary)
- [Functional Requirements](#functional-requirements-by-feature)
- [Non-Functional Requirements](#non-functional-requirements)
- [User Stories and Acceptance Criteria](#user-stories-with-acceptance-criteria)
- [Definition of Done](#definition-of-done)

## Executive Summary
Requirements reflect a Git-native, offline-first tool with fast local queries and auditable semantics. Scope favors deterministic behavior, safety, and minimal operational friction for a solo maintainer. Related: [Technical Specifications](../specs/Technical_Specifications.md).

## Functional Requirements (by feature)

Semantics
- FR-SEM-001: CLI accepts arbitrary `--type` and `--lane` strings
- FR-SEM-002: Edge CBOR stores `type_name` and `lane_name`
- FR-SEM-003: Deterministic IDs derived from names for cache keys

Cache/Query
- FR-CACHE-001: Name filters map to IDs; results equal ID-based queries
- FR-CACHE-002: Incremental rebuilds from journal tip
- FR-CACHE-003: Rebuild on schema mismatch

Cohesion Report
- FR-COH-001: Report scalar flips and set diffs across two refs
- FR-COH-002: JSON and table output modes

Advice (optional)
- FR-ADV-001: Parser loads advice records
- FR-ADV-002: Merge via hybrid CRDT (LWW scalars, OR-Set collections)
- FR-ADV-003: Apply `symmetric` and `implies` when enabled

Interop
- FR-INT-001: Provide optional export/import in open formats (CSV/JSON) for external analysis (out of repo scope)

Co‑Thought (Optional)
- FR-AI-001: Edges carry attribution (human/AI) and lanes (e.g., suggested, verified)
- FR-AI-002: CLI and library filter by `--source` and `--lane`
- FR-AI-003: Provide a local MCP service for tools to read/write edges (optional module)
- FR-AI-004: Support promote/demote flows to move edges between lanes

## Non-Functional Requirements
- NFR-001: Median list query < 10ms on 100k edges; P95 < 50ms
- NFR-002: Deterministic behavior across platforms (hash vectors)
- NFR-003: clang-tidy zero warnings in cache/journal/cbor
- NFR-004: CI gates on gcc-14/clang-20 toolchains
- NFR-005: No global state leaks; safe wrappers for unsafe APIs
- NFR-006: Privacy by default — no network dependencies in core; MCP is local‑only
- NFR-007: Deterministic merges for advice; no non‑deterministic behavior in co‑thought flows

## User Stories (with acceptance criteria)
US-01: As a user, I can link using any `--type`/`--lane` name.
- Acceptance: Names are persisted and visible in list outputs.
- Scope: ~50 LOC; O(1) lookups for option dispatch.

US-02: As a user, I can query by type/lane name with fast results.
- Acceptance: Name and ID queries return identical result sets; median < 10ms on sample.
- Scope: ~150 LOC; O(log N) bitmap lookups.

US-03: As a maintainer, I can view a cohesion report after merges.
- Acceptance: JSON and table modes; shows scalar flips and set diffs; stable ordering.
- Scope: ~200 LOC; O(k) where k is changed advice/edges.

US-04: As a maintainer, I can export a graph snapshot in CSV/JSON for external analysis.
- Acceptance: Export produces well-formed CSV/JSON with edge/node fields; author defaults from `git config`.
- Scope: ~100 LOC shell; O(N) over edges.

## Definition of Done
- Code builds in gcc-14 and clang-20
- Tests green (unit, integration); coverage meets targets
- clang-tidy zero warnings for in-scope modules
- Documentation updated (CLI help, specs, changelog)
