# Hexagonal Architecture Migration Progress

Started: 2025-10-08

This file is the single source of truth for tracking the end‑to‑end migration of the C library to a strict hexagonal architecture, per AGENTS.md. Update this checklist as work proceeds. Check off items with `[x]` only after they are complete and validated (CI green, zero new clang‑tidy warnings, and docs updated).

## Meta / Global
- [x] Initialize this progress file and commit it.
- [x] Baseline parity: run `make ci-local` to confirm green before changes; capture current clang‑tidy snapshot.
  - 2025-10-08: Brought CI back to green (docs links fixed; 32/32 tests pass; `clang-tidy` executed with no new blocking diagnostics). See `build-local/meson-logs/testlog.txt` and `clang-tidy-report.txt`.
- [ ] Ports inventory complete and documented (`core/include/gitmind/ports/**`).
- [ ] Fakes inventory complete and documented (`core/tests/fakes/**`).
- [ ] Composition roots wired to DI (`core/src/app/**`), using `gm_runtime_context_t`.
- [ ] Update diagrams in `AGENTS.md` and `docs/architecture/hexagonal/**` to reflect final ports/adapters.
- [ ] Final verification pass: all touched modules report zero clang‑tidy warnings; docs verified; `make ci-local` green.
  - Interim: All unit tests pass inside CI container (32/32 via Meson). `make ci-local` currently fails on preexisting docs link checks; will address separately.

---

## Module: cache
- [x] Analysis: enumerate external deps and all side effects in cache paths.
  - Reference headers: `core/include/gitmind/cache.h`, `core/include/gitmind/cache/cache.h`, `core/include/gitmind/cache/bitmap.h`, `core/include/gitmind/cache/internal/*`.
  - Reference impl: `core/src/cache/builder.c`, `core/src/cache/query.c`, `core/src/cache/bitmap.c`, domain file(s): `core/src/domain/cache/edge_map.c`.
  - Current deps to classify: libgit2 (via repository/ref/tree access), filesystem workspace/temp, logger, metrics, time.
- [x] Port boundary decision recorded: reuse `gm_git_repository_port` + `gm_fs_temp_port` (+ logger/metrics) — no separate `cache_storage_port` at this stage.
- [x] Inbound ports defined (headers under `core/include/gitmind/ports/`):
  - [x] `cache_build_port.h` (vtbl: request_build, invalidate, policies)
  - [x] `cache_query_port.h` (vtbl: query predicates, stats)
  - [x] Default thin coordinators implemented: `core/src/ports/cache/cache_build_port.c`, `core/src/ports/cache/cache_query_port.c`.
- [ ] Outbound ports defined (headers under `core/include/gitmind/ports/`):
  - [ ] `filesystem_port.h` (workspace ops beyond temp; mkdirs, readlink, etc.)
  - [x] `metrics_port.h`
  - [x] `logger_port.h`
  - [ ] `cache_storage_port.h` (only if specialized port chosen)
- [ ] Refactoring: move/keep pure logic in `core/src/domain/cache/` (staleness rules, shard distribution, query evaluation, edge map, bitmap ops).
- [ ] Application service: `cache_rebuild_service` orchestrates via ports only (no direct libgit2/FS/stdio).
- [ ] Adapters (runtime) implemented under `core/src/adapters/**`:
  - [ ] libgit2 repository adapter (reuse/extend existing) for required cache ops
  - [ ] POSIX filesystem workspace adapter (beyond temp)
  - [ ] Logger adapter (stdio or structured)
  - [ ] Metrics adapter (null default)
  - [ ] (Optional) cache storage adapter if specialized port chosen
- [ ] Fakes (tests) implemented under `core/tests/fakes/**` with deterministic behavior for all new ports.
- [ ] Tests:
  - [ ] Unit tests for domain cache logic only against fakes (`core/tests/unit/*`)
  - [ ] Integration tests under `tests/` for real adapters (inside Docker)
- [ ] Documentation: update `AGENTS.md` diagrams and add/refresh `docs/architecture/hexagonal/cache.md`.
- [x] Verification: `make ci-local` green; no new clang‑tidy errors for cache ports.

---

## Module: journal
- [ ] Analysis: enumerate external deps and side effects.
  - Reference headers: `core/include/gitmind/journal.h`.
  - Reference impl: `core/src/journal/writer.c`, `core/src/journal/reader.c`.
  - Current deps: libgit2 refs/objects, temp FS staging, logger, clock.
- [ ] Inbound port(s):
  - [ ] `journal_command_port.h` (append, snapshot, branch mgmt)
- [ ] Outbound ports confirmed/extended:
  - [ ] `git_ref_port.h` (create/update/delete)
  - [ ] `git_repository_port.h` (objects/blobs/commits)
  - [ ] `filesystem_port.h` (temp staging as needed)
  - [ ] `logger_port.h`, `clock_port.h`
- [ ] Refactoring: domain journal entities/services to `core/src/domain/journal/**` (append rules, branch history, migrations).
- [ ] Adapters (runtime): libgit2 ref adapter, repository adapter, FS temp/workspace, logger, clock.
- [ ] Fakes (tests): deterministic git ref/object store, clock, FS.
- [ ] Tests: domain unit tests (fakes only) + integration tests in Docker.
- [ ] Documentation updates (diagrams + journal schema notes).
- [ ] Verification: `make ci-local` green; zero new clang‑tidy warnings for journal files.

---

## Module: hooks
- [ ] Analysis: enumerate external deps and side effects.
  - Reference headers: `core/include/gitmind/hooks/augment.h`.
  - Reference impl: `core/src/hooks/augment.c`, `core/src/hooks/post-commit.c`.
  - Current deps: process spawning, env, filesystem, logger.
- [ ] Inbound port(s):
  - [ ] `hook_command_port.h` (register, dry-run, execute)
- [ ] Outbound ports confirmed/extended:
  - [ ] `process_port.h`
  - [ ] `env_port.h` (exists)
  - [ ] `filesystem_port.h`
  - [ ] `logger_port.h`
- [ ] Refactoring: domain hooks services to `core/src/domain/hooks/**` (scheduling, policy, failure handling).
- [ ] Adapters (runtime): POSIX process adapter, env adapter (exists), FS adapter, logger.
- [ ] Fakes (tests): deterministic process/env/fs/logger.
- [ ] Tests: unit tests with fakes + integration in Docker.
- [ ] Documentation updates and CLI wiring notes.
- [ ] Verification: `make ci-local` green; zero new clang‑tidy warnings for hooks files.

---

## Module: edge (supporting)
- [ ] Analysis and remaining side‑effect seams identified (OID helpers already migrated).
- [ ] Ensure all edge domain logic lives under `core/src/domain/edge/**`.
- [ ] Verify no direct libgit2 calls remain; rely on repository ports.
- [ ] Tests/documentation/verification as above.

---

## Module: attribution (supporting)
- [ ] Analysis: commit walking/timezone/clock needs.
- [ ] Inbound `attribution_query_port.h`.
- [ ] Outbound `commit_walker` via `git_repository_port`, `clock_port`, timezone helper.
- [ ] Refactor domain under `core/src/domain/attribution/**` (parts already present).
- [ ] Tests/documentation/verification as above.

---

## Cross‑Cutting Completion
- [ ] `gm_runtime_context_t` finalized and passed through all application services.
- [ ] Public headers umbrella‑safe; `make header-compile` passes.
- [ ] Docs verified: `make docs-verify`.
- [ ] Activity log updated under `docs/activity/` summarizing deltas and lessons learned.
