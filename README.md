<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->
<!-- © 2025 J. Kirby Ross / Neuroglyph Collective -->

[![CI](https://github.com/neuroglyph/git-mind/actions/workflows/ci.yml/badge.svg)](https://github.com/neuroglyph/git-mind/actions/workflows/ci.yml)

[![MIND-UCAL](https://img.shields.io/badge/license-MIND--UCAL-orange)](license/LICENSE.md)

# git-mind

**Time-travel through your understanding of any codebase.**

A blazing-fast CLI tool written in pure C with minimal dependencies.

```bash
git mind link design.md src/main.rs --type implements
git checkout HEAD~10
git mind list  # See what you thought 10 commits ago
```

An idea that seems simple at first, but the more you use it, the more you realize what it enables.

---

## The Problem

You're deep in a complex codebase. You know `UserService.java` implements the design in `user-flows.md`, but six months later, that connection is lost. New team members can't find the relationships. The context dies with time.

Traditional documentation goes stale. Comments are local. Wikis live outside your repo.

**What if your understanding could evolve with your code?**

## The Solution

`git-mind` stores semantic relationships as versioned Git objects. The connections between your files have the same permanence as the files themselves.

```bash
git mind init
git mind link design.md src/main.rs --type implements
git mind link README.md docs/api.md --type references
git mind list  # See all relationships
```

The key insight: **you can time-travel through your mental model**.

```bash
git checkout HEAD~20  # Go back 20 commits
git mind list         # See what you thought back then
git checkout main     # Return to present
```

Your understanding has history, just like your code.

---

## How This Project Unfolds

**What begins as a CLI utility becomes a mirror for your thinking — and eventually, a medium for shared intelligence.**

Different people discover different aspects:

| Level | You See | The "Aha!" |
|-------|---------|------------|
| 📱 **Personal** | *"Smart assistant that remembers how files connect"* | "I don't have to remember everything!" |
| 👥 **Teams** | *"Share context, not just content"* | "We're sharing understanding!" |
| ⚙️ **Technical** | *"Git objects as a graph database — zero dependencies"* | "Wait... Git can do THAT?" |
| 🏗️ **Strategic** | *"Relationships as infrastructure"* | "Understanding becomes code!" |

The tool doesn't change. Your perception of what it enables does.

---

## Quick Start

```bash
# Build from source
git clone https://github.com/neuroglyph/git-mind
cd git-mind && make build
export PATH="$PATH:$(pwd)/build"

# In any Git repo
git mind init
git mind link README.md src/main.c --type implements
git mind status

# The time-travel moment
git mind list           # Current understanding
git checkout HEAD~5
git mind list           # What you thought 5 commits ago
git checkout main       # Back to present
```

## Core Commands

```bash
git mind init                                # Initialize semantic linking
git mind link <source> <target> --type <TYPE>   # Create relationships  
git mind list [--from <file>]               # Query your graph
git mind traverse <file> --depth <N>        # Explore connections
git mind unlink <source> <target>           # Remove relationships
git mind check [--fix]                      # Maintain integrity
git mind status                             # Repository overview
```

Common link types: `implements`, `references`, `depends_on`, `tests`, `documents`, `inspires`

---

## What Makes This Different

**Other tools:** Store snapshots of knowledge  
**git-mind:** Versions the evolution of understanding

- **Obsidian/Notion**: Rich UI, but knowledge lives outside your code
- **Wiki systems**: Separate from development, goes stale quickly
- **Git Notes**: Built-in but no semantic types or query interface  
- **Documentation**: Static, doesn't capture evolving relationships

**git-mind is the first tool designed for how developers actually think:** relationships that evolve with code, context that survives across time and teams.

---

## Performance & Architecture

Built for speed and simplicity:

```
Startup time:       <1ms      (faster than your thoughts)
Memory usage:       ~1MB      (less than a browser tab)
Query performance:  O(log N)  (Roaring Bitmap cache layer)
Dependencies:       libgit2, croaring
```

**Storage:** Two-layer architecture for blazing performance:
- **Journal Layer**: Semantic links as CBOR-encoded Git commits (source of truth)
- **Cache Layer**: Roaring Bitmap indices sharded by SHA prefix (O(log N) queries)

```
IMPLEMENTS: design.md -> src/main.rs  # ts:1736637876
REFERENCES: README.md -> docs/api.md  # ts:1736637890
```

Human-readable journal, lightning-fast cache. Your data stays yours.

---

## Installation

**Quick install (coming soon):**
```bash
# Installation script in development
# For now, build from source
```

**From source:**
```bash
git clone https://github.com/neuroglyph/git-mind
cd git-mind
make build  # Builds in Docker
```

**Verify:**
```bash
git mind version
git mind --help
```

---

## Current Status

**Working today:**
- [x] Core CLI functionality (all commands above)
- [x] Git-native storage with full versioning
- [x] Time-travel through understanding via Git checkout
- [x] Graph traversal and integrity checking
- [x] Roaring Bitmap cache for O(log N) query performance
- [x] Cross-platform builds (Mac, Linux, Windows WSL)

**Coming soon:**
- [ ] Visual evolution timeline (`git mind evolution`)
- [ ] Web interface for graph visualization  
- [ ] AI integration (persistent memory across conversations)
- [ ] Cross-repository relationship discovery

**The vision:** This is infrastructure for augmented cognition. The CLI you see today is the foundation for new forms of human-AI collaboration on complex problems.

---

## Use Cases

**Individual developers:**
- Never lose context on architectural decisions
- Navigate complex codebases by relationship, not just directory structure
- Onboard your future self to past reasoning

**Teams:**
- Share mental models alongside code  
- Instant context for new team members
- Preserve institutional knowledge through transitions

**Complex projects:**
- Track how system understanding evolved over years
- Build on reasoning from previous developers
- Maintain context through major refactors

---

## Contributing

Written in pure C for maximum performance and portability. All development happens in Docker for consistency.

```bash
make dev     # Development environment
make test    # Full test suite  
make build   # Production binary
```

See [CONTRIBUTING.md](project/community/CONTRIBUTING.md) for details.

---

## The Realization

**Git repositories are already graphs. We're just making the connections visible.**

Your `.git` directory contains the complete history of your code. Now it can contain the complete history of your understanding of that code.

This project unfolds. **Try it on your repos. See what emerges.**

---

## What This Enables

The more you use it, the more you realize:

- **Time-aware context**: Your reasoning has history, just like your code
- **Shared understanding**: Teams can literally see each other's mental models
- **Persistent knowledge**: Context that survives refactors, transitions, and time

**This isn't just better documentation. It's the beginning of something larger.**

But today, it's a CLI that helps you remember why you connected two files.

**Start there. See where it leads.**

## License

### SPDX Notice:

> This project uses a custom ethical license: `LicenseRef-MIND-UCAL-1.0`
> Not yet registered with SPDX. Compliance required for all use. See [LICENSE.md](LICENSE.md) for full terms.
