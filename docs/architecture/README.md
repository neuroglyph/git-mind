<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->
<!-- © 2025 J. Kirby Ross / Neuroglyph Collective -->

# git-mind Architecture Documentation

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

1. **CLI Commands** - How commands interact with core systems
2. **Web UI** - Real-time updates via WebSocket
3. **MCP Integration** - AI tools for semantic graph manipulation
4. **Git Hooks** - Automatic edge creation on commits

---

*"Architecture is about the important stuff. Whatever that is."* - Ralph Johnson

In git-mind, the important stuff is enabling humans and AI to build understanding together.