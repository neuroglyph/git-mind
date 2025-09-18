---
title: Tutorial (5–10 minutes)
description: A quick, practical walkthrough of the git-mind CLI using implemented commands.
audience: [users, developers]
domain: [cli]
tags: [tutorial, quickstart]
status: stable
last_updated: 2025-09-15
---

# Tutorial (5–10 minutes)

This walkthrough uses currently implemented commands. Some README examples are aspirational; see CLI docs under `docs/cli/` for what’s available today.

## 1) Build and Prepare

```bash
meson setup build && ninja -C build
export GIT_MIND=$(pwd)/build/git-mind
```

Optional: disable the safety guard for local experimentation (not recommended in the official repo):

```bash
export GITMIND_SAFETY=off
```

## 2) Link Concepts

Create a semantic edge between two files (document → code, code → test, etc.).

```bash
$ "$GIT_MIND" link README.md core/src/main.c --type documents
  Created: README.md -[documents]-> core/src/main.c
```

More linking examples and options: `docs/cli/gitmind-link.md`.

## 3) Explore Connections

List edges from a file to see what it’s connected to.

```bash
$ "$GIT_MIND" list --from README.md
README.md -> core/src/main.c (documents)
```

Formatting and filters: `docs/cli/gitmind-list.md`.

## 4) Rebuild Cache (if needed)

If you’ve been rebasing/rewriting history or migrating branches, rebuild the on-disk cache from the journal.

```bash
$ "$GIT_MIND" cache-rebuild
```

Details: `docs/cli/gitmind-cache-rebuild.md`.

## 5) Install Hooks (optional)

Install Git hooks so certain edges or metadata are captured automatically during commits.

```bash
$ "$GIT_MIND" install-hooks
```

Details: `docs/cli/gitmind-install-hooks.md`.

## Attribution and Sessions

Control attribution and sessions with environment variables:

- `GIT_MIND_SOURCE` (e.g., `human`, `ai`, `import`)
- `GIT_MIND_AUTHOR` (override your author id)
- `GIT_MIND_SESSION` (group related edges)

See `docs/operations/Environment_Variables.md`.

## What’s Next

- Journal and cache design: `docs/architecture/`
- Planned features and roadmap: `docs/roadmap.md`
- Contributing and development workflow: `CONTRIBUTING.md`
