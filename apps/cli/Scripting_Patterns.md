---
title: CLI Scripting Patterns
description: Practical patterns for combining porcelain output and JSON logs in scripts.
audience: [developers]
domain: [operations]
tags: [cli, scripting, porcelain, json]
status: draft
last_updated: 2025-10-08
---

# CLI Scripting Patterns

## Table of Contents

- Principles
- Porcelain parsing
- JSON logs with jq
- Split streams and aggregate
- Error handling

## Principles

- stdout is for results (human or porcelain); stderr is for logs.
- Prefer `--porcelain` for stable parsing; avoid scraping human strings.
- Use `--json` and pipe stderr through `jq` for structured log pipelines.

## Porcelain Parsing

Porcelain mode emits `key=value` lines, one record per line or block. Simple POSIX tools can parse this reliably.

```
git mind --porcelain list | awk '{ for (i=1;i<=NF;i++){ split($i,a,"="); printf "%s\t", a[2] } printf "\n" }'
```

Turn porcelain lines into JSON with jq:

```
git mind --porcelain list \
| awk '{ for (i=1;i<=NF;i++){ split($i,a,"="); printf "%s:%s ", a[1], a[2] } printf "\n" }' \
| jq -R -s 'split("\n") | map(select(length>0) | split(" ") | map(select(length>0)) | map(split(":")) | map({(.[0]): .[1]}) | add)'
```

## JSON Logs with jq

Enable service logs as JSON and pipe stderr through jq:

```
git mind --json cache-rebuild 2> >(jq -c . | tee /tmp/gitmind.logs)
```

Filter by event:

```
git mind --json cache-rebuild 2> >(jq -rc 'select(.event=="rebuild_ok")')
```

## Split Streams and Aggregate

Capture results and logs to separate files:

```
git mind --json --porcelain list 1>results.kv 2>logs.json
```

Join porcelain results with selected log fields in a script:

```
RESULTS=$(git mind --porcelain list)
LOGS=$(git mind --json list 2> >(jq -rc 'select(.event|test("journal_"))'))
echo "$RESULTS" > results.kv
echo "$LOGS" > logs.json
```

## Error Handling

- Check the exit code: `if git mind list --porcelain; then ... fi`
- When using JSON logs, you can watch for failure events (e.g., `rebuild_failed`) and handle them separately.

