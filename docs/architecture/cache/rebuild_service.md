---
title: Cache Rebuild Service
description: Application-level orchestration for cache rebuilds.
audience: [contributors]
status: draft
domain: cache
tags: [cache, service]
---

# Cache Rebuild Service

## Table of Contents

- [Responsibility](#responsibility)
- [Inputs and Outputs](#inputs-and-outputs)
- [Collaborators](#collaborators)
- [Flow](#flow)
- [Testing Strategy](#testing-strategy)
- [Future Work](#future-work)

## Responsibility

`core/src/app/cache/cache_rebuild_service.c` owns the rebuild lifecycle. It:

- Validates branch input (length, existing cache metadata).
- Collects journal edges into forward/reverse maps via injected ports.
- Writes bitmap shards to a temporary workspace and builds the git tree.
- Creates the cache commit and updates the cache reference.
- Cleans up temporary directories whether rebuild succeeds or fails.

Because `gm_cache_rebuild` now delegates straight to this service, the legacy builder shim is free of orchestration concerns.

## Inputs and Outputs

- **Inputs**: `gm_context_t *ctx`, branch name, `force_full` flag.
  - Context must already carry concrete implementations for repository, filesystem, logging, metrics, and environment ports.
- **Outputs**: `gm_result` via the adapter (integer return code). The service returns `GM_OK`, `GM_ERR_INVALID_ARGUMENT`, `GM_ERR_IO_FAILED`, or `GM_ERR_UNKNOWN` depending on failures.

## Collaborators

- `gm_edge_map` domain (`core/src/domain/cache/edge_map.c`) for deterministic edge indexing.
- `gm_journal_read` to iterate journal entries.
- `gm_bitmap_write_file` for sharded bitmap persistence.
- libgit2 calls (tree builders, commits, refs) wrapped inside the service.
- Ports provided via `gm_context_t`: repository access, filesystem staging, logger, metrics.

## Flow

1. **Preparation**: load existing cache metadata when `force_full` is false; create a temp directory with collision avoidance.
2. **Edge collection**: build forward/reverse edge maps and stream journal entries through `gm_edge_map_add`.
3. **Persistence**: write bitmap shards (`forward` and `reverse`) to the temp workspace.
4. **Tree & commit**: call `gm_build_tree_from_directory`, create a commit buffer, write the commit object, and update cache metadata (`gm_cache_meta_t`).
5. **Reference update**: point `refs/gitmind/cache/<branch>` at the new commit.
6. **Cleanup**: destroy edge maps and remove the temp directory regardless of outcome.

## Testing Strategy

- Unit coverage for `gm_edge_map` (`core/tests/unit/test_cache_edge_map.c`) verifies deterministic hashing and visitor semantics.
- `test_cache_branch_limits` ensures branch length validation still holds.
- Future work should add direct service-level tests using fakes for the filesystem and repository ports once those ports are extracted explicitly.

## Future Work

- Extract a dedicated filesystem port for temp directory creation/removal.
- Replace direct libgit2 calls with outbound ports so the service can be tested without touching the real repository.
- Surface metrics/logging injection points via `gm_logger_port` and `gm_metrics_port` once the logging refactor lands.
