---
title: Local CI Parity (Docker)
description: How to run CI-equivalent builds and lint locally using project Docker images.
audience: [contributors, developers]
domain: [quality]
tags: [docker, ci, clang-tidy]
status: stable
last_updated: 2025-09-15
---

# Local CI Parity (Docker)

Table of Contents

- [TL;DR workflow](#tldr-workflow)
- [Notes](#notes)

This guide shows how to run the same checks locally that CI/CD runs, using the project’s Docker images and helper scripts.


## TL;DR workflow

1) Open dev shell (optional but handy)
   - `make dev`

2) Build + test in Docker (Meson/Ninja inside CI container)
   - `make build-docker`
   - `make test-docker`
3) Clang-tidy like CI
   - Full parity pass: `./tools/docker-clang-tidy.sh`
   - Diff-guard on changed core files vs main: `./tools/tidy-diff-docker.sh origin/main`
     - To allow a push despite diff warnings (not recommended): `GITMIND_TIDY_DIFF_ALLOW=1 ./tools/tidy-diff-docker.sh origin/main`

4) Strict multi-compiler build (optional, slower)
   - `./tools/gauntlet/run-gauntlet.sh`
   - Optional full local CI via `act`: `GITMIND_PREPUSH_FULL=1 git push`
     - Requires: `make install-hooks` (project Git hooks) and `act` installed locally.
## Notes

- Prerequisites: Docker, Git, Make. Optional: GitHub CLI (`gh`) for PR checks, `act` for local CI.
- Docker‑only builds: Meson hard‑fails host builds. Use the helpers above.
- CI clang‑tidy excludes `core/src/hooks/`; local scripts mirror that.
- For PRs, validate locally first, then push and check GitHub checks with `gh pr view`.
- CI clang‑tidy excludes `core/src/hooks/`; local scripts mirror that.
- For PRs, validate locally first, then push and check GitHub checks with `gh pr view`.
