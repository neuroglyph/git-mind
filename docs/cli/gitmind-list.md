---
title: git-mind list
description: Query and explore semantic connections.
audience: [users, developers]
domain: [cli]
tags: [cli, list]
status: stable
last_updated: 2025-09-15
---

<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->
<!-- © 2025 J. Kirby Ross / Neuroglyph Collective -->

# git-mind list

> _"Look beyond what you see."_ - Rafiki

Table of Contents

- [Name](#name)
- [Synopsis](#synopsis)
- [Description](#description)
- [The Power of Seeing](#the-power-of-seeing)

## NAME

`git-mind list` - Query and explore semantic connections

## SYNOPSIS

```bash
git mind list [options]
```

## DESCRIPTION

In the Pride Lands of your codebase, every file has connections - some obvious, some hidden, some forgotten. The `git mind list` command reveals these relationships, showing you the semantic web that binds your code together.

This is your window into understanding. Use it to discover dependencies, trace implementations, find tests, and see how knowledge flows through your repository.

## THE POWER OF SEEING

Without git-mind:

```bash
# Where is this implemented?
$ grep -r "auth flow" docs/
# What tests cover this?
$ find . -name "*test*" | xargs grep -l "parser"
# What depends on this config?
$ # ...good luck
```

With git-mind:

```bash
# All connections at a glance
$ git mind list --from core/src/parser.c

# Specific relationships
$ git mind list --type tests --to core/src/auth.c

# Everything in the web
$ git mind list
```

## OPTIONS

### Filtering Options

`--from <path>`
: Show only edges originating from this file

`--to <path>`
: Show only edges pointing to this file

`--type <relationship>`
: Filter by relationship type (implements, tests, etc.)

`--min-confidence <min>`
: Show only edges with confidence >= threshold (0.0-1.0)

### Display Options

`--format <format>`
: Output format (default: human)

- `human` - Readable format with arrows
- `json` - Machine-parseable JSON
- `tsv` - Tab-separated values
- `dot` - Graphviz DOT format

`--show-sha`
: Display blob object IDs (OIDs). The flag name retains the legacy “SHA” label.

`--show-augments`
: Include AUGMENTS evolution edges

`--show-attribution`
: Display attribution information for all edges

### Time Travel Options

`--at <commit>`
: Show connections as they existed at specific commit

`--branch <branch>`
: Query different branch (default: current)

### Performance Options

`--no-cache`
: Force journal scan (bypass cache)

`--limit <n>`
: Limit results (default: unlimited)

## EXAMPLES

### Basic Queries

Show all connections:

```bash
$ git mind list
README.md ──references──> docs/quickstart.md
core/src/main.c ──implements──> docs/design.md
tests/test_parser.c ──tests──> core/src/parser.c
core/src/parser.c ──depends_on──> config/grammar.yaml
... (42 more edges)
```

### Focused Queries

What does this file connect to?

```bash
$ git mind list --from core/src/auth.c
core/src/auth.c ──implements──> docs/auth-design.md
core/src/auth.c ──depends_on──> config/auth.yaml
core/src/auth.c ──references──> lib/crypto.c
```

What connects to this file?

```bash
$ git mind list --to core/src/parser.c
tests/test_parser.c ──tests──> core/src/parser.c
tests/bench_parser.c ──benchmarks──> core/src/parser.c
docs/parser-guide.md ──documents──> core/src/parser.c
core/src/compiler.c ──depends_on──> core/src/parser.c
```

### Filtered Queries

Find all test files:

```bash
$ git mind list --type tests
tests/test_auth.c ──tests──> core/src/auth.c
tests/test_parser.c ──tests──> core/src/parser.c
tests/integration/test_flow.c ──tests──> core/src/flow.c
```

Find uncertain connections:

```bash
$ git mind list --min-confidence 0.8
core/src/auth.c ──depends_on──> config/oauth.json [claude: claude@anthropic, conf: 0.85]
core/src/parser.c ──implements──> specs/grammar.md [gpt: gpt4@openai, conf: 0.92]
```

### Attribution Filtering

Show only human-created edges:

```bash
$ git mind list --source human
README.md ──documents──> core/src/main.c
core/src/auth.c ──implements──> docs/auth-design.md
tests/test_auth.c ──tests──> core/src/auth.c
```

Show only AI-suggested edges:

```bash
$ git mind list --source ai
core/src/parser.c ──depends_on──> lib/lexer.h [claude: claude@anthropic, conf: 0.78]
core/src/crypto.c ──references──> papers/aes.pdf [gpt: gpt4@openai, conf: 0.91]
```

Show attribution for all edges:

```bash
$ git mind list --show-attribution
README.md ──documents──> core/src/main.c [human: user@example.com]
core/src/auth.c ──depends_on──> config/oauth.json [claude: claude@anthropic, conf: 0.85]
core/src/test.c ──tests──> core/src/main.c [human: developer@team.com]
```

High-confidence AI insights only:

```bash
$ git mind list --source ai --min-confidence 0.9
core/src/crypto.c ──references──> papers/aes.pdf [gpt: gpt4@openai, conf: 0.91]
core/src/algorithm.c ──implements──> papers/dijkstra.pdf [claude: claude@anthropic, conf: 0.95]
```

### Time Travel Queries

See historical connections:

```bash
# What did we understand 50 commits ago?
$ git mind list --at HEAD~50

# What connections existed at release?
$ git mind list --at v1.0.0

# Show connections on feature branch
$ git mind list --branch feature/new-auth
```

### Evolution Tracking

See how files evolved:

```bash
$ git mind list --show-augments
core/src/auth.c:8a9f3b ──AUGMENTS──> core/src/auth.c:7c4d5e
core/src/auth.c:7c4d5e ──AUGMENTS──> core/src/auth.c:current
docs/api.md:3f2a1b ──AUGMENTS──> docs/api.md:current
```

### Machine-Readable Output

JSON for tooling:

```bash
$ git mind list --format json
{
  "edges": [
    {
      "source": "README.md",
      "target": "docs/quickstart.md",
      "type": "references",
      "confidence": 1.0,
      "timestamp": 1736637890000
    }
  ]
}
```

DOT for visualization:

```bash
git mind list --format dot > graph.dot
dot -Tpng graph.dot -o graph.png
```

## UNDERSTANDING THE OUTPUT

### Human Format (default)

```
<source> ──<type>──> <target> [metadata]
```

Metadata may include:

- `(confidence: 0.8)` - Less than certain connections
- `(OID: abc123)` - Shown when `--show-sha` is used (legacy flag name)
- `(AUGMENTED)` - When file has evolved

### Following AUGMENTS

By default, list follows AUGMENTS chains to show current connections:

```bash
# Original link: design.md -> old-parser.c
# After refactor: old-parser.c AUGMENTS new-parser.c
# List shows: design.md -> new-parser.c
```

Use `--no-follow-augments` to see historical links.

## PERFORMANCE

The list command uses the Roaring Bitmap cache for O(log N) queries:

- __With cache__: <10ms for 100K edges
- __Without cache__: ~5 seconds for 100K edges
- __Cache miss__: Automatically falls back to journal scan
- __Stale cache__: Detects and warns (run `git mind cache-rebuild`)

## ADVANCED PATTERNS

### Dependency Analysis

```bash
# What does my module depend on?
$ git mind list --from "core/src/mymodule/**" --type depends_on

# What depends on this config?
$ git mind list --to config/database.yaml --type depends_on
```

### Test Coverage Discovery

```bash
# What files lack tests?
$ comm -23 \
    <(git ls-files core/src/ | sort) \
    <(git mind list --type tests --format tsv | cut -f2 | sort)
```

### Architecture Visualization

```bash
# Generate architecture diagram
$ git mind list --format dot --type implements > arch.dot
$ dot -Tsvg arch.dot -o architecture.svg
```

### Change Impact Analysis

```bash
# What might break if I change this?
$ git mind list --to core/src/core-api.h --type depends_on
```

## PHILOSOPHY

_"The question is: Who are you?"_ - Rafiki

Your code is more than files and functions. It's a living system of relationships, dependencies, and meanings. The `list` command doesn't just show you connections - it reveals the hidden structure of your understanding.

Every query is a question about your code's nature. Every result is an answer that teaches.

## DIAGNOSTICS

Returns 0 on success, non-zero on failure.

Common warnings:

- `Cache stale, using journal scan` - Run `git mind cache-rebuild`
- `No edges found` - No links match your query
- `Following AUGMENTS chain` - File has evolved (use --no-follow-augments for original)

## SEE ALSO

- `git-mind-link(1)` - Create semantic connections
- `git-mind-traverse(1)` - Explore connection graphs
- `git-mind-cache-rebuild(1)` - Optimize query performance
- `git-mind(1)` - Main command overview

## THE LESSON

_"Oh, the past can hurt. But the way I see it, you can either run from it or learn from it."_

Every list query teaches you something about your codebase. The more you look, the more you see. The more you see, the more you understand.

---

_"It is time."_ - Time to see what connects your kingdom.
