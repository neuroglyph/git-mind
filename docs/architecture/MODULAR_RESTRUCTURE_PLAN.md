<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->
<!-- © 2025 J. Kirby Ross / Neuroglyph Collective -->
---
title: Modular Architecture Restructure Plan
description: Plan to evolve toward a modular, library-first system with a single-header API.
audience: [developers]
domain: [architecture]
tags: [modular, library, headers]
status: draft
last_updated: 2025-09-15
---

# Modular Architecture Restructure Plan

Table of Contents

- [Executive Summary](#executive-summary)
- [Goals](#goals)
- [Plan](#plan)
- [Milestones](#milestones)
- [Compatibility](#compatibility)

## Executive Summary

Transform git-mind from a monolithic CLI application into a modular, library-first architecture with a true single-header core library and clean separation of concerns.

__Key Goals:__

1. Create a single-header `gitmind.h` core library
2. Fix 11,000+ code quality issues during migration
3. Enable multiple frontends (CLI, MCP, Web)
4. Maintain backward compatibility where possible

## Current Problems

### 1. Architecture Issues

- __No clear separation__ between core logic and UI
- __Not a single-header library__ despite original goal (9+ headers)
- __Tight coupling__ between components
- __Difficult to embed__ in other applications

### 2. Code Quality Issues (NOW RESOLVED FOR CORE)

- __Started with 11,951 clang-tidy warnings__
- __Now: ZERO warnings in migrated core modules__
- Achieved through systematic migration:
  - Functions refactored to reasonable size
  - All magic numbers → named constants
  - Consistent naming conventions
  - Proper initialization everywhere
  - Self-contained headers

### 3. Maintenance Challenges

- Hard to test components in isolation
- Changes ripple across entire codebase
- Can't reuse core logic in new contexts

## Proposed Architecture

```
git-mind/
├── core/                      # Single-header library
│   ├── include/
│   │   └── gitmind.h         # THE single header (amalgamated)
│   ├── core/src/             # Source files (post-migration)
│   │   ├── edge/
│   │   ├── cbor/
│   │   ├── attribution/
│   │   └── util/
│   ├── tests/
│   └── tools/
│       └── amalgamate.py     # Combines sources into single header
│
├── apps/                      # All applications using core
│   ├── cli/                   # Command-line interface
│   ├── mcp/                   # MCP server  
│   ├── web/                   # Web daemon
│   └── hooks/                 # Git hooks
│
├── docs/
│   ├── architecture/          # ADRs and design docs
│   └── migration/            # Migration guides
│
└── quality/                   # Code quality configuration
    ├── .clang-tidy           # Strict for new code
    └── .clang-tidy.legacy    # Relaxed for migration
```

## Core Library Design

### What Goes in Core

- Edge/link data structures
- CBOR encoding/decoding  
- Attribution system
- Basic Git operations
- Pure algorithms (no I/O)

### What Stays Out

- Command-line parsing
- Output formatting
- Network operations
- File I/O (except via dependency injection)
- UI of any kind

### Single Header Strategy

```c
/* gitmind.h - Single header git-mind library */
#ifndef GITMIND_H
#define GITMIND_H

#ifdef GITMIND_IMPLEMENTATION
  /* Implementation details */
  #include "core/src/edge/edge.c"
  #include "core/src/cbor/cbor.c"
  /* ... */
#else
  /* Public API only */
#endif

#endif /* GITMIND_H */
```

## Migration Strategy ("3 Birds, 1 Stone")

### Phase 1: Core Extraction (Weeks 1-4)

For each module (edge, cbor, attribution):

1. Move to `core/src/`
2. Fix ALL clang-tidy warnings
3. Split functions exceeding size limits
4. Replace magic values with constants
5. Add comprehensive tests
6. Update migration tracker

### Phase 2: Application Separation (Weeks 5-6)

1. Move CLI to `apps/cli/`
2. Update to use only `gitmind.h`
3. Fix quality issues in CLI code
4. Separate UI logic from business logic

### Phase 3: New Applications (Week 7+)

1. Create `apps/mcp/` with clean implementation
2. All new code follows strict quality standards
3. Share only through core library

## Quality Enforcement

### Regression Prevention

```bash
# quality/snapshots/{file}.quality
function-size: 0 warnings (was: 23)
magic-numbers: 0 warnings (was: 45)
naming: 0 warnings (was: 67)
```

### Automated Testing

- Generate regression tests for each fix
- Quality snapshots prevent backsliding
- CI enforces improvements

### Progressive Standards

```yaml
# .github/workflows/quality-gate.yml
- core/**: Must pass strict .clang-tidy
- apps/**: Can use .clang-tidy.legacy during migration
- New files: Must be 100% clean
```

## File-by-File Migration Example

Taking `edge.c` as an example:

### Before Migration

```c
// core/src/edge/edge.c
void gm_edge_create(...) {
    // 45 line function with magic numbers
    if (type > 10) { ... }  // Magic number!
}
```

### After Migration  

```c
// core/src/edge/edge.c
#define MAX_EDGE_TYPE 10

static int validate_edge_type(int type) {
    return type <= MAX_EDGE_TYPE;
}

int gm_edge_create(...) {
    // Now 12 lines, no magic numbers
    if (!validate_edge_type(type)) {
        return GM_ERROR_INVALID_TYPE;
    }
}
```

## Success Metrics

### Week 1

- [ ] Core directory structure created
- [ ] First module (edge) migrated and clean
- [ ] Automated quality tracking in place

### Week 4  

- [ ] All core modules migrated
- [ ] Single header amalgamation working
- [ ] 50% reduction in total warnings

### Week 8

- [ ] CLI fully separated and using core
- [ ] MCP server implemented
- [ ] 100% of core code passes strict quality checks

## Backwards Compatibility

### For Users

- `git-mind` CLI commands unchanged
- Git hooks continue working
- Existing `.gitmind/` data compatible

### For Developers  

- Old `#include "gitmind/*.h"` → `#include "gitmind.h"`
- Function signatures preserved
- Behavior unchanged (only quality improved)

## Open Questions

1. __Version during migration?__
   - Stay on 0.x to indicate instability?
   - Jump to 2.0 when complete?

2. __Breaking changes acceptable?__
   - Can we rename poorly-named functions?
   - Standardize return codes?

3. __Migration timeline?__
   - Aggressive 8-week plan realistic?
   - Do incrementally over months?

## Next Steps

1. __Create proof-of-concept__: Migrate edge module
2. __Set up quality automation__: Snapshot and tracking tools  
3. __Get feedback__: Open discussion on approach
4. __Begin systematic migration__: Module by module

---

_This is a living document. Updates will be tracked in git history._
