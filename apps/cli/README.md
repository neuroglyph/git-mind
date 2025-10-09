---
title: CLI Quickstart
description: Flags, output channels, and common recipes for using the git-mind CLI.
audience: [developers]
domain: [operations]
tags: [cli, logging, porcelain]
status: draft
last_updated: 2025-10-08
---

# CLI Quickstart

## Flags

- `--verbose` — increase CLI stderr logger to DEBUG.
- `--porcelain` — print stable key=value lines to stdout (friendly to scripts).
- `--json` — format service logs as compact JSON strings (to stderr).

## Output Channels

- Stdout — command results (human or porcelain). Use this in scripts.
- Stderr — logs produced by services (text or JSON). Use for diagnostics.

## Recipes

Human output (stdout) + text logs (stderr):

```
git-mind link src.c tgt.c --type references
```

Porcelain output (stdout) + JSON logs (stderr):

```
git-mind --json --porcelain list 2>logs.json | tee list.txt
```

Debugging with diagnostics events:

```
export GITMIND_DEBUG_EVENTS=1
git-mind cache-rebuild 2>diag.txt
```

- `GITMIND_DEBUG_EVENTS` wires the stderr diagnostics adapter automatically; the CLI cleans it up when the process exits.
- Logger levels now track the CLI output mode: default commands log at `INFO`, while `--verbose` (or `GM_OUTPUT_VERBOSE`) raises the stderr logger to `DEBUG`.

Piping JSON logs to jq:

```
git-mind --json cache-rebuild 2> >(jq -c .)
```

## Safety

The CLI prevents running in the git-mind development repository by default. To override for CI/E2E:

```
export GITMIND_SAFETY=off
```
