---
title: Hexagonal Architecture Overview
description: Definitive reference for GitMind's hexagonal architecture boundaries, layers, and migration workflow.
audience: [contributors]
status: draft
domain: architecture
tags: [hexagonal, overview]
---

# Hexagonal Architecture Overview

## Table of Contents

- [Layering Model](#layering-model)
- [Domain Modules](#domain-modules)
- [Inbound Ports](#inbound-ports)
- [Outbound Ports](#outbound-ports)
- [Adapter Placement](#adapter-placement)
- [Cross-Cutting Seams](#cross-cutting-seams)
- [File Layout](#file-layout)
- [Migration Workflow](#migration-workflow)
- [Execution Guardrails](#execution-guardrails)

## Layering Model

- **Domain Core** — Pure, deterministic C23 modules that capture graph semantics, cache/journal rules, attribution math, and result types. No direct I/O, libgit2, or global state.
- **Application Services** — Thin use-case coordinators that translate inbound intents into domain operations, orchestrate ports, and aggregate results.
- **Ports Layer** — Declarations of every driving/driven dependency as POD structs with function pointers for dependency injection.
- **Adapters Layer** — Concrete implementations for libgit2, filesystem, logging, metrics, CLI, and test doubles (`core/src/adapters/**`, `core/tests/fakes/**`).

## Domain Modules

- **Edge Graph Domain** (`core/src/domain/edge/**`, `core/include/gitmind/edge*.h`)
  - Entities: `gm_edge_t`, attributed payloads, ULID/OID identity, merge rules.
  - Services: validation, CRDT merge (OR-set + advice LWW), hashing, equality.
  - Ports consumed: clock, entropy, attribution catalog.
- **Journal Domain** (`core/src/domain/journal/**`, `core/include/gitmind/journal.h`)
  - Entities: journal entry, append log, branch history, ref targeting.
  - Services: append, snapshot, branch repair, legacy migration.
  - Ports consumed: git ref writer, object store, clock, temp filesystem, logging.
- **Cache Domain** (`core/src/domain/cache/**`, `core/include/gitmind/cache*.h`)
  - Entities: cache snapshot, index, query predicates, builder state machine.
  - Services: hydration, query execution, invalidation, size budgeting.
  - Ports consumed: git tree/object reader, workspace filesystem, metrics, logging.
- **Attribution Domain** (`core/src/domain/attribution/**`, `core/include/gitmind/attribution.h`)
  - Entities: author timeline, attribution spans, aggregation windows.
  - Services: blame stitching, time bucketing, ancestry traversal.
  - Ports consumed: git commit walker, clock, timezone helpers.
- **Hooks & Integrations** (`core/src/domain/hooks/**`, `core/include/gitmind/hooks/**`)
  - Entities: hook definitions, event payloads, execution outcomes.
  - Services: scheduling, policy enforcement, failure handling.
  - Ports consumed: process runner, environment reader, filesystem, logging.
- **Support Domains** — Shared deterministic helpers kept pure: `error/result`, `time`, `utf8`, `util`, `crypto`.

## Inbound Ports

| Port | Purpose | Interface | Consumers |
| --- | --- | --- | --- |
| `gm_cmd_journal_port_vtbl` | Append/read journal entries, manage branches | `core/include/gitmind/ports/journal_command_port.h` | CLI, future server |
| `gm_cmd_cache_build_port_vtbl` | Trigger cache rebuild/invalidations | `core/include/gitmind/ports/cache_build_port.h` | CLI, background workers |
| `gm_qry_cache_port_vtbl` | Execute cache queries | `core/include/gitmind/ports/cache_query_port.h` | CLI, integrations |
| `gm_qry_attribution_port_vtbl` | Produce attribution reports | `core/include/gitmind/ports/attribution_query_port.h` | CLI, UI |
| `gm_cmd_hook_port_vtbl` | Register/dry-run hooks | `core/include/gitmind/ports/hook_command_port.h` | CLI, automation |
| `gm_admin_health_port_vtbl` | Health, diagnostics, warning budgets | `core/include/gitmind/ports/admin_health_port.h` | CI watchdogs |

Inbound ports return `gm_result_t` variants capturing success, warnings, and remediation hints.

## Outbound Ports

| Port | Purpose | Interface | Default Adapter |
| --- | --- | --- | --- |
| `gm_git_repository_port` | Commit/tree/ref I/O | `core/include/gitmind/ports/git_repository_port.h` | `core/src/adapters/libgit2/repository_adapter.c` |
| `gm_git_ref_port` | Ref create/update/delete | `core/include/gitmind/ports/git_ref_port.h` | `core/src/adapters/libgit2/ref_adapter.c` |
| `gm_fs_port` | Filesystem operations | `core/include/gitmind/ports/filesystem_port.h` | `core/src/adapters/fs/posix_filesystem_adapter.c` |
| `gm_clock_port` | Wall/monotonic clocks | `core/include/gitmind/ports/clock_port.h` | `core/src/adapters/time/system_clock_adapter.c` |
| `gm_entropy_port` | ULID randomness | `core/include/gitmind/ports/entropy_port.h` | `core/src/adapters/crypto/os_entropy_adapter.c` |
| `gm_logger_port` | Structured logging | `core/include/gitmind/ports/logger_port.h` | `core/src/adapters/logging/stdio_logger_adapter.c` |
| `gm_metrics_port` | Counters/gauges | `core/include/gitmind/ports/metrics_port.h` | `core/src/adapters/metrics/null_metrics_adapter.c` |
| `gm_process_port` | Spawn hook processes | `core/include/gitmind/ports/process_port.h` | `core/src/adapters/process/posix_process_adapter.c` |
| `gm_env_port` | Environment/config | `core/include/gitmind/ports/env_port.h` | `core/src/adapters/config/env_adapter.c` |
| `gm_thread_port` | Background scheduling | `core/include/gitmind/ports/thread_port.h` | `core/src/adapters/threading/null_thread_adapter.c` |

Every outbound port ships with a production adapter, a deterministic fake under `core/tests/fakes`, and contract tests.

## Adapter Placement

- Runtime adapters live in `core/src/adapters/<area>/<name>_adapter.c` with private headers.
- Composition roots:
  - `core/src/app/cli_composition.c` — wires CLI inbound ports to concrete adapters.
  - `core/src/app/background_tasks.c` — future daemon wiring.
  - Tests compose against fakes under `core/tests/support/**`.
- Each adapter exposes a factory returning the port vtable plus state and cleanup hooks.

## Cross-Cutting Seams

- `gm_result_t` encapsulates success vs error vs warnings; no raw ints.
- Warning budgets flow through `gm_logger_port` to enforce “warnings == bugs”.
- `gm_runtime_context_t` carries injected dependencies; composition roots populate once and thread downward.
- Feature toggles register optional adapters but default to safe stubs.

## File Layout

- `core/src/domain/**` — pure domain logic per bounded context.
- `core/include/gitmind/ports/**` — inbound/outbound port definitions.
- `core/src/ports/**` — inbound coordinators implemented in C when needed.
- `core/src/app/**` — application services for specific use cases (e.g., cache rebuild).
- `core/src/adapters/**` — infrastructure adapters grouped by dependency.
- `core/tests/fakes/**` — deterministic fakes/mocks/stubs per port.
- `apps/cli/src/**` — UI adapters translating CLI flags into port calls.
- `docs/architecture/**` — living documentation (this directory).

## Migration Workflow

1. **Port scaffolding sweep** — introduce skeleton headers for inbound/outbound ports with `gm_result_t` signatures and stub adapters returning `GM_ERR_NOT_IMPLEMENTED`.
2. **Module-by-module extraction** — split pure logic into `core/src/domain/<module>/`, wrap side effects behind outbound ports, and publish inbound coordinators per use case.
3. **Adapter fulfillment** — replace stubs with real adapters, starting with libgit2 repository/ref access, filesystem I/O, and clock/entropy providers.
4. **Composition wiring** — update app/CLI entry points to build a `gm_runtime_context_t` and pass dependencies explicitly; eliminate legacy singletons.
5. **Testing convergence** — build contract tests for each port (real adapters in container, fakes in unit tests).
6. **Warning budget enforcement** — track clang-tidy diagnostics per module; merges blocked unless touched modules are warning-free.
7. **Documentation cadence** — log each migration in `docs/activity/` and refresh this overview as ports/adapters graduate from TODO to done.

## Execution Guardrails

- Boundary-first refactors: wrap I/O immediately when encountered.
- Ports & adapters only: new functionality enters via inbound ports; composition roots bind concretes.
- SRP enforcement: one responsibility per translation unit, keep domain logic pure.
- Guardrails always on: run `make ci-local` (Docker) for every commit; touched files must be clang-tidy clean.
- Document progress: update `AGENTS.md` and `docs/activity/` as modules move forward.
