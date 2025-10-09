<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->
<!-- © 2025 J. Kirby Ross / Neuroglyph Collective -->

---
title: Telemetry Strategy
description: Logging and metrics approach, ownership expectations, and future enhancements.
audience: [contributors, developers]
domain: [telemetry]
tags: [logging, metrics, telemetry]
status: draft
last_updated: 2025-10-09
---

# Telemetry Strategy

This document summarizes how GitMind emits logs and metrics today, the ownership rules around telemetry data, and exploratory ideas for improving observability without compromising memory safety.

## Table of Contents

- Objectives
- Current Architecture
- Logging Guidelines
- Metrics Guidelines
- Memory & Ownership Rules
- Testing Expectations
- Future Ideas
- Open Questions

## 1. Objectives

- Provide structured, low-noise telemetry that can be consumed by CI, operators, and future dashboards.
- Keep telemetry orthogonal to domain logic by routing through ports and formatters.
- Ensure logging/metrics failures never leak `gm_error_t` instances or derail primary workflows beyond returning an error result.

## 2. Current Architecture

- **Ports:**
  - `gm_logger_port_t` exposes `gm_logger_log` returning `gm_result_void_t`.
  - `gm_metrics_port_t` exposes `gm_metrics_timing_ms`, `gm_metrics_counter_add`, and related helpers.
- **Formatter seam:** `gm_log_formatter_fn` converts key/value pairs to text or JSON depending on telemetry config (`gm_telemetry_cfg_t`). The formatter lives in `core/src/telemetry/log_format.c` and defaults to JSON (structured) or plain text.
- **Telemetry config:** `gm_telemetry_cfg_load` (in `core/src/telemetry/config.c`) reads environment variables to determine formatter, enabled metrics, and extras budget.
- **Injection:** `gm_context_t` stores the logger port, metrics port, and the active formatter function pointer used by services.

## 3. Logging Guidelines

- Always capture the return value of `gm_logger_log`. If `result.ok` is false, free `result.u.err` immediately unless you propagate it.
- Prefer structured logs using formatter key/value arrays. Include `event`, `branch`, `mode`, `duration_ms`, etc. Text fallbacks should only be used when the formatter fails or truncates.
- Log start/end events for long-running operations (journal read/append, cache rebuild) to aid traceability.
- Treat truncation as an error: if `gm_snprintf` reports truncation while building a fallback message, log an error and continue with an empty message.

## 4. Metrics Guidelines

- Like logging, metrics helpers return `gm_result_void_t`; capture and free errors.
- Guard metric emission with feature flags from `gm_telemetry_cfg_t` (`metrics_enabled`).
- Standard metrics:
  - Duration timers: `*.duration_ms`
  - Counters: `*.edges_total`, `cache.*`, `journal.*`
- Tags (`char tags[256]`) should be built via `gm_telemetry_build_tags`. On failure, clear the tag buffer and continue without tags to avoid emitting stale data.

## 5. Memory & Ownership Rules

- Telemetry helpers may allocate temporary buffers (e.g., canonical paths for repo tags). Callers must copy results into stack buffers and free the original to avoid leaks.
- Error objects (`gm_error_t`) returned from logger/metrics ports are heap allocations. `gm_error_free` must be called unless ownership transfers up the stack via result propagation.
- Formatter functions receive caller-owned buffers (`msg[256]`) and must not retain pointers after returning.

## 6. Testing Expectations

- Unit tests that exercise telemetry paths should use fakes (`core/tests/fakes/metrics/*`, `core/tests/fakes/logging/*`) to assert that calls occur and no errors leak.
- Integration tests running inside Docker verify real adapters but still guard against memory leaks by consuming `gm_result_void_t` outcomes.
- When adding new telemetry, extend tests to validate tag contents and error handling.

## 7. Future Ideas (Non-Commitments)

- **Structured sinks:** One idea might be to add a port for streaming events to JSONL or OpenTelemetry exporters once the CLI integrates with external systems.
- **Telemetry budgets:** A metrics/diagnostics hybrid port could track per-module warning budgets, allowing CI to enforce “warnings == bugs.”
- **Allocator integration:** Telemetry adapters could consume a custom allocator port to decouple buffer management from `malloc`.
- **Sampling/log levels:** Introduce configurable sampling for high-volume events (e.g., cache rebuild progress) to reduce noise.
- **Correlation IDs:** Provide context-level correlation IDs so logs and metrics from the same request are easily grouped.

## 8. Open Questions

- What format should future machine-consumable telemetry use (JSON, protobuf, OTLP)?
- How do we balance detailed diagnostic logs with performance in high-frequency operations?
- Should telemetry failures ever bubble up to callers, or remain best-effort? (Currently best-effort.)
- How can we expose telemetry configuration safely to users without leaking secrets?

As telemetry evolves, update this document alongside code changes to keep operators and contributors aligned.
