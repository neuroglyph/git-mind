# Repository Guidelines

## Agent TL;DR (read me first)

- Build/test: `make ci-local` (Docker-only; Meson guard prevents host builds). Docs verify: `make docs-verify`.
- Review seeding: `make seed-review PR=<number>`; see `docs/tools/Review_Seeding.md`.
- Red lines: OID-first equality; use `gm_snprintf/gm_strcpy_safe`; append-only journal; no public ABI breaks (append fields).
- Refs via `gm_build_ref`; reject inputs starting with `refs/`. No `GM_PATH_MAX*2` buffers.
- Public headers are umbrella-safe with C++ linkage guards; run `make header-compile`.
- Docker-only policy: override host build only with `-Dforce_local_builds=true` when absolutely necessary.

```yaml
agent:
  docker_only: true
  build: "make ci-local"
  docs_verify: "make docs-verify"
  seed_review: "make seed-review PR=${PR}"
  header_compile: "make header-compile"
  stop_signs:
    - behavior_or_abi_change
    - adding_dependencies
    - changing_ci_gates
    - large_refactor
```

## Boot Checklist

- Check for nested `AGENTS.md` when editing in subdirs
- Use Docker for builds/tests; never host-run by default
- Run `make docs-verify` for doc changes; `make header-compile` for new public headers
- Respect One‑Thing rule for touched files
- For fork PRs, seeded worksheet may be artifact+comment (see Review_Seeding)
- Sync feature/milestone updates in `docs/features/Features_Ledger.md`; follow the Capture→GA lifecycle and rerun `make features-update`.

## Recent Developments

```yaml
recent_developments:
  - date: 2025-09-16
    summary: >
      Review seeding + auto‑replies: added seed script, auto‑seed workflow (same‑repo commit, fork artifact fallback), and apply‑feedback workflow that replies to PR comments from the worksheet. Renamed bot secret to GITMIND_PR_WORKSHEET_BOT.
  - date: 2025-09-16
    summary: >
      Worksheet gates: pre‑push guard + checker to block unfilled templates/undecided sections in review worksheets; bypass knob documented.
  - date: 2025-09-16
    summary: >
      Curated AGENTS.md: TL;DR + machine‑readable agent block + Boot Checklist; added Recent Developments YAML and auto‑archival to docs/activity via pre‑commit hook.
  - date: 2025-09-16
    summary: >
      Features Ledger: introduced docs/features/Features_Ledger.md + hubless/update_progress.py (KLoC‑weighted groups); added README features-progress block; wired pre-commit and a main-branch auto-update workflow; make features-update.
  - date: 2025-09-16
    summary: >
      Review ergonomics: CodeRabbit config (.coderabbit.yml) for summary‑first; PR template guidance; docs‑only auto‑labeling workflow.
  - date: 2025-09-15
    summary: >
      CI/CD: Fixed C-Core Gate failures. README now complies with markdownlint (underscore emphasis/strong, blockquote spacing, no inline HTML wrappers, code fence fixes, heading punctuation). Added local CI rig `tools/ci/ci_local.sh` and `make ci-local` to mirror C-Core Gate (markdownlint + docs checks, Dockerized build/tests/E2E, clang-tidy pass).
  - date: 2025-09-15
    summary: >
      Build: Switched Meson `c_std` from `c23` to `c2x` (Meson-compatible while targeting C23). Moved `option()`s to `meson_options.txt`. Set `GITMIND_DOCKER=1` in gauntlet Dockerfile to satisfy the Meson host-build guard inside gating containers. Registered new unit test target `test_ref_utils` in `meson.build`.
  - date: 2025-09-15
    summary: >
      Core (refs): `gm_build_ref` now accepts Git-style branch shorthands (slashes allowed), validates the combined ref via libgit2 `git_reference_normalize_name`, and rejects leading `refs/` to prevent double-prefixing. Updated header docs and added unit tests.
  - date: 2025-09-15
    summary: >
      Core (path safety): Removed `GM_PATH_MAX * 2` buffer in `core/src/cache/tree_builder.c`; now uses `GM_PATH_MAX` and bounded `gm_snprintf`.
  - date: 2025-09-15
    summary: >
      Docs: Added `docs/architecture/Ref_Name_Validation.md` (with ToC) documenting ref-building policy and validation. Updated `docs/CI_STRATEGY.md` and `docs/code-review/CLAUDE.md` to clarify C23 policy expressed via Meson `c2x`. Expanded `docs/operations/Environment_Variables.md` with `GITMIND_CI_IMAGE` and `HOOKS_BYPASS`.
  - date: 2025-09-15
    summary: >
      Reviews: Documented rejected suggestions for PR #169 under `docs/code-reviews/rejected-suggestions/` (canonical CHANGELOG filename, keep `md-verify` alias, ignore `.PHONY` ordering churn). Linked decisions in the PR thread.
```

## Lessons Learned

- 2025-09-17 — Legacy GitHub issues drift fast; triage with `possibly-*` labels and mirror anything still relevant into `docs/features/Features_Ledger.md` so the ledger stays the source of truth.
- 2025-09-19 — Treat clang-tidy "warning counts" as per-diagnostic, not per-include churn; measure progress via filtered reports and keep lint passes surgical without opportunistic feature work.

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
- Current top priority: drive clang-tidy to zero warnings. Pause feature work; focus on cleaning modules one by one. Measure per-file warning counts via `rg 'path/to/file' clang-tidy-report-full.txt | wc -l` and confirm the filtered count drops to 0 before moving on.

### One-Thing Rule (Touched code policy)

