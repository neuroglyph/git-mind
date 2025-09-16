---
title: Environment Variables
description: Environment variables affecting build, CI, runtime, and tests.
audience: [developers]
domain: [operations]
tags: [environment, ci, runtime, cli]
status: stable
last_updated: 2025-09-15
---

# Environment Variables

This page lists environment variables that influence git‑mind builds, tests, and runtime behavior.

## Build and CI

- `GITMIND_DOCKER`
  - Purpose: Allows host builds when set to `1` inside the CI Docker image or dev shell.
  - Used by: `meson.build` (host build e‑brake).
  - Default: unset (host builds refused unless in CI or overridden).

- `GITHUB_ACTIONS`
  - Purpose: Signals CI environment; permits builds in GitHub Actions.
  - Used by: `meson.build` (host build e‑brake).
  - Default: set by GitHub Actions runners.

- `GITMIND_ALLOW_HOST_BUILD`
  - Purpose: Explicit override to permit host builds outside Docker/CI.
  - Used by: `meson.build` (host build e‑brake).
  - Values: `1` to allow; otherwise refused.

- `GITMIND_CI_IMAGE`
  - Purpose: Select CI Docker image for local/CI runs that execute builds/tests in a container.
  - Used by: `.github/workflows/c_core.yml`, `tools/ci/ci_local.sh`.
  - Default: `gitmind/ci:clang-20`.

## Runtime (CLI)

- `GITMIND_SAFETY`
  - Purpose: Bypass the CLI safety guard that prevents running inside the git‑mind development repo.
  - Used by: `apps/cli/main.c`.
  - Values: `off`, `0`, or `false` to disable the guard.
  - Notes: The guard uses libgit2 to inspect Git remotes and blocks only if a remote URL strictly matches the official repo path (`neuroglyph/git-mind[.git]`).

- `GITMIND_CBOR_DEBUG`
  - Purpose: Enable verbose CBOR decode tracing in the journal reader.
  - Used by: `core/src/journal/reader.c`.
  - Values: `1`, `true`, or `yes` (case-insensitive) to enable; anything else disables.
  - Notes: Prints offsets and decode attempts to `stderr`; use only for debugging.

- `GIT_MIND_SOURCE`
  - Purpose: Set attribution source for edges (e.g., `human`, `claude`, `gpt`, `system`, `import`).
  - Used by: Core attribution (`gm_attribution_from_env`).

- `GIT_MIND_AUTHOR`
  - Purpose: Override author identifier recorded on edges.
  - Used by: Core attribution.

- `GIT_MIND_SESSION`
  - Purpose: Session/conversation ID for grouping related edges.
  - Used by: Core attribution.

- `GIT_MIND_BRANCH` (reserved)
  - Purpose: Planned: control branch context for certain operations.
  - Status: Reserved; not required today.

- `GIT_MIND_NO_CACHE` (reserved)
  - Purpose: Planned: disable cache usage to force journal scans.
  - Status: Reserved; not required today.

- `GIT_MIND_VERBOSE` (reserved)
  - Purpose: Planned: enable verbose output in some contexts.
  - Status: Reserved; not required today.

## Test Harness / E2E

- `GIT_MIND`
  - Purpose: Path to the built `git-mind` binary used by E2E tests.
  - Used by: `tests/e2e/run_all_tests.sh` and related scripts.
  - Example: `export GIT_MIND=$(pwd)/build/git-mind`

- `GITMIND_TEST_BRANCH`
  - Purpose: Direct journal writes to a specific branch in tests when the repository has no HEAD yet.
  - Used by: `core/src/journal/writer.c` (test override).
  - Example: `export GITMIND_TEST_BRANCH=testq`
  - Notes: For tests only; production code should rely on current branch.

## Git Hooks

- `HOOKS_BYPASS`
  - Purpose: Bypass local git hooks (e.g., the pre‑push CHANGELOG guard) in emergencies.
  - Usage: `HOOKS_BYPASS=1 git push`
  - Notes: Do not make a habit of bypassing hooks; update the CHANGELOG instead.

## Notes

- Prefer running builds and tests in Docker using the provided scripts. Host builds are intentionally gated to reduce accidental repository corruption.
- If you must run locally, set `GITMIND_ALLOW_HOST_BUILD=1` and understand the risks.
