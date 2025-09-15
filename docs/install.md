# Install git-mind

This project is early-stage. Prefer running in the provided Docker CI image or build from source locally. Homebrew/Winget packages are not published yet.

## Prerequisites

- Git, C toolchain, Meson, Ninja
- libgit2 and dependencies (present in CI image)
- Optional: Docker for reproducible builds

## Build From Source

```bash
meson setup build
ninja -C build
ninja -C build test
```

Notes:

- Builds are gated for safety. If you’re outside CI/Docker, set `GITMIND_ALLOW_HOST_BUILD=1` to allow a host build. See `docs/operations/Environment_Variables.md`.
- Public headers compile check: `ninja -C build header-compile`
- Header guard lint: `meson run -C build lint_header_guards`

## Lint (Clang-Tidy in Docker)

```bash
./tools/docker-clang-tidy.sh
```

Produces `clang-tidy-report.txt` with CI-parity configuration.

## CLI Binary

The CLI builds with the core. After building, the binary is in `build/`.

- E2E tests expect `GIT_MIND` to point at the binary:

```bash
export GIT_MIND=$(pwd)/build/git-mind
tests/e2e/run_all_tests.sh
```

## Safety Guard

The CLI includes a safety guard that blocks execution inside the official `git-mind` repo unless overridden for CI/testing. To disable (only when you understand the risk):

```bash
export GITMIND_SAFETY=off
```

Details: `docs/operations/Environment_Variables.md`.

## Uninstall / Clean

```bash
rm -rf build
make clean  # cleans shim targets if used
```

## Troubleshooting

- Host build refused: set `GITMIND_ALLOW_HOST_BUILD=1` or use the Docker scripts.
- Tidy mismatches: run `./tools/docker-clang-tidy.sh` to match CI image env.
- Headers fail to compile: run `ninja -C build header-compile` and fix include hygiene.