- You touch it? You refactor it. If you modify a type or function that clearly bundles multiple concerns in a single file, split out the thing you touched into its own file (1 file = 1 thing):
  - One enum per header.
  - One struct per header (public structs under `include/gitmind/`, internal-only in `core/include/gitmind/<module>/internal/`).
  - One responsibility per C file where practical.
- You touch it? You factor it out. Make sure the item you edited truly lives in its own file: one enum/struct/module per translation unit, no dumping new responsibilities into an existing compilation unit just because it was nearby.
- Beyond the “one thing” split, every touched area must uphold SRP, be test-double friendly, and lean on dependency injection. Whenever an operation can fail, surface a proper result type. New or modified code must stay warning-free under the current clang‑tidy rules before the change is considered done.
- Scope: Only the item you touched (don’t boil the ocean). If you edit `Bar` in `foo.c` and `foo.c` contains 12 types, you only have to extract `Bar` now.
- Enforcement: Pre-commit includes a heuristic check (`one-thing`); bypass in emergencies with `GM_ONE_THING_ALLOW=1` in the environment. PRs should include a brief note when bypassed.
- History policy: Do not amend or rebase once commits exist. Create new commits for follow-up fixes, merge instead of rebasing, and never rewrite history. Force pushes are forbidden—if you believe one is unavoidable, stop and get explicit user approval first.

## Working Knowledge (for agents)

- Journal-first graph: Edges are CBOR commits under `refs/gitmind/edges/<branch>`; cache lives in `refs/gitmind/cache/<branch>` and is rebuildable (never merged).
- Semantics as names: Store `type_name` and `lane_name` as UTF-8 strings on edges; do not collapse into a generic/custom type. Derive 64-bit IDs from names (NFC + stable hash) only for cache/filters.
- Time-travel correctness: All semantics (and optional advice) are in-history; queries evaluate against the chosen commit and branch.
- Merge/conflict model: Append-only journal; edge ULIDs form an OR-Set; “Semantics Advice” (optional) merges with hybrid CRDT (LWW scalars, OR-Set collections).
- Link vs code authors: Edge attribution `author` is the link creator (from `git config` unless provided); code authorship at the chosen commit is recorded separately (per-file last commit author/time) when captured by external tooling.
- Docker hygiene: Images are namespaced/labeled (`gitmind/ci:clang-20`, `gitmind/gauntlet:<compiler>`; label `com.gitmind.project=git-mind`). The CI image is built locally from `.ci/Dockerfile`; use `make docker-clean` to reclaim space safely.
- CI/Tidy nuance: Local builds and tests pass. Clang-tidy in Docker depends on CRoaring headers in the CI image; add a source build step for deterministic results on aarch64 if CI flags it.
 - CI path filters: Core build/test workflows are restricted to code paths; doc-only changes do not trigger core builds. Markdown lint runs separately with reduced noise via `.markdownlint.jsonc`.

## Agent Activity Log

See archives under `docs/activity/` for older logs.

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

## Regression Guardrails

- Build/run
  - Docker-only for build and test; use `make ci-local` or `tools/ci/ci_local.sh`.
  - Meson gate blocks host builds; override only with `-Dforce_local_builds=true` when necessary.
- API semantics
  - Equality is OID-first: if both OIDs present they must match; SHA fallback only when OIDs are absent.
  - Refs: build via `gm_build_ref`; reject inputs starting with `refs/`; follow ref-name safety rules.
- Safety ops
  - Use `gm_snprintf`/`gm_strcpy_safe`/`gm_memcpy_span`; check return codes; treat truncation as error.
  - Zero output buffers on error paths before formatting or copying.
- Public headers
  - Umbrella-safe; direct includes only; add C++ linkage guards; stable header guards.
- Cache/journal
  - Journal is append-only under `refs/gitmind/edges/<branch>`; cache under `refs/gitmind/cache/<branch>`.
  - Avoid ABI breaks in public structs; append new fields at struct tail; mark deprecated fields.
- Paths/buffers
  - Prefer `GM_PATH_MAX`; do not use `GM_PATH_MAX*2`; allocate if larger is required.
- Docs
  - Front matter first; single H1; `## Table of Contents`; titles match H1; add `api_version` to API docs.
  - License/SPDX comments after front matter.
- Tests
  - Run in Docker; avoid HEAD assumptions in bare repos; seed branches in tests when needed.

## PR Review Checklist

- Scope & safety
  - Docker-only build/test verified; no host-only assumptions.
  - No public ABI breaks (or explicitly called out and versioned).
- Semantics & correctness
  - OID-first behavior enforced; no SHA fallback when OIDs exist.
  - Refs created via `gm_build_ref` with input validation.
- Security & robustness
  - All string/memory ops use safe wrappers; truncation and errors handled.
  - Outputs zeroed on failure; no partial writes.
- Headers & includes
  - C++ linkage guards present; include-what-you-use; umbrella viability.
- Docs & tooling
  - Front matter valid; titles/H1 aligned; TOC heading H2; API docs include `api_version`.
  - Changelog updated for pushes to `main` (pre-push hook regex covers variants).
- CI & review hygiene
  - Local CI (Docker) passes (tests + tidy as configured).
  - If CodeRabbit runs: summary-first; propose fix-it patches; avoid noisy doc inlines.

References
- Guide: `docs/tools/Review_Seeding.md` (end-to-end review seeding + bot setup)
- Code review seeding script: `tools/review/seed_feedback_from_github.py` (uses `GITHUB_TOKEN`).
- CodeRabbit config: `.coderabbit.yml` (summary-first, caps, doc filters).
- PR template: `.github/pull_request_template.md` (guidance for reviewers).
