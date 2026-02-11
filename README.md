# git-mind

> Version your thoughts. Branch your ideas. Merge understanding.

**git-mind** turns any Git repository into a semantic knowledge graph. Link files, concepts, and ideas with typed, confidence-scored edges — all stored in Git itself. No servers. No databases. Just `git push`.

Because the graph lives *in* Git, it evolves with your code. Check out last month's commit and see what you understood then. Check out today's and see how your understanding has grown. Your knowledge has a history — git-mind makes it visible.

## Watch your understanding evolve

```bash
# What implemented the auth spec six months ago?
$ git checkout main~200
$ git mind list --type implements --target docs/auth-spec.md
  src/basic_auth.js --[implements]--> docs/auth-spec.md (100%)

# What implements it now?
$ git checkout main
$ git mind list --type implements --target docs/auth-spec.md
  src/oauth2.js --[implements]--> docs/auth-spec.md (100%)
  src/jwt_handler.js --[implements]--> docs/auth-spec.md (100%)
  src/basic_auth.js --[implements]--> docs/auth-spec.md (100%)

# Your code changed. Your understanding changed with it.
```

Try an idea in a branch. If it works, merge it — graph and all. If it doesn't, delete the branch. Your knowledge graph supports the same workflow your code does.

## What it does

```bash
# Initialize a knowledge graph in your repo
git mind init

# Link files with semantic relationships
git mind link file:src/auth.js spec:auth --type implements
git mind link module:cache module:db --type depends-on

# Import a graph from YAML
git mind import graph.yaml

# See all connections
git mind list

# Query nodes
git mind nodes --prefix task

# Check graph health
git mind status

# View filtered projections
git mind view architecture
git mind view roadmap
```

## Quick start

```bash
# Clone and install
git clone https://github.com/neuroglyph/git-mind.git
cd git-mind && npm install

# Use in any Git repo
cd /path/to/your/repo
npx git-mind init
npx git-mind link file:README.md doc:spec --type documents
npx git-mind list
npx git-mind status
```

See [GUIDE.md](GUIDE.md) for a complete walkthrough.

## Edge types

| Type | Meaning |
|------|---------|
| `implements` | Source implements the target spec/design |
| `augments` | Source extends or enhances the target |
| `relates-to` | General semantic relationship |
| `blocks` | Source blocks progress on target |
| `belongs-to` | Source is a member/child of target |
| `consumed-by` | Source is consumed/used by target |
| `depends-on` | Source depends on target |
| `documents` | Source documents/describes target |

Each edge carries a **confidence score** (0.0 to 1.0). Human-created edges default to 1.0. AI-suggested edges start low and get promoted through review.

## Built on git-warp

git-mind is a thin layer on [`@git-stunts/git-warp`](https://github.com/nicktomlin/git-warp) — a multi-writer CRDT graph database that lives in Git. This gives git-mind:

- **Time-travel** — check out any commit, see the graph as it was at that moment. Watch connections appear, change, and deepen over time.
- **Conflict-free merging** — multiple writers, deterministic convergence. No merge conflicts in your knowledge graph, ever.
- **Branch and merge** — try experimental connections in a branch, merge what works, discard what doesn't. Same workflow as your code.
- **Git-native storage** — invisible to normal workflows. No files in your working tree, no databases to run. It's just Git.

## Documentation

- [GUIDE.md](GUIDE.md) — Complete user guide (install, tutorial, features, technical deep dives)
- [GRAPH_SCHEMA.md](GRAPH_SCHEMA.md) — Authoritative graph schema specification (node IDs, edge types, import format)
- [CONTRIBUTING.md](CONTRIBUTING.md) — How to contribute
- [CHANGELOG.md](CHANGELOG.md) — Release history

## Status

**v2.0.0-alpha.0** — Core features work. API may evolve.

## License

[Apache-2.0](LICENSE)

Copyright 2025-2026 James Ross
