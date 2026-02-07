# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.0-alpha.0] - 2026-02-07

Complete rewrite from C23 to Node.js on `@git-stunts/git-warp`.

### Added

- **Graph module** — Initialize, load, and checkpoint WARP graphs in any Git repo
- **Edge CRUD** — Create, query, and remove typed edges with confidence scores
- **8 edge types** — `implements`, `augments`, `relates-to`, `blocks`, `belongs-to`, `consumed-by`, `depends-on`, `documents`
- **Observer views** — Filtered projections: `roadmap`, `architecture`, `backlog`, `suggestions`
- **Commit hooks** — Parse directives (`IMPLEMENTS:`, `AUGMENTS:`, etc.) from commit messages to auto-create edges
- **CLI** — `git mind init`, `link`, `list`, `view`, `suggest` (stub), `review` (stub)
- **25 tests** — Full coverage of graph, edges, views, and hooks modules

### Changed

- **License** — Changed from MIND-UCAL-1.0 to Apache-2.0
- **Architecture** — Replaced C23 hexagonal architecture (libgit2, libsodium, CBOR, Roaring Bitmaps) with a thin Node.js wrapper around git-warp's CRDT graph engine

### Removed

- All C23 source code (archived on `archive/c23-final` branch)
- Meson build system
- Docker-based CI/CD
- All C-specific documentation

[2.0.0-alpha.0]: https://github.com/neuroglyph/git-mind/releases/tag/v2.0.0-alpha.0
