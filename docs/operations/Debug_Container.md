---
title: Debug Container Workflow
description: Stage-and-debug workflow with the CI Docker image and ASAN helpers.
audience: [developers]
domain: [operations]
tags: [debugging, docker, tooling]
status: draft
last_updated: 2025-10-09
---

# Debug Container Workflow

This guide codifies the staged debug loop we have been hand-running for cache and journal crashes. It keeps destructive binaries away from the working tree, wires in the CI Docker image, and applies the Memory Allocation Strategies ownership playbook.

## Why stage first?

- `git-mind` manipulates Git internals; running integration tests in-place risks corrupting the repo.
- Staging lets us remove `origin` automatically, so the debug copy cannot be pushed by accident.
- We can toggle sanitizers, gdb, or lldb inside the container without polluting the primary toolchain.

## Script quickstart

```
./tools/debug/debug-container.sh --packages "gdb valgrind" -- --login
```

The script performs the following:

- Syncs the repository into a temp directory (`/tmp/gitmind-debug-XXXXXX` by default), respecting `.gitignore` while skipping build artifacts.
- Drops the `origin` remote inside the copy.
- Launches `gitmind/ci:clang-20` with the staged repo mounted at `/workspace`.
- Installs extra apt packages when `--packages` is supplied (runs as root only when needed).

### Key flags

- `--stage <dir>` reuses an existing staging checkout (set `--resync` to refresh).
- `--keep-stage` retains the staged tree after exiting.
- `--packages "gdb valgrind"` installs additional tooling before running the requested command.
- Arguments after `--` execute inside the container; omit them to land in an interactive shell.

### Examples

Run the cache query test under ASAN:

```
./tools/debug/debug-container.sh --packages "gdb" -- \
  "set -euo pipefail; CC=clang meson setup build-asan -Db_sanitize=address -Db_lundef=false; \
   ninja -C build-asan; ASAN_OPTIONS=detect_leaks=0 ./build-asan/test_cache_query"
```

Attach gdb with a prepped stage:

```
STAGE=/tmp/gitmind-debug-sanity
./tools/debug/debug-container.sh --stage "$STAGE" --resync
./tools/debug/debug-container.sh --stage "$STAGE" --packages gdb -- \
  "cd build-asan && gdb -q ./test_cache_query"
```

## Applying the Memory Allocation playbook

- Treat every pointer out of `gm_fs_temp_port_canonicalize_ex` as a `_view`. Clients **must not** `free()` the returned pointer—copy it or wrap it in `_owned` helpers before the next port call.
- Adopt suffixes in variable names when touching lifetime-sensitive code (for example, `canon_view`, `repo_canon_view`).
- Prefer `_borrowed` or `_view` notes when returning adapters’ internal buffers to make intent obvious during reviews.

## Neo4j doc indexing

After adjusting docs or code, mirror the mapping in the shared Neo4j catalog so contributors know which docs move with which modules:

```
cd /Users/james/git/agent-collab
./bin/neo4j_link.sh \
  --doc ../git-mind/docs/operations/Debug_Container.md \
  --code ../git-mind/tools/debug/debug-container.sh \
  --code ../git-mind/core/src/journal/writer.c \
  --code ../git-mind/core/include/gitmind/ports/fs_temp_port.h
```

(Adjust the helper arguments once we finish scripting full automation, but keep this pairing up to date whenever cache or journal ownership rules move.)

## Notes and hygiene

- Sanitizers: run `meson setup build-asan -Db_sanitize=address -Db_lundef=false` when you need ASAN/UBSAN context. The stage script leaves your compiler cache intact between runs.
- Keep stage copies short-lived unless you pass `--keep-stage`. The cleanup hook defaults to wiping mktemp directories so we do not accumulate `/tmp/gitmind-ci-*` clutter.
- Remember to sync telemetry/logging docs when adapters gain new disposers or formatters, as the CLI README now documents those surfaces.
