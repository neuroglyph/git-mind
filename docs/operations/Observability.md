---
title: Observability (Logs, Metrics, Diagnostics)
description: How to observe git‑mind via logs, metrics, and diagnostics in local dev, CI, and scripts.
audience: [developers]
domain: [operations]
tags: [logging, metrics, diagnostics]
status: draft
last_updated: 2025-10-08
---

# Observability (Logs, Metrics, Diagnostics)

## Table of Contents

- Overview
- Logs (text/JSON)
- Metrics
- Diagnostics Events (dev/test)
- CLI recipes
- References

## Overview

git‑mind exposes three layers of observability:

- Logs — low‑cardinality events emitted by services (cache, journal).
- Metrics — counters/gauges/timings with bounded tag sets.
- Diagnostics — optional debug breadcrumbs for local dev and tests.

See: Telemetry Configuration for full environment knobs.

## Logs

- Default format: human text strings.
- JSON format: set `GITMIND_LOG_FORMAT=json` or pass `--json` to the CLI.
- Level: `GITMIND_LOG_LEVEL=DEBUG|INFO|WARN|ERROR` (services honor this); the CLI sets stderr logger minimum to DEBUG when `--verbose` is used.

Examples:

```
# Human logs to stderr
git mind cache-rebuild

# JSON logs to stderr (pipe to jq)
git mind --json cache-rebuild 2> >(jq -c .)
```

## Metrics

- Emitted via `gm_metrics_port`; wrappers no‑op unless a metrics adapter is wired.
- Tagging and repo hashing are controlled via environment (safe defaults).
- See Telemetry Configuration for knobs and naming conventions.

## Diagnostics Events (Dev/Test)

- Optional, low‑volume breadcrumbs separate from normal logs.
- Enable stderr diagnostics via:

```
export GITMIND_DEBUG_EVENTS=1
git mind cache-rebuild 2>diag.txt
```

- Use test fakes to capture events deterministically.
- See Diagnostics Events for event lists.

## CLI Recipes

Split stdout (results) and stderr (logs):

```
# Porcelain results + JSON logs
git mind --porcelain --json list 1>results.txt 2>logs.json
```

Tail JSON logs live:

```
git mind --json cache-rebuild 2> >(jq -c . | tee /tmp/gitmind.log)
```

Diagnostics + logs together:

```
export GITMIND_DEBUG_EVENTS=1
git mind --json cache-rebuild 2> >(tee /tmp/diag+logs.txt)
```

## References

- Telemetry Configuration: docs/operations/Telemetry_Config.md
- Diagnostics Events: docs/operations/Diagnostics_Events.md

