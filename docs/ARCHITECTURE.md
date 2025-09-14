<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->
<!-- © 2025 J. Kirby Ross / Neuroglyph Collective -->

# git-mind Architecture

This document serves as the master index for all architectural documentation and decisions.

## 🏗️ Current Architecture Status

**Status**: Library‑first architecture in progress; CLI and tooling evolving  
**Focus**: Journal (edges‑as‑commits), cache (Roaring Bitmaps), CLI (link/list/cache‑rebuild)  
**Target**: Single‑header C library with optional frontends (CLI, MCP, hooks)

## 📚 Architecture Documents

### Core Design
- [Modular Restructure Plan](docs/architecture/MODULAR_RESTRUCTURE_PLAN.md) — plan for modular system and single‑header direction
- Memory Architecture (TODO) — custom allocators and memory pooling design
- Single Header Design (TODO) — how the amalgamated `gitmind.h` works

### API Design
- Core Library API (TODO) — Public API for `gitmind.h`
- MCP Service (Optional, TODO) — Local‑only tools integration for co‑thought

### Implementation Details
- CBOR Encoding Format — see [PRD](docs/PRDs/PRD-git-mind-semantics-time-travel-prototype.md)
- Git Storage Model — see [Journal Architecture Pivot](docs/architecture/journal-architecture-pivot.md)
- Cache Architecture — see [Bitmap Cache](docs/architecture/bitmap-cache-design.md)

## 🎯 Target Architecture

```
git-mind/
├── core/               # Library (toward single-header gitmind.h)
├── apps/
│   ├── cli/            # Command-line interface
│   ├── mcp/ (opt)      # Local MPC/co‑thought service
│   └── hooks/          # Git hooks
└── quality/            # Code quality configs
```

## 🔧 Key Design Decisions

1. **Single‑header library (direction)** — core functionality under one include
2. **Lean dependencies** — libgit2 + CRoaring for cache (vendor or pin in CI)
3. **Custom allocators** — pool allocation for performance (planned)
4. **CBOR encoding** — compact binary format for edges
5. **Git as database** — serverless; no external storage needed

## 🚀 Migration Status

| Component    | Status        | Notes |
|--------------|---------------|-------|
| Core Library | In progress   | Journal/cache/CLI prioritized |
| Edge Module  | Migrated      | Continues to evolve with semantics changes |
| CBOR Module  | Migrated      | Extended with `type_name`/`lane_name` |
| Attribution  | Integrated    | Filters and lanes to be expanded |
| CLI App      | In progress   | link/list/cache‑rebuild stabilized first |

### Migration Philosophy
See [MIGRATION_PHILOSOPHY](docs/architecture/MIGRATION_PHILOSOPHY.md). During migration, we prioritize correctness and clarity; post‑migration we focus effort on real issues and developer velocity.

## 📋 Quick Links

- [GitHub Issues](https://github.com/neuroglyph/git-mind/issues)
- [RFC: Modular Architecture (#116)](https://github.com/neuroglyph/git-mind/issues/116)
- [Migration Guide](docs/architecture/MODULAR_RESTRUCTURE_PLAN.md#migration-strategy)

---

For detailed information on any component, see the linked documents above.
