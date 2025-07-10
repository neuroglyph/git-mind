<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->
<!-- © 2025 J. Kirby Ross / Neuroglyph Collective -->

# `git-mind` 🧠

_A Git-integrated knowledge graph for tracking relationships between code artifacts._

__🚧 MAJOR ARCHITECTURAL MIGRATION IN PROGRESS 🏗️__

We’re transforming `git-mind` from a monolithic CLI into a clean, embeddable C library with zero warnings under extreme compiler strictness. 

__Progress__: Core library 90% complete | CLI separation 0% | New apps 0%

See [The Great Migration](#the-great-migration) below for details.

---

## What is `git-mind`?

`git-mind` captures and versions the connections between your code, documentation, and design artifacts. It stores these relationships as Git objects, making them as permanent and trackable as your code.

```bash
# Link a design document to its implementation
git mind link docs/auth-flow.md src/auth.c --type implements

# Find all code that implements a specific design
git mind traverse docs/auth-flow.md --direction forward

# See how your understanding evolved
git checkout v1.0
git mind list  # View connections from that point in time
```

---

## Installation

### From Source

```bash
git clone https://github.com/neuroglyph/git-mind
cd git-mind
meson setup build
ninja -C build
sudo ninja -C build install
```

### Requirements

- Git 2.28+
- C23-compliant compiler (GCC 12+ or Clang 16+)
- Meson build system
- Ninja
- libsodium
- libgit2 (for Git object manipulation)

## Core Concepts

- __Links__: Directed relationships between files (e.g., “implements”, “documents”, “tests”)   
- __Traversal__: Navigate your codebase through semantic connections, not just file paths   
- __Time Travel__: Your knowledge graph evolves with your code - checkout any commit to see that era’s understanding   

## Usage

### Creating Links

```bash
# Basic link creation
git mind link <source> <target> --type <relationship>

# Common relationship types
git mind link README.md src/main.c --type documents
git mind link test_auth.c src/auth.c --type tests
git mind link design.md src/module/ --type implements
```

### Exploring Connections

```bash
# List all links
git mind list

# Find what a file connects to
git mind traverse src/auth.c

# Find what connects to a file
git mind traverse src/auth.c --direction backward

# Filter by relationship type
git mind list --type tests
```

### Advanced Features

```bash
# Export knowledge graph
git mind export --format dot > graph.dot

# Check link integrity
git mind verify

# Remove broken links
git mind prune
```

## Architecture

git-mind stores relationship data in `.git/refs/minds/` using Git’s object database. Each link is a Git object containing:

- Source and target paths
- Relationship type
- Creation timestamp
- Optional metadata

This design ensures links are:

- Version controlled
- Distributed with the repository
- Preserved through Git operations
- Queryable at any point in history

----

## Development

### Building

```bash
# Standard build
meson setup build
ninja -C build

# Debug build
meson setup build_debug -Dbuildtype=debug
ninja -C build_debug

# Run tests
ninja -C build test
```

### Code Organization

```
core/           # Core library implementation
├── include/    # Public headers
├── src/        # Implementation
└── tests/      # Unit tests

cli/            # Command-line interface
tools/          # Development utilities
docs/           # Documentation
```

### Contributing

See [CONTRIBUTING]('./CONTRIBUTING.md)

----

## 🚧 The Great Migration

### Why We’re Migrating

We started with **11,951 compiler warnings** - a technical debt mountain that was holding us back. Rather than patch over problems, we're rebuilding git-mind with extreme code quality standards:

🎯 **ZERO WARNINGS ACHIEVED** on core library modules!  
🏆 **GNU CRY GAUNTLET** - survived the strictest compiler settings  
⚡ **C23 FUTURE-PROOF** - leveraging the latest C standard

- __Zero Warnings Policy__: Every module in `core/` must have ZERO clang-tidy warnings
- __GNU CRY GAUNTLET__: Our CI runs the strictest compiler settings that “make GNU developers cry”
- __C23 Standard__: Leveraging the latest C standard for better type safety and modern features
- __Library-First Design__: Transform from monolithic CLI to embeddable C library
- __Single-Header Core__: Ultimate goal is a single `#include <gitmind.h>` for all functionality

### Migration Milestones

#### 🎯 Milestone 1: "Core Complete" (~90% done)

__Status__: 🟩🟩🟩🟩🟩🟩🟩🟩🟩⬜  
__Target__: ~~January 2025~~ **AHEAD OF SCHEDULE**

✅ __Completed (Warning-Free™)__

- ✅ Error handling, Result types
- ✅ Type system (paths, strings, IDs, ULID)  
- ✅ Crypto backend (pluggable SHA256, random)
- ✅ Time operations (mockable for testing)
- ✅ CBOR encoding/decoding 
- ✅ UTF-8 validation
- ✅ I/O operations (secure file handling)
- ✅ Edge system (graph operations, relationships)
- ✅ Attribution (authorship tracking)  
- ✅ Journal system (Git object storage)
- ✅ Cache system (query optimization, roaring bitmaps)

🔄 __Final Core Components__

- Utility consolidation (cleanup legacy duplicates) - **Only module remaining!**

#### 🎯 Milestone 2: “CLI: Oh My!” (0% done)

__Status__: ⬜⬜⬜⬜⬜⬜⬜⬜⬜⬜  
__Target__: February 2025

📋 __Application Separation Tasks__

- Extract CLI from src/cli/ to apps/cli/
- Extract Git hooks to apps/hooks/
- Create proper libgit2 integration layer
- Implement against libgitmind API
- Add modern CLI features (colors, progress bars)

#### 🎯 Milestone 3: “Beyond CLI” (0% done)

__Status__: ⬜⬜⬜⬜⬜⬜⬜⬜⬜⬜  
__Target__: March 2025

🚀 __New Applications__

- MCP server for AI integration
- Web UI daemon
- Git hooks as separate binaries
- Language bindings (Python, Rust)

### Current Components Status

```
core/               # ✅ Foundation library (90% complete)
├── include/        # ✅ Public API headers 
├── src/            # ✅ Core implementations (zero warnings!)
│   ├── cache/      # ✅ Query optimization (roaring bitmaps)
│   ├── edge/       # ✅ Graph operations
│   ├── journal/    # ✅ Git object storage
│   └── ...         # ✅ All other core modules
└── tests/          # ✅ Comprehensive unit tests

src/                # 🔄 Legacy code being migrated
├── util/           # 📋 Legacy utilities (FINAL MODULE)
├── cli/            # 📋 Command-line interface
└── hooks/          # 📋 Git hooks (post-commit, etc.)
```

### Target Architecture

```
git-mind/
├── libgitmind/     # Single-header library
│   ├── core/       # Foundation (types, crypto, I/O)
│   ├── graph/      # Graph operations (edges, attribution)
│   └── storage/    # Git persistence (journal, cache)
├── apps/
│   ├── cli/        # Command-line interface
│   ├── hooks/      # Git hooks (separate binaries)
│   ├── mcp/        # Model Context Protocol server
│   └── web/        # Web UI daemon
└── bindings/       # Language bindings (Python, Rust, etc.)
```

## 🚀 Beyond Migration: Coming Soon

### 🧠 Semantic Intelligence (Q2 2025)

- __AI-Powered Discovery__: Automatically detect and suggest relationships between code artifacts
- __Natural Language Queries__: “Show me all code that implements authentication”
- __Intelligent Refactoring__: Track concept migrations across architectural changes

### 🌐 Distributed Knowledge (Q3 2025)

- __Cross-Repository Links__: Connect knowledge across project boundaries
- __Federated Graphs__: Share and merge knowledge graphs between teams
- __Knowledge Synchronization__: Keep understanding in sync across distributed teams

## Support

- Issues: [GitHub Issues](https://github.com/neuroglyph/git-mind/issues)
- Discussions: [GitHub Discussions](https://github.com/neuroglyph/git-mind/discussions)
- Documentation: [docs/](docs/)

---

## License

Licensed under `LicenseRef-MIND-UCAL-1.0`. See [LICENSE](./LICENSE) file for details.

© 2025 – J. Kirby Ross • <https://github.com/flyingrobots>
