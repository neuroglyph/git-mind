# Repository Guidelines

## Project Structure & Module Organization

- `core/` — C23 library: `include/` (public headers), `src/` (impl), `tests/` (unit). Add new code here.
- `include/` — Umbrella API (`gitmind.h`) and namespaced headers under `include/gitmind/`.
- `src/` — Legacy code under migration. Avoid adding new modules here.
- `apps/` — Future CLI/hooks/apps. Until announced, prefer `core/` additions.
- `tests/` — E2E/integration/legacy tests and fixtures.
- `tools/`, `quality/`, `.githooks/` — Dev tooling (clang-tidy, formatting, gauntlet, secret scanning).
- `docs/` — Architecture, dev setup, and operational notes.

## Build, Test, and Development Commands

- Configure + build: `meson setup build && ninja -C build`
- Run unit tests: `ninja -C build test` (or `meson test -C build`)
- Make shims: `make`, `make test`, `make clean`
- Lint (CI parity): `./tools/docker-clang-tidy.sh` → produces `clang-tidy-report.txt`
- Strict multi-compiler build: `./tools/gauntlet/run-gauntlet.sh` (recommended before PRs)
- Enable hooks: `pre-commit install` (clang-format + detect-secrets + docs link/TOC checks)
- Docs linter (all docs): `python3 tools/docs/check_docs.py --mode link` and `--mode toc`
- Pre-commit (changed files): `pre-commit run --all-files docs-link-check` and `docs-toc-check`

## Coding Style & Naming Conventions

- Language: C23 with warnings-as-errors; no VLAs or shadowing; explicit prototypes.
- Formatting: `.clang-format` (LLVM-based, 4 spaces, 80 cols, pointer alignment right). Pre-commit runs `clang-format`.
- Naming: functions/vars `lower_snake_case` (prefix `gm_`), macros `UPPER_SNAKE`, types end in `_t`, header guards `GITMIND_*`.
- Includes: prefer specific headers; order/regroup per `.clang-format`.

## Testing Guidelines

- Unit tests live in `core/tests/unit/` as `test_<module>.c`. Keep deterministic and isolated.
- Meson wires test backends as needed—just build and run tests via Meson/Ninja.
- E2E/integration under `tests/` for CLI/flow checks. Keep fixtures minimal.

## Commit & Pull Request Guidelines

- Conventional commits: `type(scope): description` (e.g., `fix(core/cbor): handle null keys`). Reference issues (`Fixes #123`).
- PRs must describe changes, link issues, include a short test plan, pass CI, and introduce no new clang-tidy warnings (`./tools/docker-clang-tidy.sh`). Update docs when applicable.

## Security & Configuration

- Do not commit secrets. Pre-commit runs `detect-secrets` with `.secrets.baseline`.
- Use `.env.example` as a template; never check in real credentials.

## Agent-Specific Notes

- Make minimal, focused diffs; avoid drive-by refactors. Match existing patterns in `core/` and headers under `include/`.
- Run build, tests, and lint locally before proposing changes. Keep changes zero-warnings and formatted.

## Working Knowledge (for agents)

- Journal-first graph: Edges are CBOR commits under `refs/gitmind/edges/<branch>`; cache lives in `refs/gitmind/cache/<branch>` and is rebuildable (never merged).
- Semantics as names: Store `type_name` and `lane_name` as UTF-8 strings on edges; do not collapse into a generic/custom type. Derive 64-bit IDs from names (NFC + stable hash) only for cache/filters.
- Time-travel correctness: All semantics (and optional advice) are in-history; queries evaluate against the chosen commit and branch.
- Merge/conflict model: Append-only journal; edge ULIDs form an OR-Set; “Semantics Advice” (optional) merges with hybrid CRDT (LWW scalars, OR-Set collections).
- Link vs code authors: Edge attribution `author` is the link creator (from `git config` unless provided); code authorship at the chosen commit is recorded separately (per-file last commit author/time) when captured by external tooling.
- Docker hygiene: Images are namespaced/labeled (`gitmind/ci:clang-20`, `gitmind/gauntlet:<compiler>`; label `com.gitmind.project=git-mind`). Use `make docker-clean` to reclaim space safely.
- CI/Tidy nuance: Local builds and tests pass. Clang-tidy in Docker depends on CRoaring headers in the CI image; add a source build step for deterministic results on aarch64 if CI flags it.

## Vision Snapshot

- Version your thoughts: graph of code/docs/notes tracked in Git.
- Serverless distributed graph DB: repos are the database; clone/branch/merge.
- Human + AI co‑thought: shared memory with attribution and review lanes.
- Names‑as‑truth semantics; edges‑as‑commits; optional advice merging via CRDT.
- MCP service (optional, local‑only) for tools to read/write edges.

## External Tracking (Local Experiments)

If you maintain a separate experimental tracking system locally (e.g., a personal graph database or notes), keep it entirely outside this repository and CI. Do not include configuration or scripts here.

## Agent Activity Log

- 2025-09-13
  - fix(include): guard time conversion macros in `include/gitmind/constants_internal.h`.
  - fix(output): add missing EOF newlines in `include/gitmind/output.h` and `apps/cli/output.c`.
  - feat(cli): introduce `gm_cli_ctx_t`; refactor `apps/cli/main.c` and commands to pass explicit CLI output context (removed global output).
  - chore(cli): normalize includes; prefer specific public headers; added `inc_cli` include dirs in `meson.build` for CLI builds.
  - feat(cli/safety): implement libgit2-based safety guard with strict official-repo match; add `GITMIND_SAFETY` override; extracted URL helper in `include/gitmind/safety.h`.
  - test(core): add `core/tests/unit/test_safety.c` and register in Meson.
  - ci(e2e): run E2E inside CI Docker image in `.github/workflows/c_core.yml`; improved `tests/e2e/run_all_tests.sh` to honor `GIT_MIND` env.
  - fix(core/journal): add temporary CBOR decode wrappers in `core/src/journal/reader.c` (`gm_edge_decode_cbor_ex`, `gm_edge_attributed_decode_cbor_ex`) to unblock CLI until attributed decoder lands.
  - fix(core/journal/tidy): replace `memset/strncpy/snprintf` in `reader.c` with safe wrappers (`GM_MEMSET_SAFE`, `gm_memcpy_safe`, `gm_snprintf`); include security headers. Updated `tools/docker-clang-tidy.sh` to set `GITMIND_DOCKER=1` for Meson.
  - docs(ops): added `docs/operations/Environment_Variables.md`; linked from `README.md` and `docs/README.md`. Noted runtime safety behavior in Test Plan.
  - style(include): use angle includes in `include/gitmind.h` and trim optional-include block comment.
  - Opened PR: <https://github.com/neuroglyph/git-mind/pull/166> (branch `feat/cli-safety-e2e`). Built and ran unit tests in Docker (15/15 passing). E2E wired to run in CI container.
