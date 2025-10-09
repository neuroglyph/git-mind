<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->
<!-- © 2025 J. Kirby Ross / Neuroglyph Collective -->

---
title: Memory Allocation Policy
description: Current ownership rules, allocation patterns, and exploratory ideas for future allocator work.
audience: [contributors]
domain: [architecture]
tags: [memory, hexagonal-architecture]
status: draft
last_updated: 2025-10-09
---

# Memory Allocation Policy & Future Directions

This note captures how the codebase currently allocates memory, the ownership contracts we rely on, and a set of exploratory ideas for improving safety and performance as the hexagonal migration continues. It is intentionally descriptive rather than prescriptive—where it suggests new mechanisms, treat them as options to evaluate, not commitments.

## Table of Contents

- Architectural Context
- Current Allocation & Ownership Patterns
- Requirements & Guardrails
- Known Future Pressures
- Exploratory Ideas (Non-Commitments)
- Decision Checklist for Contributors
- Open Questions

## 1. Architectural Context

- **Domain purity:** Modules under `core/src/domain/**` must remain allocation-free and deterministic. Any heap allocation happens in adapters or application services that sit outside the domain ring.
- **Ports as seams:** Inbound/outbound ports define ownership. When a port method returns dynamically-allocated data, it must document the lifetime and ideally supply a matching `destroy`/`free` helper so callers cannot leak objects.
- **Context wiring:** `gm_context_t` is the composition root. Today it owns concrete adapters (logger, metrics, fs temp, git repo). In the future it should also surface allocator configuration so application services can request memory without hard-coding `malloc`.
- **Result types:** We represent fallible operations with `gm_result_*`. Any allocation failure must bubble up as a `gm_error_t`; caller code is responsible for freeing `result.u.err` when it does not propagate the error.

## 2. Current Allocation & Ownership Patterns

### 2.1 Stack-first buffers

- Hot paths (logging, telemetry tags, ref strings) prefer stack buffers sized by policy constants (`GM_PATH_MAX`, 256 for log messages, etc.).
- The helpers `gm_snprintf` and `gm_strcpy_safe` enforce truncation checks; callers clear buffers or abort work on failure.

### 2.2 Ports returning heap memory

- `gm_fs_temp_port_canonicalize_ex` now returns a heap-allocated canonical path. Callers must free the string after copying or when aborting.
- Similar patterns exist (or will exist) for repo/branch introspection, staged temp directories, and future git-object adapters. Each port is responsible for documenting who frees the memory.

### 2.3 Borrowed pointers

- Some ports still hand out borrowed pointers into internal buffers (e.g., fake FS port scratch space) when the lifetime is aligned with the port call. We require callers to copy if they need longer-lived data. The long-term goal is to eliminate ambiguous borrowing in favor of explicit ownership or caller-supplied buffers.

### 2.4 Ownership transfer in results

- `gm_result_ptr_t` conveys ownership transfer for arbitrary pointers. The receiver is expected to free or otherwise dispose the pointer, often via a helper supplied by the module that produced it.
- For structs (e.g., `gm_tempdir_t`), we encode ownership via embedded pointers: if `tempdir.path` is non-NULL, the adapter remains responsible for cleanup unless ownership is explicitly transferred (this is rare; keep it documented when it happens).

### 2.5 Error objects as allocations

- Every `gm_error_t` is heap-allocated. Ignoring a failed `gm_logger_log` or `gm_metrics_*` call leaks memory. Recent refactors ensure we capture and free those errors immediately after the log/metric attempt.

## 3. Requirements & Guardrails

1. **No hidden ownership:** Any function that returns heap memory must document who frees it and when. Prefer returning a `gm_result_ptr_t` or pairing the pointer with a destroy function pointer.
2. **Single responsibility:** Allocate in the layer that owns lifetime decisions. Domain code never calls `malloc`; services/adapters may, but they must pair allocation with cleanup or transfer ownership explicitly.
3. **Error-path hygiene:** On every failure path, clear or free partial allocations and reset output buffers (e.g., `out_path[0] = '\0'`).
4. **Thread awareness:** New allocations must remain safe under concurrent use. If a helper caches buffers, it must be per-thread or guarded by synchronization.
5. **Extensibility:** Upcoming adapters (git commit walkers, filesystem staging, hook runners) must be able to swap allocation strategies without changing domain code.

