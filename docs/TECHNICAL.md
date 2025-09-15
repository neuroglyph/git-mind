# Technical Overview

This page orients engineers to the core implementation and points to deeper docs.

## Core Architecture

- Journal‑first storage: semantic edges are CBOR commits under `refs/gitmind/edges/<branch>`.
- Rebuildable cache: high‑performance Roaring Bitmap cache under `refs/gitmind/cache/<branch>`.
- OID‑first APIs: new interfaces use `gm_oid_t` (`git_oid`) and compare via `git_oid_cmp`.
- Append‑only semantics: edges form an OR‑Set via ULIDs; merges resolve predictably.

See `docs/architecture/` for full write‑ups:

- Journal Architecture Pivot: `docs/architecture/journal-architecture-pivot.md`
- Bitmap Cache Design: `docs/architecture/bitmap-cache-design.md`
- Attribution System: `docs/architecture/attribution-system.md`

## Codebase Layout

- `core/` — C23 library (public headers under `core/include/`, impl under `core/src/`, unit tests under `core/tests/`).
- `include/` — Umbrella API `gitmind.h` and public namespace headers.
- `apps/` — CLI and hooks.
- `tests/` — E2E/integration.

Style: C23, warnings‑as‑errors, `gm_` prefixes, `_t` types, `GITMIND_*` header guards. Includes must be umbrella‑safe and C++ compatible (`extern "C"`).

## Build, Test, Lint

```bash
meson setup build && ninja -C build
ninja -C build test
./tools/docker-clang-tidy.sh
```

Header compile check: `ninja -C build header-compile`  
Header guards lint: `meson run -C build lint_header_guards`

## Safety and Environment

- Safety guard prevents running in the official repo unless explicitly disabled (`GITMIND_SAFETY=off`).
- Host builds can be gated; set `GITMIND_ALLOW_HOST_BUILD=1` if needed.
- Full list: `docs/operations/Environment_Variables.md`.

## Next Engineering Tasks

- Complete OID‑only on‑disk cache migration and readers.
- Extend CBOR schema to store OIDs explicitly.
- Add focused tests: CBOR base64 round‑trip, `gm_snprintf` truncation, OID equality paths.

