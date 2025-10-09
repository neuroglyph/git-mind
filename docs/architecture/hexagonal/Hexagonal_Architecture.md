<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->
<!-- © 2025 J. Kirby Ross / Neuroglyph Collective -->

---
title: Hexagonal Architecture Overview
description: Structure, layering rules, and migration guidance for GitMind's core services.
audience: [contributors, developers]
domain: [architecture]
tags: [hexagonal, architecture, layering]
status: draft
last_updated: 2025-10-09
---

# Hexagonal Architecture Overview

The GitMind core is transitioning to a hexagonal (ports-and-adapters) design so that domain logic remains pure, testable, and independent of infrastructure details. This document captures the target structure and the expectations contributors must follow.

## Table of Contents

- Goals & Principles
- Layering Model
- Port Taxonomy
- Adapter Responsibilities
- Composition & Context Wiring
- Migration Guidelines
- Verification Checklist
- Open Questions

## 1. Goals & Principles

- **Single Responsibility:** Each translation unit implements one concept (domain entity, port facade, adapter, etc.).
- **Pure Domain Core:** Domain modules own business rules and should be deterministic, allocation-free, and free of IO.
- **Dependency Injection:** All side effects flow through ports injected into services via `gm_context_t` (or future context structs).
- **Test Double Friendly:** Every outbound port must have a fake in `core/tests/fakes/**` so unit tests never touch real infrastructure.
- **No Hidden State:** Global singletons are being removed; services express their dependencies explicitly in constructor/init helpers.

## 2. Layering Model

We organize code into concentric layers:

| Layer | Location | Responsibilities |
| --- | --- | --- |
| **Domain Core** | `core/src/domain/**`, headers under `core/include/gitmind/<domain>/` | Entities, value objects, pure functions, validation logic. Never touch IO or context. |
| **Application Services** | `core/src/app/**` | Orchestrate use cases, manage transactions, translate inbound intents to domain calls. All dependencies provided via ports. |
| **Inbound Ports** | `core/include/gitmind/ports/*_command_port.h`, `*_query_port.h`, `*_admin_port.h` | Define entry points for CLI, hooks, or background schedulers. Own request/response contracts and surface `gm_result_*`. |
| **Outbound Ports** | `core/include/gitmind/ports/**` | Declare dependencies on infrastructure (git repo, filesystem, clock, etc.). Provide helper destroy functions when they allocate. |
| **Adapters (Runtime)** | `core/src/adapters/<area>/**` | Concrete implementations that talk to libgit2, filesystem, env, logging, telemetry, etc. |
| **Adapters (Test)** | `core/tests/fakes/**` | Deterministic fakes/stubs implementing outbound ports for unit tests. |
| **Composition Roots** | `core/src/app/cli_composition.c`, future background composition files | Assemble contexts, wire ports to adapters, register allocators. |

Crossing inward (toward the domain) requires an explicit interface dependency; domain code never imports adapter headers.

## 3. Port Taxonomy

- **Inbound (driving) ports** handle commands and queries. Examples: `gm_cmd_journal_port_vtbl`, `gm_cmd_cache_build_port_vtbl`, `gm_qry_cache_port_vtbl`.
- **Outbound (driven) ports** encapsulate side effects: git repository, refs, filesystem temp directories, logger, metrics, entropy, clock, process, env.
- **Future additions** under discussion include an allocator port, metrics budget tracker, and git-object walkers. Design new ports as POD structs containing function pointers plus optional context pointer.
- Every outbound port must ship with:
  - Runtime adapter in `core/src/adapters/...`
  - Fake/deterministic implementation under `core/tests/fakes/...`
  - Contract tests ensuring behaviour matches documentation.

## 4. Adapter Responsibilities

- Maintain ownership clarity: adapters that allocate memory must document and expose a matching free/destroy helper.
- Avoid caching mutable global state; prefer adapter instances stored in context structs.
- Respect logging and telemetry guidelines—return `gm_result_void_t`, never drop errors.
- Provide granular init/dispose functions (e.g., `gm_posix_fs_temp_port_create`) so composition roots can assemble adapters cleanly.

## 5. Composition & Context Wiring

- `gm_context_t` is the current runtime context. It should remain a thin aggregate of ports, log formatter function pointer, and configuration.
- Composition roots create concrete adapters, store them in the context, and hand references to application services/inbound ports.
- Future plans include slicing context per bounded context (cache, journal, attribution) to enforce SRP even further.
- Avoid on-demand global initialization; prefer explicit `*_init` helpers invoked during context assembly.

## 6. Migration Guidelines

1. **Touch-Point Rule:** If you edit a file that mixes responsibilities, extract the functionality you touched into a dedicated translation unit consistent with hexagonal boundaries.
2. **Ports First:** When encountering direct IO in application services or domain code, introduce an outbound port before implementing new features.
3. **No IO in Domain:** When porting legacy modules, split IO into adapters, keep computation in `core/src/domain/**`.
4. **Tests:** Unit tests should rely exclusively on fakes; integration tests exercise real adapters inside Docker.
5. **Documentation:** Update `docs/activity/<date>_hexagonal.md` with progress, and note any new ports/adapters in `AGENTS.md` or dedicated architecture docs.

## 7. Verification Checklist

- Does the module obey the layer boundaries (no adapter header includes in domain code)?
- Are dependencies injected through function parameters or context structs instead of static globals?
- If the code allocates memory, is ownership explicit and paired with cleanup?
- Do logs/metrics handle `gm_result_void_t` correctly and free `gm_error_t` on failure?
- Have fakes/tests been updated to reflect new ports or adapter behavior?

## 8. Open Questions

- How should allocator ports integrate with `gm_context_t` without bloating every service? (See `docs/architecture/Memory_Allocation_Policy.md` for ideas.)
- What is the best composition strategy for background daemons that share adapters across long-lived tasks?
- Do we need a code generator or templates for new ports/adapters to avoid boilerplate drift?

Contributors are encouraged to extend this document as the migration advances or when new architectural decisions land.
