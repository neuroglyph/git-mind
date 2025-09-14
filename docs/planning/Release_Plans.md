# Release Plans

Table of Contents

- [Executive Summary](#executive-summary)
- [Release Sequence Overview](#release-sequence-overview)
- [Detailed Release Scope](#detailed-release-scope)
- [Risk Assessment Matrix and Mitigations](#risk-assessment-matrix-and-mitigations)
- [Acceptance Criteria per Release](#acceptance-criteria-per-release)

## Executive Summary

Define a sequence of small, self-contained releases prior to v1.0.0. Each release is independently valuable, can be paused safely, and avoids calendar dates. See also: [Product Roadmap](Product_Roadmap.md) and [Technical Specifications](../specs/Technical_Specifications.md).

## Release Sequence Overview

- Pre-1.0 releases: 7 (v0.4.0 → v0.10.0)
- Optional post‑0.10: v0.11.0 Co‑Thought MCP (tools integration)
- v1.0.0 follows once acceptance criteria across journal, cache, CLI, semantics/advice, and CI/quality are met.

## Detailed Release Scope

Release v0.4.0 — Journal Foundations

- Journal writer/reader for attributed edges (CBOR in refs/gitmind/edges/<branch>)
- AUGMENTS post-commit hook prototype
- CLI: link/list minimal; legacy decoders tolerated

Release v0.5.0 — CLI & UX Essentials

- CLI: link/list/unlink/status with human/JSON outputs
- Robust path→blob resolution, errors, and help text
- Golden fixtures for CLI outputs

Release v0.6.0 — Cache & Query Performance

- Roaring Bitmap cache builder (forward/reverse); incremental rebuilds
- Query by blob SHA and simple filters; baseline perf docs

Release v0.7.0 — Semantics Foundations

- Deterministic helpers: `gm_sem_type_id`, `gm_sem_lane_id` (NFC + hash)
- Extend edge CBOR with `type_name`, `lane_name` (tolerant decoders)
- CLI accepts arbitrary names; legacy rel_type mapped if present
- Unit tests: hash vectors; round-trip CBOR (with/without names)

Release v0.8.0 — Cache/Query Integration

- Cache builder reads names → IDs; query by name routed via helpers
- Update list/link paths to respect branch scoping
- Cohesion report CLI (basic): scalar flips and set diffs
- Performance baseline: median query < 10ms on 100k edges sample

Release v0.9.0 — Advice (Optional) and Hooks

- Advice record format + parser; hybrid CRDT merge rules implemented
- Minimal application: `symmetric` and `implies` in list/cohesion report
- Define plugin hook points; ship no-op stub runner and tests

Release v0.10.0 — CI/Quality and Packaging

- CI gates: gcc-14/clang-20; non-blocking canaries for older compilers
- clang-tidy: zero warnings in cache/journal/cbor modules
- Docker images standardized and cleanup tools added
- Packaging: release archives with checksums; versioning policy docs

Release v0.11.0 — Co‑Thought MCP (Optional)

- MCP service for local tools/agents to read/write edges (stable surface)
- Lane conventions documented (suggested/verified) and enforced by CLI flags
- Filters by attribution and lane wired into CLI and library

## Risk Assessment Matrix and Mitigations

| Risk | Probability | Impact | Mitigation | Contingency |
|------|-------------|--------|------------|-------------|
| Hash instability across platforms | Medium | 4 | Specify algorithm (FNV-1a-64 over NFC UTF-8) + test vectors | Lock alternative (SipHash-1-3) behind macro and reindex cache |
| CBOR b/w compatibility regressions | Low | 4 | Tolerant decoders; golden tests | Feature flag readers; fallback to legacy fields |
| Cache mismatch after semantics change | Medium | 3 | Rebuilder invalidates on schema version bump | Auto full rebuild when mismatch detected |
| Advice merge complexity/perf | Medium | 3 | Cap set sizes; limit depth; iterative approach | Disable advice application via flag |
| Toolchain mismatch in CI | Medium | 3 | Pin images; verify versions in pipeline | Fallback matrix; mark canaries non-blocking |
| Single-maintainer bandwidth | High | 3 | Small releases; clear stopping points | Defer optional features to next release |
| Windows portability | Medium | 3 | CI job for MSYS/MinGW or WSL; path handling tests | Limit support to POSIX initially |
| libgit2/CRoaring version drift | Medium | 3 | Pin versions; vendor fallback | Vendor source builds in CI images |

## Acceptance Criteria per Release

v0.7.0

- Hash helpers return stable results across runs/platforms (golden tests)
- Edge CBOR round-trips with and without names, zero data loss
- CLI accepts and persists arbitrary `type_name`/`lane_name`

v0.8.0

- Query by name yields identical results to direct ID queries
- Cohesion report surfaces scalar flips and set diffs for a test merge
- Median query < 10ms on sample dataset; documented runbook

v0.9.0

- Advice parser loads records; merge results deterministic (unit tests)
- `symmetric` and `implies` applied correctly on list/cohesion outputs
- Plugin hooks documented; stub runner enabled/disabled via flag

v0.10.0

- CI fails if compiler < C23 (gcc-14/clang-20 gate)
- clang-tidy shows zero warnings in cache/journal/cbor
- Release archives built with checksums; version tags and changelog entries

v0.11.0 (optional)

- MCP service starts/stops locally; reads/writes edges via stable protocol
- AI‑suggested edges appear under `suggested` lane and filter by `--source ai`
- Review flow documented and tested (promote to `verified` lane)
