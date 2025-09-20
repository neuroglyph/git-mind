---
title: Cache Architecture Overview
description: Map of contents and executive summary for the cache subsystem within the hexagonal architecture.
audience: [contributors]
status: draft
domain: cache
tags: [cache, hexagonal]
---

# Cache Architecture Overview

## Table of Contents

- [Executive Summary](#executive-summary)
- [Map of Contents](#map-of-contents)
- [Current Ports and Adapters](#current-ports-and-adapters)
- [Open Questions](#open-questions)

## Executive Summary

The cache subsystem is responsible for materialising query-ready snapshots of the journal graph. In the hexagonal architecture it is split into:

- **Domain logic** (`core/src/domain/cache/**`) that understands cache metadata, edge indexing, and query semantics.
- **Application services** (`core/src/app/cache/**`) that coordinate rebuilds and orchestrate ports.
- **Ports/adapters** that surface external dependencies (libgit2 repository access, filesystem staging, logging, metrics) and make the subsystem test-double friendly.

This separation keeps the observable behaviour of cache builds intact while allowing us to ratchet clang-tidy warnings to zero for the files we touch.

## Map of Contents

- [Cache Rebuild Service](rebuild_service.md) — orchestration layer that owns rebuild lifecycles, port interactions, and temp-directory hygiene.
- [Edge Map Domain](edge_map.md) — deterministic hash table used to group edges and write sharded bitmaps.
- [Legacy Builder Adapter](builder.md) — thin wrapper preserved for public API compatibility; delegates to the service.

## Current Ports and Adapters

| Port | Role | Implementation |
| --- | --- | --- |
| `gm_cmd_cache_build_port_vtbl` | Drives cache rebuilds | `core/src/app/cache/cache_rebuild_service.c` (service) + CLI composition |
| `gm_git_repository_port` | Libgit2 repository access | `core/src/adapters/libgit2/repository_adapter.c` |
| `gm_fs_port` | Filesystem staging | `core/src/adapters/fs/posix_filesystem_adapter.c` |
| `gm_logger_port` | Structured logging | `core/src/adapters/logging/stdio_logger_adapter.c` |
| `gm_metrics_port` | Build metrics | `core/src/adapters/metrics/null_metrics_adapter.c` (placeholder) |

## Open Questions

- Should we introduce a dedicated filesystem port for temporary-directory creation to avoid direct `mkdir/stat` usage inside the service?
- When the cache migration reaches querying code, do we fold query execution into the same app layer or expose a separate inbound port?
