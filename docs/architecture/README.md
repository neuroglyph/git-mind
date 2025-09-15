<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->
<!-- © 2025 J. Kirby Ross / Neuroglyph Collective -->
---
title: Architecture Index
description: Entry point to git-mind’s architecture docs and core systems.
audience: [developers]
domain: [architecture]
tags: [architecture, journal, cache, attribution]
status: stable
last_updated: 2025-09-15
---

# git-mind Architecture Documentation

Table of Contents

- [Core Systems](#core-systems)
- [Design Decisions](#design-decisions)
- [Diagrams](#diagrams)
- [Integration Points](#integration-points)

This directory contains the technical architecture documentation for git-mind.

## Core Systems

### [Attribution System](attribution-system.md)

The foundation for human-AI collaboration. Tracks who created each edge (human or AI) and enables filtered views, review workflows, and consensus building.

- [Integration Guide](attribution-integration-guide.md) - How to add attribution to existing code
- [Use Cases](attribution-use-cases.md) - Real-world collaboration scenarios

### [Journal System](journal-architecture-pivot.md)

Git-native storage using commits as the source of truth for semantic edges.

### [Bitmap Cache](bitmap-cache-design.md)

High-performance cache layer using Roaring Bitmaps for O(log N) query performance.

### [AUGMENTS System](augments-system.md)

Automatic tracking of file evolution through Git commits.

## Design Decisions

### [Journal Architecture Pivot](journal-architecture-pivot.md)

The radical rebuild that transformed git-mind from orphan refs to journal commits.

## Diagrams

Key architectural diagrams are embedded throughout the documentation using Mermaid:

- Attribution system class hierarchy
- Data flow sequences
- Human-AI collaboration workflows
- Cache sharding structure

## Integration Points

1. __CLI Commands__ - How commands interact with core systems
2. __Web UI__ - Real-time updates via WebSocket
3. __MCP Integration__ - AI tools for semantic graph manipulation
4. __Git Hooks__ - Automatic edge creation on commits

---

_"Architecture is about the important stuff. Whatever that is."_ - Ralph Johnson

In git-mind, the important stuff is enabling humans and AI to build understanding together.

## Target Architecture (Repo Layout)

```
git-mind/
├── core/               # C23 library: include/, src/, tests/
├── include/            # Umbrella API: gitmind.h and public headers
├── apps/               # CLI and hooks
├── tests/              # E2E/integration
├── tools/              # Dev tooling, quality scripts
└── docs/               # Architecture and project documentation
```

Key decisions:

- Lean dependencies (libgit2, CRoaring via CI image)
- Journal‑first storage, rebuildable cache
- OID‑first APIs and compare via `git_oid_cmp`
