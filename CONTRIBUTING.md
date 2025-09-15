# Contributing to git-mind

Thanks for your interest in improving git‑mind! This guide summarizes expectations and the local workflow.

## Development Workflow

1. Build and test
   - `meson setup build && ninja -C build`
   - `ninja -C build test`
2. Lint (CI parity)
   - `./tools/docker-clang-tidy.sh` → writes `clang-tidy-report.txt`
   - Header guards: `meson run -C build lint_header_guards`
   - Public headers compile: `ninja -C build header-compile`
3. Keep diffs focused; match existing patterns in `core/` and `include/`.

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

## Getting Help

Open a Discussion or Issue describing what you’re trying to do and where you’re blocked. Include your environment, commands run, and logs where possible.

