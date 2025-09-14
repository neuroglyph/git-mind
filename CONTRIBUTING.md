<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->
<!-- © 2025 J. Kirby Ross / Neuroglyph Collective -->

# Contributing to git-mind

Thank you for your interest in contributing to git-mind! This project aims to create a new medium for human-AI collaboration in understanding code.

> __🤖 AI Assistants__: Also read [CLAUDE.md](CLAUDE.md) for specific operational orders and workflow guidelines.  
> __👤 Human Contributors__: CLAUDE.md contains useful practices that apply to all contributors!

## Getting Started

1. __Read the documentation__
   - [README.md](README.md) - Project overview
   - [CLAUDE.md](CLAUDE.md) - Development standards and practices
   - [TASKLIST.md](TASKLIST.md) - Current development status

2. __Set up development environment__

   ```bash
   git clone https://github.com/neuroglyph/git-mind
   cd git-mind
   make dev  # Opens Docker development shell
   ```

3. __Run tests__

   ```bash
   make test  # Runs all tests in Docker
   ```

4. __Test across multiple compilers__

   ```bash
   # The DEVIL'S COMPILER GAUNTLET 666 🔥
   ./tools/gauntlet/run-gauntlet.sh
   
   # Quick infrastructure test
   ./tools/gauntlet/test-gauntlet.sh
   ```

## Development Principles

1. __Test-Driven Development (TDD)__ - Write tests first
2. __SOLID Principles__ - Keep code clean and maintainable
3. __KISS__ - Simple solutions over complex ones
4. __YAGNI__ - Don't add features until needed
5. __Dependency Injection__ - For testability

## Code Standards

- __NO MAGIC NUMBERS__ - Use #defined constants
- __NO MAGIC STRINGS__ - Use #defined string constants
- __Silent by default__ - Output only on error or with --verbose
- __Single Responsibility__ - Each function does one thing
- __Always use Docker__ - Never build on host
- __C23 compatibility__ - Must pass the DEVIL'S COMPILER GAUNTLET 666

## Making Changes

### Before You Start (Planning Phase)

1. __Find or create a GitHub issue__ - All work should be tracked
2. __Check README.md migration status__ - See what's in progress
3. __Make a plan__ - Think through your approach before coding
4. __Share your plan__ - Get feedback early (via issue comments or PR draft)

### During Development (Implementation Phase)

1. __Create a feature branch__ - Use descriptive names
2. __Write tests first__ (TDD) - Tests define success
3. __Make micro-commits__ - Small, focused changes with conventional commit messages
4. __Reference issues in commits__ - e.g., "fix: resolve warnings in CBOR module #153"
5. __Keep builds green__ - Fix failures immediately
6. __Drop a SITREP if stuck__ - Communicate blockers early

### Before Submitting (Quality Phase)

1. __Run all tests__ - `ninja -C build test`
2. __Check for warnings__ - `./tools/docker-clang-tidy.sh` (must be ZERO)
3. __Run the GNU CRY GAUNTLET__ - All compilers must pass
4. __Update documentation__ - Keep README and docs in sync
5. __Submit pull request__ - With clear description of changes

## The GNU CRY GAUNTLET 🎯

This project uses __EXTREME__ compiler strictness to ensure code quality. Our CI makes GNU developers cry:

### What it tests

- __GCC 12__ (Pre-C23 - THE DEVIL) - Tests backwards compatibility
- __GCC 13__ (Partial C23) - Early C23 support
- __GCC 14__ (Better C23) - Improved C23 support
- __Clang 18__ (Partial C23) - LLVM C23 support
- __Clang 19__ (Better C23) - Improved LLVM C23
- __Clang 20__ (Latest C23) - Cutting-edge C23 support

### How it works

1. Runs __6 compilers in parallel__ (not waiting 5evah!)
2. Each compiler builds and tests your code independently
3. Logs saved to `build-<compiler>.log` files
4. Results in `result-<compiler>.txt` files

### Usage

```bash
# Full GAUNTLET (all 6 compilers)
./tools/gauntlet/run-gauntlet.sh

# Quick test (subset)
./tools/gauntlet/test-gauntlet.sh

# CI integration
# The GAUNTLET runs automatically on all PRs
```

### Results

- ✅ __ALL PASS__ = Your code is __UNHOLY and BULLETPROOF__
- ❌ __ANY FAIL__ = Some compilers made you cry (check logs)

__If your code survives the DEVIL'S COMPILER GAUNTLET 666, it's ready for production.__

## Communication Guidelines

### SITREPs (Situation Reports)

When working on complex tasks or hitting blockers:

- __What__: Current task and status
- __Where__: Files/modules being modified
- __Blockers__: Any issues preventing progress
- __Next__: Proposed next steps
- __Options__: Alternative approaches if stuck

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

## Questions

Open an issue on GitHub or check the documentation in `/docs/`.

## Additional Resources

- [CLAUDE.md](CLAUDE.md) - AI-specific operational orders
- [README.md](README.md) - Project overview and migration status
- [GitHub Issues](https://github.com/neuroglyph/git-mind/issues) - Current work items

---

_"Keep code in files, truth in commits, speed in shards."_
