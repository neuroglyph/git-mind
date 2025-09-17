# Contributing to git-mind

Thanks for your interest in improving git‑mind! This guide summarizes expectations and the local workflow.

## Development Workflow

> [!WARNING]
> **DO NOT BUILD GITMIND OUTSIDE OF DOCKER**
> git-mind manipulates Git internals (refs/*, objects, config). Building or testing on the host can corrupt this repository or others on your machine. Always use the CI Docker image for safety and parity.

> [!INFO]
> _If you really want to..._
> Use the container workflow:
> - `make ci-local` — docs checks + build + unit tests in the CI image
> - `tools/ci/ci_local.sh` — same as above, direct
> Advanced (at your own risk):
> - `meson setup build -Dforce_local_builds=true` — explicit Meson override
> - `GITMIND_ALLOW_HOST_BUILD=1` — legacy env override (discouraged)

1. Build and test (in Docker)
   - `make ci-local` or `tools/ci/ci_local.sh`
2. Lint (CI parity)
   - `./tools/docker-clang-tidy.sh` → writes `clang-tidy-report.txt`
   - Header guards: `meson run -C build lint_header_guards`
   - Public headers compile: `ninja -C build header-compile`
3. Keep diffs focused; match existing patterns in `core/` and `include/`.

## Planning & Feature Tracking

- `docs/features/Features_Ledger.md` is the single source of truth for roadmap, milestones, and tasks.
- Add new ideas under the appropriate feature group during **Capture**, tag them with a milestone, and log supporting tasks in the `## Tasklist` section.
- Progress bars are generated automatically—run `make features-update` after updating progress %, KLoC estimates, or tasks.
- Milestone callouts (`MVP`, `Alpha`, `Beta`, `v1.0.0`) auto-pull outstanding tasks based on `[Milestone]` tags; keep those tags in sync as work moves stages.

## Code Style

- Language: C23; warnings‑as‑errors; no VLAs or shadowing; explicit prototypes.
- Naming: `gm_` prefix for functions/vars; types end with `_t`; macros UPPER_SNAKE; header guards `GITMIND_*`.
- Includes: umbrella‑safe; specific headers preferred; C++ compatible (`extern "C"`).
- Formatting: `.clang-format` (LLVM‐based, 4 spaces, 80 cols, pointer alignment right). Pre‑commit runs `clang-format`.

## Commits and PRs

- Conventional commits: `type(scope): description` (e.g., `fix(core/cbor): handle null keys`).
- Reference issues where applicable (e.g., `Fixes #123`).
- PRs must: describe changes, include a short test plan, pass CI, and introduce no new clang‑tidy warnings.

## Safety and Environment

- Host builds may be gated; set `GITMIND_ALLOW_HOST_BUILD=1` to override locally.
- CLI safety guard: blocks running inside the official repo unless `GITMIND_SAFETY=off`.
- See `docs/operations/Environment_Variables.md` for all variables.

## Tests

- Unit tests: `core/tests/unit/` named `test_<module>.c`.
- E2E/integration tests under `tests/` with minimal fixtures.

## Reviews

- Keep `.coderabbit.yml` updated.
- Prefer summary reviews; limit per‑line comments on docs.
- If rejecting a suggestion, document rationale under `docs/code-reviews/rejected-suggestions/` with links.
- See `docs/tools/Review_Seeding.md` for the auto‑worksheet flow and how to seed PR feedback locally or via CI. Seeded files live under `docs/code-reviews/PR<PR#>/<sha>.md`.

## Getting Help

Open a Discussion or Issue describing what you’re trying to do and where you’re blocked. Include your environment, commands run, and logs where possible.
