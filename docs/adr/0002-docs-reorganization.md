---
title: ADR 0002 — Documentation Reorganization and Planning Structure
description: Establish stable docs hubs, add indexes, and improve cross-linking.
audience: [contributors]
domain: [project]
tags: [ADR, docs]
status: accepted
last_updated: 2025-09-15
---

# ADR 0002: Documentation Reorganization and Planning Structure

Table of Contents

- [Status](#status)
- [Context](#context)
- [Decision](#decision)
- [Consequences](#consequences)
- [Related Documents](#related-documents)

## Status

- Accepted

## Context

The `docs/` directory had grown organically with mixed audiences and no single entry point, making it hard to pause and resume work. Planning content (roadmaps, releases, sprints) did not exist in a cohesive place, and cross-document links were inconsistent.

## Decision

1. Establish a non-destructive structure with clear hubs:
   - Planning: `docs/planning/` (roadmap, releases, milestones, sprints)
   - Architecture: `docs/architecture/` (unchanged, now linked by a central index)
   - Specs, Testing, Deployment, Requirements, Risk, Charter in dedicated folders
2. Add anchored, GitHub-friendly TOCs to major documents and use relative Markdown links across documents.
3. Create `docs/README.md` as the canonical index/map.
4. Introduce a `.legacy/` archive for historical planning/notes, and a `.trash/` quarantine for removed or superseded docs (with rationale in `.trash/README.md`).

## Consequences

- Pros: Easier navigation; project can be paused/resumed without loss of context; consistent links; planning is first-class alongside architecture/specs.
- Cons: Some existing deep links may need updates if/when files are moved into new folders. The `.trash/` process adds a minor maintenance step.

## Related Documents

- [docs/README.md](../README.md)
- [Product Roadmap](../planning/Product_Roadmap.md)
- [Release Plans](../planning/Release_Plans.md)
- [System Architecture](../architecture/System_Architecture.md)
- [.legacy/README.md](../../.legacy/README.md)
