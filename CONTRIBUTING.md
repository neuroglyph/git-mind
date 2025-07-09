<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->
<!-- © 2025 J. Kirby Ross / Neuroglyph Collective -->

# Contributing to git-mind

Thank you for your interest in contributing to git-mind! This project aims to create a new medium for human-AI collaboration in understanding code.

> **🤖 AI Assistants**: Also read [CLAUDE.md](CLAUDE.md) for specific operational orders and workflow guidelines.  
> **👤 Human Contributors**: CLAUDE.md contains useful practices that apply to all contributors!

## Getting Started

1. **Read the documentation**
   - [README.md](README.md) - Project overview
   - [CLAUDE.md](CLAUDE.md) - Development standards and practices
   - [TASKLIST.md](TASKLIST.md) - Current development status

2. **Set up development environment**
   ```bash
   git clone https://github.com/neuroglyph/git-mind
   cd git-mind
   make dev  # Opens Docker development shell
   ```

3. **Run tests**
   ```bash
   make test  # Runs all tests in Docker
   ```

4. **Test across multiple compilers**
   ```bash
   # The DEVIL'S COMPILER GAUNTLET 666 🔥
   ./tools/gauntlet/run-gauntlet.sh
   
   # Quick infrastructure test
   ./tools/gauntlet/test-gauntlet.sh
   ```

## Development Principles

1. **Test-Driven Development (TDD)** - Write tests first
2. **SOLID Principles** - Keep code clean and maintainable
3. **KISS** - Simple solutions over complex ones
4. **YAGNI** - Don't add features until needed
5. **Dependency Injection** - For testability

## Code Standards

- **NO MAGIC NUMBERS** - Use #defined constants
- **NO MAGIC STRINGS** - Use #defined string constants
- **Silent by default** - Output only on error or with --verbose
- **Single Responsibility** - Each function does one thing
- **Always use Docker** - Never build on host
- **C23 compatibility** - Must pass the DEVIL'S COMPILER GAUNTLET 666

## Making Changes

### Before You Start (Planning Phase)
1. **Find or create a GitHub issue** - All work should be tracked
2. **Check README.md migration status** - See what's in progress
3. **Make a plan** - Think through your approach before coding
4. **Share your plan** - Get feedback early (via issue comments or PR draft)

### During Development (Implementation Phase)
1. **Create a feature branch** - Use descriptive names
2. **Write tests first** (TDD) - Tests define success
3. **Make micro-commits** - Small, focused changes with conventional commit messages
4. **Reference issues in commits** - e.g., "fix: resolve warnings in CBOR module #153"
5. **Keep builds green** - Fix failures immediately
6. **Drop a SITREP if stuck** - Communicate blockers early

### Before Submitting (Quality Phase)
1. **Run all tests** - `ninja -C build test`
2. **Check for warnings** - `./tools/docker-clang-tidy.sh` (must be ZERO)
3. **Run the GNU CRY GAUNTLET** - All compilers must pass
4. **Update documentation** - Keep README and docs in sync
5. **Submit pull request** - With clear description of changes

## The GNU CRY GAUNTLET 🎯

This project uses **EXTREME** compiler strictness to ensure code quality. Our CI makes GNU developers cry:

### What it tests:
- **GCC 12** (Pre-C23 - THE DEVIL) - Tests backwards compatibility
- **GCC 13** (Partial C23) - Early C23 support
- **GCC 14** (Better C23) - Improved C23 support
- **Clang 18** (Partial C23) - LLVM C23 support
- **Clang 19** (Better C23) - Improved LLVM C23
- **Clang 20** (Latest C23) - Cutting-edge C23 support

### How it works:
1. Runs **6 compilers in parallel** (not waiting 5evah!)
2. Each compiler builds and tests your code independently
3. Logs saved to `build-<compiler>.log` files
4. Results in `result-<compiler>.txt` files

### Usage:
```bash
# Full GAUNTLET (all 6 compilers)
./tools/gauntlet/run-gauntlet.sh

# Quick test (subset)
./tools/gauntlet/test-gauntlet.sh

# CI integration
# The GAUNTLET runs automatically on all PRs
```

### Results:
- ✅ **ALL PASS** = Your code is **UNHOLY and BULLETPROOF**
- ❌ **ANY FAIL** = Some compilers made you cry (check logs)

**If your code survives the DEVIL'S COMPILER GAUNTLET 666, it's ready for production.**

## Communication Guidelines

### SITREPs (Situation Reports)
When working on complex tasks or hitting blockers:
- **What**: Current task and status
- **Where**: Files/modules being modified
- **Blockers**: Any issues preventing progress
- **Next**: Proposed next steps
- **Options**: Alternative approaches if stuck

### Commit Messages
Follow conventional commit format:
```
type(scope): description

Longer explanation if needed.

Fixes #123
```

Types: `feat`, `fix`, `docs`, `style`, `refactor`, `test`, `chore`

### Pull Request Descriptions
Include:
- Summary of changes
- Link to related issue(s)
- Test plan
- Any breaking changes

## Attribution System

If you're an AI assistant helping with development:
- Set proper attribution when creating edges
- Use confidence scores appropriately
- Follow guidelines in [CLAUDE.md](CLAUDE.md)
- Include co-authorship in commits

## License

By contributing, you agree that your contributions will be licensed under the MIND-UCLA v1.0 License.

## Questions?

Open an issue on GitHub or check the documentation in `/docs/`.

## Additional Resources

- [CLAUDE.md](CLAUDE.md) - AI-specific operational orders
- [README.md](README.md) - Project overview and migration status
- [GitHub Issues](https://github.com/neuroglyph/git-mind/issues) - Current work items

---

*"Keep code in files, truth in commits, speed in shards."*