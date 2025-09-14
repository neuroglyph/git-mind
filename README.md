<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->
<!-- © J. Kirby Ross / Neuroglyph Collective -->

# git-mind 🧠

<p align="center">
  <img src="assets/logo.jpg" alt="git-mind logo" width="200" />
</p>

## `git‑mind` turns Git repositories into serverless, distributed graph databases where relationships are first‑class and move through time with your history

- Version your thoughts
- Travel back in time to see what you were thinking and then step forward, watching your thoughts evolve and change over time
- Branch your mind
- Fork someone else's
- Merge your thoughts
- AI/human co‑cognition platform

---

Table of Contents

- [Overview](#overview)
- [Why git-mind](#why-git-mind)
- [How It Works](#how-it-works)
- [Core Concepts](#core-concepts)
- [Quickstart](#quickstart)
- [Human + AI Co‑Thought](#human--ai-co-thought)
- [Status & Roadmap](#status--roadmap)
- [Architecture](#architecture)
- [Contributing](#contributing)
- [License](#license)

## Overview

git‑mind lets you link code, docs, notes, experiments — anything tracked in Git — with first‑class semantic edges (e.g., “implements”, “tests”, “documents”, “refines”, “depends_on”).

These edges are stored in your repository (no servers), replicate with `git clone`, and are scoped to branches and commits. Check out an old commit and you get the semantics from that moment in time. Merge branches and the graph merges deterministically, just like your code.

In short: organize ideas and artifacts as a graph, with Git as the transport, history, and security model.

## Why git-mind

- Version your thoughts: track the “why” and “how” alongside the “what”.
- Serverless graph DB: your repo is the database — clone, branch, merge.
- Time‑travel semantics: branch/commit‑scoped relationships that evolve.
- Query by meaning: stop guessing with `grep`; ask the graph directly.
- Fast locally: a Roaring Bitmap cache makes common queries instant.
- Cross‑repo, forkable semantics: share, branch, and merge understanding across clones.

Human + AI co‑thought

- Shared memory for people and tools: the repo holds both your links and AI‑suggested links.
- Trust and review: filter by attribution (human/AI) and lanes (e.g., suggested, verified).
- Forkable cognition: try ideas in branches, then merge deterministic edges back.

Use cases

- Architecture: link docs ⇄ code ⇄ tests; explore fan‑in/out and evolution.
- Notes/Zettelkasten: link ideas, excerpts, and references across a repo.
- Research: connect papers, datasets, scripts, and results with provenance.
- Product: trace features through specs, issues, code, tests, and docs.
- Decisions: tie ADRs to impacted modules and follow their downstream effects.

## How It Works

- Journal (truth)
  - Each edge append becomes a Git commit under `refs/gitmind/edges/<branch>` with a compact CBOR payload.
  - Append‑only, branch‑scoped, time‑travel‑safe; merges behave like code.
- Cache (speed)
  - Optional Roaring Bitmap indices under `refs/gitmind/cache/<branch>` for O(log N) set ops.
  - Rebuildable; never merged; safe to delete.

## Core Concepts

- Edge: A directed link (source → target) between any two Git blobs with metadata.
- Names‑as‑truth: `type_name`/`lane_name` are stored as strings on the edge; IDs are derived only for performance.
- Attribution: Who/what created the edge (human/AI), with author/session metadata. Surfaces as filters and review signals.
- AUGMENTS: Evolution links (old blob → new blob) created on edits to preserve meaning through change.
- Advice (optional): Data‑driven semantics (e.g., symmetry, implies) that merge deterministically.

## Quickstart

Requirements (dev): Git, Meson, Ninja, C23 compiler (gcc‑14 or clang‑20 recommended). See [DEV_SETUP](docs/DEV_SETUP.md).

Build and test

```bash
make            # meson+ninja build in ./build
make test       # runs unit tests
```

Create and query links (code, docs, notes)

```bash
# Link a design doc to its implementation
git mind link docs/auth-flow.md src/auth.c --type implements --lane verified

# List links (human)
git mind list --from src/auth.c

# Example (stub) output
> docs/auth-flow.md  (type: implements, lane: verified)

# List links (JSON)
git mind list --from src/auth.c --format json

# Rebuild performance cache for current branch
git mind cache-rebuild

# Link notes and research
git mind link notes/idea.md notes/followup.md --type refines --lane journal
git mind link notes/notes-on-paper.md data/paper.pdf --type cites

# Query note graph (stub)
git mind list --from notes/idea.md
> notes/followup.md  (type: refines, lane: journal)
```

What to expect

- Links are stored under `refs/gitmind/edges/<branch>` and show up in history.
- Queries use the cache when available; otherwise scan the journal.
- Everything is just Git — no external servers, no hidden DBs.

### Safety Guard

- git‑mind refuses to run inside its own source repository to prevent accidental journal writes.
- Detection uses Git remotes (via libgit2) and strict matching of the official repo path (`neuroglyph/git-mind[.git]`).
- To explicitly bypass (e.g., certain CI/E2E scenarios), set `GITMIND_SAFETY=off`.
- See also: docs/operations/Environment_Variables.md for all supported env vars.

## Human + AI Co‑Thought

git‑mind is designed to be a shared, versioned memory for humans and AI — a place where both parties can write edges, discover connections, and converge by merging branches.

- Shared memory, no servers: the repo is the database; AI tools can read/write edges locally just like you.
- Clear authorship: attribution marks edges as human/AI with author/session metadata.
- Lanes for review: AI can write to a `suggested` lane; humans accept into `verified`.
- Deterministic merges: edges are append‑only with ULIDs; advice uses hybrid CRDT rules, so branches converge predictably.
- Control and safety: filter by attribution, lanes, or commit; disable advice application; keep AI ops in a branch until reviewed.

Example flows (concept)

```bash
# AI suggests edges into a separate branch/lane
git checkout -b ai/suggestions
git mind link notes/idea.md src/feature.c --type implements --lane suggested --source ai

# Human reviews and merges
git checkout main
git merge ai/suggestions
git mind list --lane verified   # or filter out --source ai

# Inspect AI suggestions (stub output)
git mind list --lane suggested --source ai
> notes/idea.md  ->  src/feature.c  (type: implements, lane: suggested, source: ai)
```

See: [Attribution System](docs/architecture/attribution-system.md) and [ADR 0001](docs/adr/0001-first-class-semantics.md).

## Status & Roadmap

Project status: early‑stage, with core primitives usable today. The vision is a shared, serverless, forkable thought‑graph for humans and AI. Today we are focused on the core that makes that vision real over time:

- Shipping now/next: journal (edges‑as‑commits), cache (fast queries), CLI (link/list/cache‑rebuild), names‑as‑truth semantics, AUGMENTS for evolution.
- Optional (behind flags, later): advice application (symmetry/implies), co‑thought workflows (AI “suggested” lanes, attribution filters), MCP service for tools to read/write edges locally.

Expect CLI and APIs to change as we stabilize the core. See the planning docs for the full project scope and staging.

- Roadmap: [Product Roadmap](docs/planning/Product_Roadmap.md)
- Releases: [Release Plans](docs/planning/Release_Plans.md)
- Milestones & Sprints: [Milestones](docs/planning/Milestones.md), [Sprint Plans](docs/planning/Sprint_Plans.md)

## Architecture

- System overview: [System Architecture](docs/architecture/System_Architecture.md)
- Data model: edges as commits, names‑as‑truth, branch/time scoping — see [Journal Architecture Pivot](docs/architecture/journal-architecture-pivot.md)
- Cache design: [Bitmap Cache](docs/architecture/bitmap-cache-design.md)
- Semantics PRD & ADR: [PRD](docs/PRDs/PRD-git-mind-semantics-time-travel-prototype.md), [ADR 0001](docs/adr/0001-first-class-semantics.md)

## Contributing

- Start here: [CONTRIBUTING.md](CONTRIBUTING.md)
- Dev setup: [DEV_SETUP](docs/DEV_SETUP.md)
- Docs index: [docs/README.md](docs/README.md)
- Pre‑commit: `pre-commit install` (clang‑format, docs link/TOC checks, secrets)

Principles

- C23, warnings‑as‑errors; keep clang‑tidy warnings at zero in target modules.
- Names‑as‑truth for semantics; caches are derived and rebuildable.
- Small, pause‑safe increments; document decisions (ADRs) as you go.

### Target Architecture

```
git-mind/
├── libgitmind/     # Single-header library
│   ├── core/       # Foundation (types, crypto, I/O)
│   ├── graph/      # Graph operations (edges, attribution)
│   └── storage/    # Git persistence (journal, cache)
├── apps/
│   ├── cli/        # Command-line interface
│   ├── hooks/      # Git hooks (separate binaries)
│   ├── mcp/        # Model Context Protocol server
│   └── web/        # Web UI daemon
└── bindings/       # Language bindings (Python, Rust, etc.)
```

## 🚀 Future Directions (Exploratory)

Note: Aspirational concepts — not implemented yet.

### 🧠 Semantic Intelligence

- __AI-Powered Discovery__: Automatically detect and suggest relationships between code artifacts
- __Natural Language Queries__: "Show me all code that implements authentication"
- __Intelligent Refactoring__: Track concept migrations across architectural changes

### 🌐 Distributed Knowledge

- __Cross-Repository Links__: Connect knowledge across project boundaries
- __Federated Graphs__: Share and merge knowledge graphs between teams
- __Knowledge Synchronization__: Keep understanding in sync across distributed teams

## Support

- Issues: [GitHub Issues](https://github.com/neuroglyph/git-mind/issues)
- Discussions: [GitHub Discussions](https://github.com/neuroglyph/git-mind/discussions)
- Documentation: [docs/](docs/)

---

## License

Licensed under `LicenseRef-MIND-UCAL-1.0`. See [LICENSE](./LICENSE) file for details.

© J. Kirby Ross • <https://github.com/flyingrobots>
