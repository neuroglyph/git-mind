<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->
<!-- © 2025 J. Kirby Ross / Neuroglyph Collective -->
---
title: git-mind link
description: Create a semantic connection between two paths.
audience: [users, developers]
domain: [cli]
tags: [cli, link]
status: stable
last_updated: 2025-09-15
---

# git-mind link

> _"Everything the light touches is our kingdom... But a king's time as ruler rises and falls like the sun."_ - Mufasa

Table of Contents

- [Name](#name)
- [Synopsis](#synopsis)
- [Description](#description)
- [The Heart of Git-Mind](#the-heart-of-git-mind)
- [Options](#options)

## NAME

`git-mind link` - Create semantic connections between files

## SYNOPSIS

```bash
git mind link <source> <target> --type <relationship> [options]
```

## DESCRIPTION

In the vast savanna of your codebase, no file stands alone. Each is connected to others through invisible threads of meaning - a test file validates its implementation, documentation explains its design, configuration guides its behavior.

The `git mind link` command makes these connections visible and permanent. It creates semantic edges that travel through time with your code, surviving refactors, renames, and the endless circle of development.

## THE HEART OF GIT-MIND

This is the command that started it all. When you create a link, you're not just adding metadata - you're encoding understanding into the fabric of your repository. These links become part of your Git history, as permanent as the code itself.

```bash
# The moment of connection
$ git mind link design.md core/src/parser.c --type implements
Created edge: design.md ──implements──> core/src/parser.c
```

From this moment forward, anyone who checks out your code can discover this relationship. More importantly, they can travel back in time and see what you understood at any point in history.

## OPTIONS

`--type <relationship>`
: __Required.__ The type of semantic relationship. Common types:

- `implements` - Source implements design/spec in target
- `references` - Source references or mentions target
- `depends_on` - Source requires target to function
- `tests` - Source tests the functionality in target
- `documents` - Source documents how target works
- `inspires` - Source inspired the creation of target
- `refactors` - Source is refactored version of target
- _...or any relationship that captures your understanding_

`--confidence <float>`
: Confidence level from 0.0 to 1.0 (default: 1.0)

- `1.0` - Certain (default)
- `0.8` - Highly confident
- `0.5` - Somewhat confident
- `0.2` - Speculative

`--note <message>`
: Add a note explaining the connection (stored in commit message)

`--bidirectional`
: Create edges in both directions

`--quiet`
: Suppress success message

## EXAMPLES

### Basic usage - The daily workflow

```bash
# Implementation tracks design
$ git mind link core/src/auth.c docs/auth-design.md --type implements

# Test validates implementation  
$ git mind link tests/test_auth.c core/src/auth.c --type tests

# Config guides behavior
$ git mind link core/src/auth.c config/auth.yaml --type depends_on
```

### With confidence - Encoding uncertainty

```bash
# Human creating a confident link (confidence defaults to 1.0)
$ git mind link core/src/parser.c rfc/grammar.txt --type implements

# AI suggesting a relationship with confidence score
$ export GIT_MIND_SOURCE=claude
$ export GIT_MIND_AUTHOR=claude@anthropic
$ git mind link core/src/parser.c rfc/grammar.txt --type implements --confidence 0.85
Created link: core/src/parser.c ──implements──> rfc/grammar.txt [claude: claude@anthropic, conf: 0.85]

# High-confidence AI relationship
$ git mind link core/src/algorithm.c papers/dijkstra62.pdf --type references --confidence 0.95
```

### Bidirectional links - Symmetric relationships

```bash
# These files are peers that reference each other
$ git mind link core/src/client.c core/src/server.c --type references --bidirectional
Created edge: core/src/client.c ──references──> core/src/server.c
Created edge: core/src/server.c ──references──> core/src/client.c
```

### Human-AI Collaboration - Attribution System

```bash
# Human creates edge (default behavior)
$ git mind link README.md core/src/main.c --type documents
Created link: README.md ──documents──> core/src/main.c

# AI creates edge with attribution
$ export GIT_MIND_SOURCE=claude
$ export GIT_MIND_AUTHOR=claude@anthropic  
$ export GIT_MIND_SESSION=analysis_2025
$ git mind link core/src/auth.c config/oauth.json --type depends_on --confidence 0.82
Created link: core/src/auth.c ──depends_on──> config/oauth.json [claude: claude@anthropic, conf: 0.82]

# System-generated edge (from hooks)
$ export GIT_MIND_SOURCE=system
$ git mind link old_file.c new_file.c --type augments
```

## HOW IT WORKS

### Git-Native Storage

Each link is stored as a Git commit in `refs/gitmind/edges/<branch>`. The commit contains:

- CBOR-encoded edge data in the commit message
- Empty tree (git's null tree: 4b825dc...)
- Author/committer from your Git config
- Timestamp of creation

### Content Addressing

Links connect specific versions of files using their Git object IDs (OIDs):

```
source_oid: 8a9f3b2c...  # Object ID of design.md's content
target_oid: 7c4d5e6f...  # Object ID of parser.c's content
```

This means links point to exact versions. The CLI still exposes flags such as `--show-sha`; those names linger for compatibility but surface the same OID values. When files change, the AUGMENTS system (see `git-mind-install-hooks(1)`) tracks their evolution.

### Branch Isolation

Links are branch-specific. Create different connections on different branches:

```bash
git checkout -b feature/auth
git mind link core/src/auth.c new-auth-api.md --type implements

git checkout main  
git mind list  # Won't show the feature branch link
```

## RELATIONSHIP TYPES

While you can use any string as a relationship type, these conventions help maintain consistency:

### Structural Relations

- `implements` - Code implements a design/specification
- `depends_on` - Requires target to function correctly
- `extends` - Builds upon target's functionality
- `replaces` - Newer version replacing target

### Documentation Relations

- `documents` - Explains how target works
- `references` - Mentions or links to target
- `examples` - Shows usage of target
- `tutorials` - Teaches concepts in target

### Development Relations

- `tests` - Validates target's behavior
- `benchmarks` - Measures target's performance
- `fixes` - Resolves issues in target
- `refactors` - Restructured version of target

### Inspiration Relations

- `inspires` - Target inspired this implementation
- `adapts` - Modified version of target's approach
- `conflicts` - Incompatible with target
- `discusses` - Analyzes or critiques target

## BEST PRACTICES

### Be Specific

```bash
# Too vague
$ git mind link core/src/parser.c docs/ --type references  # Which doc?

# Better
$ git mind link core/src/parser.c docs/parser-design.md --type implements
```

### Use Conventional Types

Stick to common relationship types when possible. This makes queries more powerful:

```bash
# Find all tests easily
$ git mind list --type tests
```

### Link at the Right Granularity

- Link files, not directories (unless the directory is the unit)
- Link to specific documents, not entire folders
- Link the most specific relevant files

### Document Non-Obvious Links

```bash
# Why does the parser depend on the logger config?
$ git mind link core/src/parser.c config/logger.yaml --type depends_on \
    --note "Parser uses logger for syntax error reporting"
```

## PERMANENCE & EVOLUTION

_"Remember who you are."_

Every link you create becomes part of your repository's permanent history. Future developers can:

- See what you connected and why
- Travel back to see historical connections
- Understand the evolution of architectural decisions

This is why git-mind exists: to encode not just what your code is, but what it means and how it relates to everything else.

## ERROR HANDLING

Common errors and solutions:

__"File not found"__

- Ensure both source and target exist in the repository
- Use paths relative to repository root
- Check for typos in filenames

__"Not in a git repository"__

- Run from within a Git repository
- Initialize with `git init` if needed

__"Invalid confidence value"__

- Use a float between 0.0 and 1.0
- Default is 1.0 if not specified

## SEE ALSO

- `git-mind-list(1)` - Query semantic links
- `git-mind-traverse(1)` - Explore connection graphs
- `git-mind-install-hooks(1)` - Track file evolution
- `git-mind(1)` - Main command overview

## THE PHILOSOPHY

_"There's more to being a king than getting your way."_

Creating links is an act of documentation, but more than that - it's an act of teaching. Every link you create teaches future readers (including future you) about the relationships that matter.

The power isn't in the tool. It's in the understanding you encode with it.

---

_"You are more than what you have become. Remember."_ - Your semantic links remember for you.
