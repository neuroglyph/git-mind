<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->
<!-- © 2025 J. Kirby Ross / Neuroglyph Collective -->

# git-mind Architecture

This document serves as the master index for all architectural documentation and decisions.

## 🏗️ Current Architecture Status

**Status**: Transitioning from monolithic CLI to modular library-first architecture  
**Progress**: 75/11,951 code quality issues fixed  
**Target**: Single-header C library with multiple frontends

## 📚 Architecture Documents

### Core Design
- [Modular Restructure Plan](docs/architecture/MODULAR_RESTRUCTURE_PLAN.md) - The master plan for transforming git-mind into a modular system
- Memory Architecture (TODO) - Custom allocators and memory pooling design
- Single Header Design (TODO) - How the amalgamated `gitmind.h` works

### API Design
- Core Library API (TODO) - Public API for `gitmind.h`
- MCP Protocol Design (TODO) - Model Context Protocol integration
- Web API Design (TODO) - REST/GraphQL endpoints

### Implementation Details
- CBOR Encoding Format (TODO) - Edge serialization specification
- Git Storage Model (TODO) - How edges are stored in Git
- Cache Architecture (TODO) - Bitmap cache and query optimization

## 🎯 Target Architecture

```
git-mind/
├── core/              # Single-header library (gitmind.h)
├── apps/
│   ├── cli/          # Command-line interface
│   ├── mcp/          # MCP protocol server
│   ├── web/          # Web UI daemon
│   └── hooks/        # Git hooks
└── quality/          # Code quality configs
```

## 🔧 Key Design Decisions

1. **Single-header library** - Core functionality in one `#include`
2. **Zero dependencies** - Only libgit2 required
3. **Custom allocators** - Pool allocation for performance
4. **CBOR encoding** - Compact binary format for edges
5. **Git as database** - No external storage needed

## 🚀 Migration Status

| Component | Status | Quality |
|-----------|--------|---------|
| Core Library | 🟡 In Progress | - |
| Edge Module | ✅ **DONE** | 0 warnings (was 12) |
| CBOR Module | ✅ **DONE** | 0 warnings (was 63) |
| Attribution | 🔴 Not Started | ~50 warnings |
| CLI App | 🔴 Not Started | TBD warnings |

### Migration Philosophy
**During Migration**: PEDANTIC MODE - Every warning must die!  
**After Migration**: "Linus ain't mad" - Focus on real issues only

## 📋 Quick Links

- [GitHub Issues](https://github.com/neuroglyph/git-mind/issues)
- [RFC: Modular Architecture (#116)](https://github.com/neuroglyph/git-mind/issues/116)
- [Migration Guide](docs/architecture/MODULAR_RESTRUCTURE_PLAN.md#migration-strategy)

---

For detailed information on any component, see the linked documents above.