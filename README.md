# git-mind 🧠

> **⚠️ MAJOR REWRITE IN PROGRESS**: This project is undergoing a complete architectural overhaul. We've reduced warnings from 11,951 to 410 and are migrating to meson/ninja build system. Not production-ready. See [The Great Migration](#the-great-migration) below.

## How We Gate Quality

1. **Push whatever you want** - No pre-push hooks
2. **GitHub Actions runs full CI** - Meson, clang-tidy, sanitizers, coverage  
3. **If CI fails, fix & force-push** - Simple and deterministic

![git-mind](./assets/images/wide-logo.png)

## What is git-mind?

A Git-native tool for versioning your understanding of code, not just the code itself.

```bash
# Create semantic links between files
git mind link design.md src/main.rs --type implements

# Time-travel through your mental model
git checkout HEAD~10
git mind list  # See what connections you understood 10 commits ago
```

**The key insight**: Your understanding of how code connects evolves over time. git-mind makes that evolution visible and queryable.

## Why?

You know that `UserService.java` implements the design in `user-flows.md`. Six months later, that connection is lost. New team members can't find the relationships. The context dies with time.

git-mind stores these relationships as Git objects, giving them the same permanence and version history as your code.

## 🚧 The Great Migration

**Current Status**: Major progress! Warnings reduced from 11,951 to 410

We're completing the architectural rebuild with modern build tools:

- **Phase 1** (Complete): Built solid foundations in `core/`
  - Error handling with Result types ✅
  - Strong typedefs for domain concepts ✅
  - Security primitives ✅
  - Migrated to meson/ninja build system ✅
  - Reduced warnings from 11,951 to 410 ✅

- **Phase 2**: Migrate existing functionality
  - Reimplement each module using new foundations
  - Comprehensive test coverage
  - Delete legacy code as we go

- **Phase 3**: New features on solid base
  - Web UI with graph visualization
  - AI collaboration features
  - Cross-repository relationships

See [docs/enforcer/ROADMAP_TO_REFACTORING.md](docs/enforcer/ROADMAP_TO_REFACTORING.md) for the full plan.

## Current State

### What Works ✅
- Basic CLI commands (link, list, traverse)
- Git-native storage
- Time-travel through git checkout
- Tests pass (on legacy code)

### What's Being Fixed 🔧
- ~~11,951~~ 410 compiler warnings (96% reduction!)
- Proper error handling with Result types
- Memory safety improvements
- Security-first design
- Modern build system (meson/ninja)

### What We're Building 🏗️
- Clean modular architecture
- Proper error handling (Result types)
- Memory-safe operations
- Security-first design
- Zero warnings on all code

## For Developers

If you want to help or just watch the migration:

```bash
git clone https://github.com/neuroglyph/git-mind
cd git-mind

# Build with meson/ninja
meson setup build
ninja -C build
ninja -C build test

# Don't touch src/ - it's quarantined
# New development happens in core/
```

**Important**: 
- The pre-commit hook blocks changes to `src/` (legacy code)
- All new development happens in `core/` with zero warnings
- We're following "touch it = rewrite it completely" rule

## The Vision (Unchanged)

git-mind will become infrastructure for versioning understanding:

- **For individuals**: Never lose context on why code connects
- **For teams**: Share mental models that evolve with code
- **For AI**: Collaborate on building knowledge graphs
- **For the future**: Make understanding as permanent as code

## Timeline

- **Now - Week 4**: Build foundations (error handling, types, security)
- **Week 5-8**: Migrate core functionality with zero warnings
- **Week 9+**: New features on solid foundation
- **Target**: Production-ready by Q2 2025

## Contributing

Want to help? We need:
- C developers who care about code quality
- Feedback on the architecture
- Ideas for the future

But please understand: we're in heavy construction mode. Things will break and change rapidly.

## License

This project uses a custom ethical license: `LicenseRef-MIND-UCAL-1.0`

---

**Bottom line**: git-mind is a tool for versioning your understanding of code. We're rebuilding it properly. Come back in a few months for the good stuff, or join us in eliminating the last 410 warnings. 🎯

For the full vision and technical details, see:
- [ARCHITECTURE.md](ARCHITECTURE.md) - Technical architecture
- [docs/enforcer/](docs/enforcer/) - Migration documentation
- [CLAUDE_vs_THE_ENFORCER.md](CLAUDE_vs_THE_ENFORCER.md) - The story of how this started
