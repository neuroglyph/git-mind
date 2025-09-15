---
title: Deployment and Operations
description: CI/CD pipeline, environments, rollback, and monitoring.
audience: [contributors, developers]
domain: [operations]
tags: [deployment, ci]
status: draft
last_updated: 2025-09-15
---

# Deployment and Operations

Table of Contents

- [Executive Summary](#executive-summary)
- [CI/CD Pipeline](#cicd-pipeline)
- [Environments](#environments)
- [Rollback Procedures](#rollback-procedures)
- [Monitoring and Alerts](#monitoring-and-alerts)

## Executive Summary

The project ships as a CLI and a C library. CI enforces correctness and toolchain standards; artifacts are Docker images and test/tidy reports. Operations focus on reproducible builds and safe rollback of refs. See also: [Test Plan](../testing/Test_Plan.md).

## CI/CD Pipeline

Stages

1) Lint/Format: pre-commit checks, clang-tidy (non-blocking for non-target modules)
2) Build: Meson/Ninja builds for gcc-14 and clang-20
3) Test: unit + integration (CLI flows)
4) Tidy Gate: zero warnings for cache/journal/cbor
5) Package: Docker images with pinned toolchains
6) Canaries: older compilers (non-blocking)

Gating Rules

- Fail if compiler < C23 (gcc-14/clang-20)
- Fail if tidy warnings exist in scoped modules

## Environments

- Dev: Docker shell (`make dev`); local git repo; optional local tools (outside this repo) for export testing
- Prod: Not applicable (local CLI). For library consumers, publish release archives and image tags.

## Rollback Procedures

1) Build/Tooling Regression
   - Revert CI workflow commit; re-run pipeline
2) Journal Data Regression
   - Identify bad commit(s) under `refs/gitmind/edges/<branch>`
   - Create a new ref at last-known-good commit
   - Force-update the branch ref (local); push if publishing refs
3) Cache Issues
   - Delete `refs/gitmind/cache/<branch>` and rebuild via CLI

## Monitoring and Alerts

- Local metrics: emit counts/timings to stderr in verbose mode
- Health checks: `git mind status` to validate env and cache/journal alignment
- Thresholds (local logging only)
  - Rebuild rate < 20k edges/sec → warn
  - Median query > 20ms on sample → warn
