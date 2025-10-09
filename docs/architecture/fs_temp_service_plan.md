---
title: Filesystem & Temp Path Service Plan
description: Strategy for introducing a platform-aware filesystem/temp-path port.
audience: [contributors]
status: draft
domain: infrastructure
tags: [filesystem, temp, ports]
---

## Table of Contents

- [Motivation](#motivation)
- [Deliverables](#deliverables)
- [Implementation Steps](#implementation-steps)
- [Open Questions](#open-questions)
- [Timeline](#timeline)

# Filesystem & Temp Path Service Plan

## Motivation

- Current tests and cache code drop temporary directories (e.g. `.gm_cache_branch_tmp`) in the working directory or `$HOME`, which is noisy and non-portable.
- Hexagonal architecture expects filesystem access to flow through ports/adapters for test doubles and platform flexibility.
- We want a deterministic location schema: `~/.gitmind/<repo-identifier>/<filename>` across Linux, macOS, and Windows.

## Deliverables

1. **Port Definition** (`core/include/gitmind/ports/filesystem_port.h`): add methods for
   - `make_temp_dir(repo_id, component)` → returns the fully-qualified path inside `~/.gitmind/<repo>/component-XXXXXX`.
   - `remove_tree(path)` → recursive delete used by cache rebuild service and tests.
   - Potential helpers (`path_join`, `ensure_directory`).
   - `canonicalize_ex(path, opts)` → mode-aware normalization resolving either logical strings or physical filesystem paths (see notes below).
2. **Adapters**
   - Production adapter under `core/src/adapters/fs/posix_temp_adapter.c` implementing the port using platform checks (POSIX APIs now; Windows stub to follow).
   - Test fake under `core/tests/fakes/fs/temp_fs_fake.c` mirroring behaviour in-memory.
3. **Context wiring**
   - Extend `gm_context_t` with a `filesystem_port` slot.
   - Update composition roots to supply the new adapter.
4. **Consumers migration**
   - Cache rebuild service switches to the port for temp directory creation/removal.
   - `test_cache_branch_limits` (and similar tests) request paths through the port rather than writing into the repo.

## Implementation Steps

1. Draft the port header with `gm_result_*` functions and document expected path schema.
2. Implement the POSIX adapter (using `getenv("HOME")` + repo identifier derived from `.git/config` URL or absolute path hash).
3. Create a repo identifier helper so the port knows where to place directories.
4. Update `gm_runtime_context_t` and constructors to carry the port.
5. Refactor cache service + tests to call the port.
6. Add unit tests for the adapter (under `core/tests/unit/test_fs_temp_service.c`).
7. Update docs once the implementation lands.

### Canonicalization Modes

- `GM_FS_CANON_LOGICAL` (default): collapses `//`, `/./`, and safe `/../` segments without touching the filesystem. Works for non-existent paths and keeps symlink segments intact.
- `GM_FS_CANON_PHYSICAL_EXISTING`: resolves symlinks and requires the path to exist (implemented via `realpath` on POSIX). Returns `GM_ERR_NOT_FOUND` for missing inputs.
- `GM_FS_CANON_PHYSICAL_CREATE_OK`: resolves the parent directory physically (must exist) and then appends the final segment logically so callers can prepare paths they intend to create.
- Adapters must implement all three modes; fakes should mirror the contract so unit tests can validate both success and error paths consistently.
- Canonicalize returns a `_view` owned by the adapter; call sites **must** copy or promote to `_owned` before the next fs-temp call and must never `free()` the pointer.

## Open Questions

- How do we derive a stable repo identifier? Options include absolute path hashing, git remote URL, or `git_repository_path` canonicalisation.
- Should temp directories include a random suffix per call to avoid collisions? (Likely yes.)
- Windows implementation: we need a separate adapter using `SHGetKnownFolderPath` or `%LOCALAPPDATA%`.

## Timeline

- Prototype port/adapters and migrate cache service/tests in the next refactor cycle.
- Wire additional consumers (hooks, journal tests) as we touch them.
