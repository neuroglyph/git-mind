# git-mind

> Version your thoughts. Branch your ideas. Merge understanding.

**git-mind** turns any Git repository into a semantic knowledge graph. Link files, concepts, and ideas with typed, confidence-scored edges — all stored in Git itself. No servers. No databases. Just `git push`.

## What it does

```bash
# Initialize a knowledge graph in your repo
git mind init

# Link files with semantic relationships
git mind link src/auth.js docs/auth-spec.md --type implements
git mind link module:cache module:db --type depends-on

# See all connections
git mind list

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
npx git-mind link README.md docs/spec.md --type documents
npx git-mind list
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

- **Conflict-free merging** — multiple writers, deterministic convergence
- **Time-travel** — check out any commit, get the graph from that moment
- **Git-native storage** — invisible to normal workflows, inherits Git's integrity
- **Branch and merge** — try experimental connections in a branch, merge what works

## Documentation

- [GUIDE.md](GUIDE.md) — Complete user guide (install, tutorial, features, technical deep dives)
- [CONTRIBUTING.md](CONTRIBUTING.md) — How to contribute
- [CHANGELOG.md](CHANGELOG.md) — Release history

## Status

**v2.0.0-alpha.0** — Core features work. API may evolve.

## License

[Apache-2.0](LICENSE)

Copyright 2025-2026 James Ross
