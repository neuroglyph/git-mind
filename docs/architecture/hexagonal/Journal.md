---
title: Journal (Hexagonal Architecture)
description: Journal writer/reader split into app coordinators and pure domain helpers.
audience: [developers]
domain: [architecture]
tags: [hexagonal, journal]
status: draft
last_updated: 2025-10-08
---

# Journal (Hexagonal Architecture)

## Table of Contents

- Overview
- Structure
- Telemetry & Diagnostics
- Notes

## Overview

The Journal bounded context persists edges as CBOR payloads in commit messages under `refs/gitmind/edges/<branch>`. In the hexagonal model:

- App coordinators live in `core/src/journal/` and orchestrate ports (git repo, logger, metrics, diagnostics).
- Pure domain helpers live under `core/src/domain/journal/` and decide how to encode/decode messages and how to shape commit specs, without performing IO.

## Structure

- App coordinators
  - `core/src/journal/writer.c` — builds commit messages, resolves parent tip, and updates the ref via `gm_git_repository_port`.
  - `core/src/journal/reader.c` — walks commits for the journal ref and decodes edges via domain helpers.

- Domain helpers (pure)
  - `domain/journal/codec.c` — base64 encode/decode of CBOR to/from commit messages.
  - `domain/journal/append_planner.c` — builds a parent-aware commit plan (tree OID, message, parent list).
  - `domain/journal/read_decoder.c` — selects decoding strategy (attributed ↔ legacy) and performs safe conversions.

- Ports consumed
  - `gm_git_repository_port` (outbound) for commit/refs traversal and mutation.
  - `gm_logger_port`, `gm_metrics_port` for telemetry.
  - `gm_diagnostics_port` for low-volume debug breadcrumbs.

## Telemetry & Diagnostics

- Logs
  - writer: `journal_append_start`, `journal_append_ok`/`journal_append_failed`
  - reader: `journal_read_start`, `journal_read_ok`/`journal_read_failed`

- Metrics
  - `journal.append.duration_ms`, `journal.append.edges_total`
  - `journal.read.duration_ms`, `journal.read.edges_total`

- Diagnostics (optional, dev/test)
  - `journal_commit_create_failed{code}`
  - `journal_nff_retry{ref}` (non-fast-forward, one retry)
  - `journal_ref_update_failed{ref,code}`
  - `journal_read_message_failed{code}`
  - `journal_cbor_invalid{offset,remaining}`
  - `journal_walk_failed{code}`

## Notes

- Writer resolves parent tip via a repo walk; the selection policy (first seen) is kept simple and isolated from IO through the planning helper.
- Reader uses a pure decoder to keep format branching out of the IO loop.
- All coordinators guard cardinality in logs and keep metrics low-volume.
