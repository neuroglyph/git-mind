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
- `GITMIND_METRICS_REPO_HASH_ALGO=fnv|sha256` (default `fnv`)
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

## Repo Tag Hash Algorithm

- When `GITMIND_METRICS_REPO_TAG=hash`, git‑mind derives a low‑cardinality repo identifier from the canonical gitdir path (or a stable repo_id fallback).
- Control the algorithm via `GITMIND_METRICS_REPO_HASH_ALGO`:
  - `fnv` (default): 64‑bit FNV‑1a; first 12 hex chars.
  - `sha256`: SHA‑256 digest; first 12 hex chars.
- Both options are stable and privacy‑safe; `sha256` may be preferable in environments with crypto policy requirements. Change does not affect metric names or tags beyond the repo value.

## Custom Logger Adapters (Extension Point)

The logging port (`gm_logger_port`) is intentionally minimal — one `log(level, component, message)` entry point. This keeps the public ABI stable and makes it easy to swap adapters.

To integrate your logging backend:

1. Implement the adapter

```
#include "gitmind/ports/logger_port.h"

typedef struct { /* your state */ } my_logger_state_t;

static gm_result_void_t log_impl(void *self, gm_log_level_t level,
                                 const char *component, const char *message) {
  /* Ship to syslog/journald/OpenTelemetry/etc. */
  return gm_ok_void();
}

static const gm_logger_port_vtbl_t VTBL = { .log = log_impl };

gm_result_void_t my_logger_init(gm_logger_port_t *port, my_logger_state_t *st) {
  port->vtbl = &VTBL;
  port->self = st; /* caller manages lifetime */
  return gm_ok_void();
}
```

2. Wire it at composition

In your runtime/composition code (CLI, service), initialize your adapter and assign it to `ctx->logger_port` (and optionally `ctx->logger_port_dispose`). Application services will start emitting logs through your adapter automatically.

3. Structured output + formatter DI seam (advanced)

- Services format log entries consistently (event name + key/values). When `GITMIND_LOG_FORMAT=json`, messages are JSON — adapters can parse or pass‑through as needed. If your adapter natively supports structured fields, you can ignore the final string and map the known fields directly.
- Advanced: A formatting DI seam is available via `gm_context_t.log_formatter` (internal). To override, assign a function with signature `gm_log_formatter_fn` that renders either JSON or text based on the `json` flag. If unset, the default renderer is used.

4. Testing

- Use the provided fakes (`core/tests/fakes/logging` and `core/tests/fakes/metrics`) as patterns for deterministic tests of your adapter wiring.

## Quickstart

Enable metrics with repo hash and two org‑specific tags:

```
export GITMIND_METRICS_ENABLED=1
export GITMIND_METRICS_REPO_TAG=hash
export GITMIND_METRICS_REPO_HASH_ALGO=sha256   # or fnv
export GITMIND_METRICS_EXTRA_TAGS="team=dev,role=ops"
```

## Diagnostics (Dev/Test)

For low-volume debugging breadcrumbs (separate from logs/metrics), see Diagnostics Events. In the CLI, you can enable a stderr diagnostics sink by setting:

```
export GITMIND_DEBUG_EVENTS=1
```

When enabled, failure edges in cache/journal will print structured one-liners to stderr (and are also capturable in tests via a fake diagnostics sink). See: Diagnostics Events (docs/operations/Diagnostics_Events.md).
