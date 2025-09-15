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

> [!WARNING]
> **DO NOT BUILD GITMIND OUTSIDE OF DOCKER**
> git-mind manipulates Git internals (refs/*, objects, config). Building or running tests on the host can corrupt this repository or others on your machine. All builds and tests must run inside the CI Docker image for safety and parity.

> [!INFO]
> _If you really want to..._
> Use the provided container workflow:
> - `make ci-local` — runs docs checks, builds, and unit tests in the CI image
> - `tools/ci/ci_local.sh` — same as above, invoked directly
>
> Advanced (at your own risk):
> - `meson setup build -Dforce_local_builds=true` — explicit Meson override
> - `GITMIND_ALLOW_HOST_BUILD=1` — legacy env override (discouraged)
> If you override, you accept responsibility for any repo damage.

- Configure + build (inside CI container): `make ci-local` or `tools/ci/ci_local.sh`
- Run unit tests (inside CI container): done as part of `make ci-local`
- Make shims: `make`, `make test`, `make clean`
- Lint (CI parity): `./tools/docker-clang-tidy.sh` → produces `clang-tidy-report.txt`
- Strict multi-compiler build: `./tools/gauntlet/run-gauntlet.sh` (recommended before PRs)
- Enable hooks: `pre-commit install` (clang-format + detect-secrets + docs link/TOC checks)
- Docs linter (all docs): `python3 tools/docs/check_docs.py --mode link` and `--mode toc`
- Pre-commit (changed files): `pre-commit run --all-files docs-link-check` and `docs-toc-check`
 - Markdown linter: `make md-lint` (check) and `make md-fix` (autofix) — uses `.markdownlint.jsonc`
 - Header guards lint: `meson run -C build lint_header_guards`
 - Public headers compile check: `ninja -C build header-compile`

## Coding Style & Naming Conventions

- Language: C23 with warnings-as-errors; no VLAs or shadowing; explicit prototypes.
- Formatting: `.clang-format` (LLVM-based, 4 spaces, 80 cols, pointer alignment right). Pre-commit runs `clang-format`.
- Naming: functions/vars `lower_snake_case` (prefix `gm_`), macros `UPPER_SNAKE`, types end in `_t`, header guards `GITMIND_*`.
- Includes: prefer specific headers; order/regroup per `.clang-format`.
 - Public headers must be umbrella-safe: compile standalone, correct include order, stable header guards, and `extern "C"` when included from C++.

## Testing Guidelines

- Unit tests live in `core/tests/unit/` as `test_<module>.c`. Keep deterministic and isolated.
- Meson wires test backends as needed—just build and run tests via Meson/Ninja.
- E2E/integration under `tests/` for CLI/flow checks. Keep fixtures minimal.

## Commit & Pull Request Guidelines

- Conventional commits: `type(scope): description` (e.g., `fix(core/cbor): handle null keys`). Reference issues (`Fixes #123`).
- PRs must describe changes, link issues, include a short test plan, pass CI, and introduce no new clang-tidy warnings (`./tools/docker-clang-tidy.sh`). Update docs when applicable.
 - Code review (CodeRabbit): Keep `.coderabbit.yml` updated. Prefer summary reviews, limit per-line comments on docs. If rejecting a suggestion, add a note under `docs/code-reviews/rejected-suggestions/{commit}_{branch}_{PR#}_{suggestion}.md` with a link to the original comment and rationale.

## Security & Configuration

- Do not commit secrets. Pre-commit runs `detect-secrets` with `.secrets.baseline`.
- Use `.env.example` as a template; never check in real credentials.
 - E2E safety: some CI E2E steps require `GITMIND_SAFETY=off` in the container environment.

## Agent-Specific Notes

- Make minimal, focused diffs; avoid drive-by refactors. Match existing patterns in `core/` and headers under `include/`.
- Run build, tests, and lint locally before proposing changes. Keep changes zero-warnings and formatted.
- Prefer OID-first APIs for SHA-agnostic correctness. Use `git_oid` (typedef `gm_oid_t`) in new core interfaces, and compare via `git_oid_cmp`.

### One-Thing Rule (Touched code policy)

- You touch it? You refactor it. If you modify a type or function that clearly bundles multiple concerns in a single file, split out the thing you touched into its own file (1 file = 1 thing):
  - One enum per header.
  - One struct per header (public structs under `include/gitmind/`, internal-only in `core/include/gitmind/<module>/internal/`).
  - One responsibility per C file where practical.
- Scope: Only the item you touched (don’t boil the ocean). If you edit `Bar` in `foo.c` and `foo.c` contains 12 types, you only have to extract `Bar` now.
- Enforcement: Pre-commit includes a heuristic check (`one-thing`); bypass in emergencies with `GM_ONE_THING_ALLOW=1` in the environment. PRs should include a brief note when bypassed.

## Working Knowledge (for agents)

- Journal-first graph: Edges are CBOR commits under `refs/gitmind/edges/<branch>`; cache lives in `refs/gitmind/cache/<branch>` and is rebuildable (never merged).
- Semantics as names: Store `type_name` and `lane_name` as UTF-8 strings on edges; do not collapse into a generic/custom type. Derive 64-bit IDs from names (NFC + stable hash) only for cache/filters.
- Time-travel correctness: All semantics (and optional advice) are in-history; queries evaluate against the chosen commit and branch.
- Merge/conflict model: Append-only journal; edge ULIDs form an OR-Set; “Semantics Advice” (optional) merges with hybrid CRDT (LWW scalars, OR-Set collections).
- Link vs code authors: Edge attribution `author` is the link creator (from `git config` unless provided); code authorship at the chosen commit is recorded separately (per-file last commit author/time) when captured by external tooling.
- Docker hygiene: Images are namespaced/labeled (`gitmind/ci:clang-20`, `gitmind/gauntlet:<compiler>`; label `com.gitmind.project=git-mind`). Use `make docker-clean` to reclaim space safely.
- CI/Tidy nuance: Local builds and tests pass. Clang-tidy in Docker depends on CRoaring headers in the CI image; add a source build step for deterministic results on aarch64 if CI flags it.
 - CI path filters: Core build/test workflows are restricted to code paths; doc-only changes do not trigger core builds. Markdown lint runs separately with reduced noise via `.markdownlint.jsonc`.

## Agent Activity Log

2025-09-15
- CI/CD: Fixed C-Core Gate failures. README now complies with markdownlint (underscore emphasis/strong, blockquote spacing, no inline HTML wrappers, code fence fixes, heading punctuation). Added local CI rig `tools/ci/ci_local.sh` and `make ci-local` to mirror C-Core Gate (markdownlint + docs checks, Dockerized build/tests/E2E, clang-tidy pass).
- Build: Switched Meson `c_std` from `c23` to `c2x` (Meson-compatible while targeting C23). Moved `option()`s to `meson_options.txt`. Set `GITMIND_DOCKER=1` in gauntlet Dockerfile to satisfy the Meson host-build guard inside gating containers. Registered new unit test target `test_ref_utils` in `meson.build`.
- Core (refs): `gm_build_ref` now accepts Git-style branch shorthands (slashes allowed), validates the combined ref via libgit2 `git_reference_normalize_name`, and rejects leading `refs/` to prevent double-prefixing. Updated header docs and added unit tests.
- Core (path safety): Removed `GM_PATH_MAX * 2` buffer in `core/src/cache/tree_builder.c`; now uses `GM_PATH_MAX` and bounded `gm_snprintf`.
- Docs: Added `docs/architecture/Ref_Name_Validation.md` (with ToC) documenting ref-building policy and validation. Updated `docs/CI_STRATEGY.md` and `docs/code-review/CLAUDE.md` to clarify C23 policy expressed via Meson `c2x`. Expanded `docs/operations/Environment_Variables.md` with `GITMIND_CI_IMAGE` and `HOOKS_BYPASS`.
- Reviews: Documented rejected suggestions for PR #169 under `docs/code-reviews/rejected-suggestions/` (canonical CHANGELOG filename, keep `md-verify` alias, ignore `.PHONY` ordering churn). Linked decisions in the PR thread.
 - Core (OID-first): Advanced OID-first migration in cache/journal; replaced custom zero/equality checks with `git_oid_is_zero`/`git_oid_cmp` and standardized formatting via `git_oid_fmt`/`git_oid_tostr`. Stored binary tip OID alongside hex in cache meta for performance and porcelain separation.
 - Core (safe ops): Replaced unsafe libc string/memory calls in hot paths with safe wrappers (`gm_strcpy_safe`, `gm_snprintf`, `gm_memcpy_span`) to maintain warnings-as-errors and harden boundaries.
 - CBOR: Added `GITMIND_CBOR_DEBUG` flag for verbose decode tracing. Extended edge encoder/decoder to write/read OID fields while keeping legacy SHA fallback for compatibility.
 - Tests: Added unit tests `core/tests/unit/test_edge_oid_fallback.c` and `core/tests/unit/test_cache_shard_distribution.c`; expanded `test_ref_utils.c`. Registered in Meson. Tracking a small set of failing tests to align equality and fallback semantics.
 - Policy: Began enforcing the “One-Thing” touched-code policy via a pre-commit heuristic (`tools/quality/check_one_thing.py`) wired into `.pre-commit-config.yaml`.
 - CI: Ensured `tools/ci/ci_local.sh` builds and runs unit tests entirely inside the CI Docker image; tuned header-compile include paths for container parity.

Learnings (2025-09-15)
- Equality semantics need OID-first strictness with explicit legacy fallback; tests should state intent to avoid ambiguity when SHA matches but OIDs differ.
- `git_reference_normalize_name` is reliable for ref validation; rejecting inputs beginning with `refs/` prevents double-prefixing errors when building namespaced refs.
- Running all builds/tests inside the CI Docker image reduces environment drift; using Meson `c2x` preserves C23 intent while satisfying toolchain expectations.
- Pre-commit enforcement of the One-Thing rule must stay conservative to minimize false positives on umbrella changes; scope it to files actually touched.
- CBOR compatibility requires writing new fields while maintaining robust reader fallbacks; do not rely on field ordering and prefer explicit keys.

2025-09-14
- CI/CD: Added Markdown linting with repo-aligned rules (`.markdownlint.jsonc`), `make md-lint`/`md-fix` targets, and path filters limiting core workflows to code paths. Enabled docker pull retry and set `GITMIND_SAFETY=off` for E2E in the core workflow.
- Header hygiene: Standardized header guards to `GITMIND_*`, added umbrella viability check (public headers compile standalone), and a header guard linter (`tools/quality/check_header_guards.py`) wired via Meson run target.
- OID-first migration: Introduced `typedef git_oid gm_oid_t` and `GM_OID_RAWSZ`. Migrated edges, cache queries, and hooks APIs to OID-first and switched equality to `git_oid_cmp`. Maintained compatibility where needed; CBOR decode backfills OIDs from legacy fields temporarily.
- Journal safety: Base64-encoded CBOR commit messages on write and decoded on read; added strict `gm_snprintf` overflow checks and security-conscious includes.
- CLI fixes: Adjusted `apps/cli` commands for include hygiene, error handling, and consistent porcelain output; freed libgit2 arrays correctly.
- Reviews/PRs: Pushed branches, resolved merge conflicts, and responded to CodeRabbit with summary feedback. Opened a PR for CodeRabbit + markdownlint config changes and updated PRs addressing invalid path errors and core cleanup.

## Next Steps (handoff checklist)
- Complete on-disk cache migration to OID-only storage and naming; ensure rebuild and fallback readers handle both formats or gate with a one-time migration.
- Extend journal CBOR schema to store OIDs explicitly (not only derivable from legacy fields); update reader/writer and consumers.
- Sweep any remaining headers for guard/extern "C" issues and fix items flagged by `lint_header_guards`.
- Expand attributed edge CBOR to include OIDs alongside legacy fields; update cache/journal consumers accordingly.
- Stabilize and merge open PRs (#164, #165, #166, #167) after CI green; incorporate CodeRabbit actionable feedback and document any rejections under `docs/code-reviews/rejected-suggestions/`.
- Add focused tests: journal base64 encode/decode roundtrip, `gm_snprintf` truncation behavior, and OID-based equality/lookup paths in cache and hooks.

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
