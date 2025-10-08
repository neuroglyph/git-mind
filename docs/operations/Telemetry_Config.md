---
title: Telemetry Configuration (Logs + Metrics)
description: Safe, low-cardinality defaults with bounded customization via environment.
audience: [developers]
domain: [operations]
tags: [telemetry, logging, metrics]
status: draft
last_updated: 2025-10-08
---

<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->
<!-- © 2025 J. Kirby Ross / Neuroglyph Collective -->

# Telemetry Configuration (Logs + Metrics)

## Goals

- Low‑cardinality, sensible defaults that are always safe to ship.
- Bounded customization for teams that need 1–3 org‑specific tags.
- No behavior change when telemetry ports aren’t wired (no‑op wrappers).

## Defaults

- Logs (via `gm_logger_port`):
  - component: `cache`, event: `rebuild_start` / `rebuild_ok` / `rebuild_failed`
  - fields at end: `branch`, `mode`, `edge_count`, `duration_ms` (where applicable)
- Metrics (via `gm_metrics_port`):
  - `cache.rebuild.duration_ms` (timing_ms)
  - `cache.edges_processed_total` (counter_add)
  - `cache.tree_size_bytes` (gauge_set, approximate)
  - Tags: `branch=<name>`, `mode=full|incremental` (defaults; can be disabled)

## Configuration (Environment)

Read via `gm_env_port` (or `getenv` fallback until wiring completes). Proposed knobs:

- `GITMIND_METRICS_ENABLED=1|0` (default `1`)
- `GITMIND_METRICS_BRANCH_TAG=1|0` (default `1`)
- `GITMIND_METRICS_MODE_TAG=1|0` (default `1`)
- `GITMIND_METRICS_REPO_TAG=off|hash|plain` (default `off`; `hash` = short hash of canonical repo path)
- `GITMIND_METRICS_EXTRA_TAGS="key1=val1,key2=val2"` (default none; max 3 keys; `[a-z0-9_-]+`)
- `GITMIND_LOG_LEVEL=DEBUG|INFO|WARN|ERROR` (default `INFO`)
- `GITMIND_LOG_FORMAT=text|json` (default `text`)
- `GITMIND_LOG_COMPONENTS="cache,journal"` (default all)

## Guardrails

- Hard cap ≤ 5 tags per metric after merging defaults + extras.
- Validate key/value charset and lengths; drop invalids (log once).
- Don’t put: commit IDs, file paths, ULIDs, timestamps in tags (okay in log body).
- Sampling (future): `GITMIND_METRICS_SAMPLE_PCT=100..1` for high‑rate events.

## Naming Conventions

- `cache.rebuild.duration_ms` — rebuild time in milliseconds
- `cache.edges_processed_total` — total edges processed in rebuild
- `cache.tree_size_bytes` — approximate cache tree size for the branch
- Future (not yet emitted):
  - `cache.rebuild.success_total` / `cache.rebuild.failure_total`
  - `cache.query.hit_total` / `cache.query.miss_total`

## Implementation Plan (Incremental)

1. Shim: Build a small `gm_telemetry_cfg_t` from env — exposed internally only.
2. Instrumentation: Apply config in `cache_rebuild_service` (already emits basic metrics/logs).
3. Fakes + tests: Add deterministic fakes for logger/metrics and a unit test covering tag assembly.
4. Docs: Keep this file and AGENTS.md updated with defaults and knobs.

## Example (Text Log)

```
2025-10-08T05:22:00Z [INFO] cache: rebuild_ok branch=main mode=full edge_count=12345 duration_ms=530
```

## Example (Metrics with tags)

```
cache.rebuild.duration_ms{branch="main",mode="full"} 530
cache.edges_processed_total{branch="main",mode="full"} 12345
cache.tree_size_bytes{branch="main",mode="full"} 9876543
```

