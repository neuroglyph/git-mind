# git-mind

> Version your thoughts. Branch your ideas. Merge understanding.

**git-mind** turns any Git repository into a causally-versioned semantic knowledge graph. Link code, specs, decisions, and tasks with typed, confidence-scored edges. Query with composable views and lenses. Travel back in time to see what you understood then.

No servers. No databases. Just `git push`.

---

## Quick start

```bash
# Install
npm install -g git-mind

# Initialize in any Git repo
cd /path/to/your/repo
git mind init

# Create semantic edges
git mind link file:src/auth.js spec:auth --type implements
git mind link task:login spec:auth --type belongs-to
git mind link task:login task:setup --type depends-on

# See the graph
git mind list
git mind status

# Query with views and lenses
git mind view roadmap
git mind view roadmap:incomplete
git mind view roadmap:incomplete:frontier
```

---

## What it does

### Typed, confidence-scored edges

Every relationship has a type and a confidence score. Human-created edges default to 1.0. AI-suggested edges start low and get promoted through review.

```bash
git mind link spec:auth crate:jwt     --type depends-on --confidence 0.9
git mind link file:src/auth.js spec:auth --type implements
git mind link adr:0007 task:M11       --type blocks
```

| Edge type | Meaning |
|-----------|---------|
| `implements` | Source implements the target spec/design |
| `augments` | Source extends or enhances the target |
| `relates-to` | General semantic relationship |
| `blocks` | Source blocks progress on target |
| `belongs-to` | Source is a member/child of target |
| `consumed-by` | Source is consumed/used by target |
| `depends-on` | Source depends on target |
| `documents` | Source documents/describes target |

### Views and composable lenses

Views project the graph into focused surfaces. Lenses filter those projections further. Chain them with colons.

```bash
# Built-in views
git mind view roadmap        # milestones, tasks, blockers
git mind view architecture   # specs, ADRs, crates, relationships
git mind view backlog        # open tasks
git mind view suggestions    # low-confidence edges for review

# Compose with lenses
git mind view roadmap:incomplete          # only unfinished nodes
git mind view roadmap:incomplete:frontier # unfinished with no dependents (ready to start)
git mind view roadmap:critical-path       # longest dependency chain
git mind view backlog:blocked             # tasks that are stuck
git mind view backlog:parallel            # tasks with no mutual dependencies (run concurrently)
```

Available lenses: `incomplete`, `frontier`, `critical-path`, `blocked`, `parallel`

### Import from YAML or Markdown

Seed the graph from structured files. Import is atomic and idempotent — safe to re-run.

```bash
# YAML import
git mind import graph.yaml
git mind import graph.yaml --dry-run     # preview without writing
git mind import graph.yaml --validate    # schema check only

# Pull relationships out of Markdown frontmatter
git mind import --from-markdown "docs/**/*.md"
```

YAML format:

```yaml
version: 1
nodes:
  - id: spec:auth
    props: { title: "Authentication spec", status: in-progress }
  - id: task:login
    props: { status: todo }
edges:
  - source: task:login
    target: spec:auth
    type: implements
    confidence: 0.9
```

### Time-travel

The graph has a full causal history. Travel to any commit and see what you knew then.

```bash
git mind at main~100      # graph as it was 100 commits ago
git mind at v1.0.0        # graph at a tagged release
git mind diff HEAD~10..HEAD --prefix task   # what changed in tasks
```

### Graph health

```bash
git mind doctor           # detect dangling edges, orphan nodes, duplicates
git mind doctor --fix     # auto-fix safe issues
git mind status           # node/edge counts, blocked items, low-confidence edges
```

### AI-assisted curation

```bash
git mind suggest --ai     # LLM-suggested edges based on recent commits
git mind review           # interactive accept/reject/adjust loop
git mind review --batch   # non-interactive batch review
```

### Multi-repo federation

```bash
# Cross-repo edges
git mind link spec:auth repo:org/other-repo:crate:jwt --type depends-on

# Merge another repo's graph into this one
git mind merge --from ../other-repo
```

### Commit directives

Add edges directly from commit messages — no CLI required:

```
feat: add JWT validation

gm: task:login implements spec:auth
gm: task:login depends-on task:setup
```

### JSON output for scripting

Every command supports `--json`. All outputs are schema-validated.

```bash
git mind status --json | jq '.totals.nodes'
git mind view roadmap:incomplete --json | jq '.nodes[]'
git mind diff HEAD~5..HEAD --json
```

---

## Built on git-warp

git-mind is built on [`@git-stunts/git-warp`](https://github.com/nicktomlin/git-warp) — a multi-writer CRDT graph that lives in Git. This gives git-mind properties that no external database can match:

- **Causal history** — every write has a Lamport tick. `git mind at` isn't a snapshot lookup — it materializes the exact causal state of the graph at that point in history.
- **Conflict-free merging** — multiple writers, deterministic convergence. No merge conflicts in your knowledge graph, ever.
- **Branch and merge** — explore experimental relationships in a branch. Merge what works, discard what doesn't. Same workflow as your code.
- **Zero infrastructure** — the graph is stored in Git's object store. Invisible to normal workflows. No files cluttering your working tree.

---

## Documentation

| Document | Purpose |
|----------|---------|
| [GUIDE.md](GUIDE.md) | Complete user guide — install, tutorial, all features |
| [GRAPH_SCHEMA.md](GRAPH_SCHEMA.md) | Schema spec — node IDs, edge types, import format |
| [ROADMAP.md](ROADMAP.md) | Milestone history and upcoming work |
| [CONTRIBUTING.md](CONTRIBUTING.md) | How to contribute |
| [CHANGELOG.md](CHANGELOG.md) | Release history |
| [docs/VISION_NORTH_STAR.md](docs/VISION_NORTH_STAR.md) | Long-term direction |
| [docs/adr/](docs/adr/) | Architecture decision records |

---

## Status

**v3.2.0** — M1–M10 shipped. 461 tests. JSON schema contracts on all `--json` outputs.

The API is stabilising. Breaking changes will be noted in [CHANGELOG.md](CHANGELOG.md).

---

## License

[Apache-2.0](LICENSE)

Copyright 2025–2026 James Ross