## 4. Known Future Pressures

- **Attribution pipelines:** Planned commit-walk adapters will build large in-memory graphs of author/time data. Expect a mix of short-lived iteration buffers and longer-lived aggregates suitable for pooling or arena allocation.
- **Cache service evolution:** Cache rebuilds already materialize sizable maps; future enhancements (e.g., incremental snapshots, multi-branch rebuilds) will multiply concurrent allocations.
- **Hook orchestration:** Process adapters may collect stdout/stderr logs of arbitrary length. We might need streaming buffers that grow without fragmenting the heap.
- **Telemetry batching:** Moving toward structured telemetry could require short-lived allocations per event plus shared interned keys/values.

## 5. Exploratory Ideas (Non-Commitments)

### 5.1 Allocator Port

Create a `gm_allocator_port_t` injected through `gm_context_t`. It would provide function pointers for `alloc`, `realloc`, `free`, and optional diagnostics. Default adapter wraps `malloc`. Tests could supply deterministic arenas to simulate OOM or track leaks.

### 5.2 Arena allocators for services

One idea might be to layer an arena per application service invocation (e.g., journal read, cache rebuild). All temporary allocations flow through the arena and are released wholesale once the operation completes. This helps bound lifetime and eliminates per-object frees.

### 5.3 Small-object pools

Given the prevalence of short-lived structs (telemetry kv pairs, log message buffers, edge metadata), we could offer a slab allocator tuned to common sizes. This would reduce fragmentation without complicating domain code.

### 5.4 String/SHA interning

Interning frequently repeated strings (paths, ref names) and SHA buffers could shrink the working set. A future `gm_intern_table_t` adapter could own a hash table that returns shared immutable slices. Consumers would hold references with explicit ref-counting or borrow semantics guarded by the allocator port.

### 5.5 Reference-counted handles

For data that survives beyond a single service call (e.g., cached snapshots handed to query ports), reference-counted handles might simplify lifetime management. We could define a lightweight `gm_ref_t` that pairs a pointer with release logic. Weak references are likely overkill for now, but the pattern could emerge if UI adapters need long-lived observers.

### 5.6 Borrow checker protocols

If we find frequent “borrow vs own” confusion, one idea might be to standardize naming: `*_borrowed` for pointers valid only within the call stack, `*_owned` for heap pointers that the caller must free, and `*_view` for immutable slices tied to another object’s lifetime.

### 5.7 Telemetry of allocator usage

We could instrument allocator adapters to emit metrics (total bytes, peak usage, allocation failures). This would help tune pool sizes and catch leaks during CI runs.

## 6. Decision Checklist for Contributors

- Does this code run in the domain ring? If yes, avoid heap allocation entirely.
- Am I returning heap memory? Document the lifetime and add a destroy helper if one doesn’t exist.
- Am I ignoring a `gm_result_*` from a logger/metrics/port call? Capture and free any embedded error.
- Is there a clear cleanup path when the function exits early? Ensure buffers are cleared and allocations freed on every branch.
- Would this benefit from a reusable allocator? If you see repeated patterns, log them here or open an issue so we can evaluate arena/slab adoption.

## 7. Open Questions

- Which services justify arenas vs on-demand allocation? (Cache rebuilds and attribution queries are leading candidates.)
- Do we need reference-counted handles for cache snapshots or can we keep push/pull lifetimes simple?
- How should allocator configuration integrate with future configuration files or environment toggles?
- Can we share interning tables across processes, or will they remain in-memory only?

Contributors are encouraged to append observations, benchmarks, or experiments to this document as the architecture evolves.
