<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->
<!-- © 2025 J. Kirby Ross / Neuroglyph Collective -->

# Contributing to git-mind

Thank you for your interest in contributing to git-mind! This project aims to create a new medium for human-AI collaboration in understanding code.

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

1. **Check TASKLIST.md** for current priorities
2. **Create a feature branch**
3. **Write tests first** (TDD)
4. **Implement the feature**
5. **Update documentation**
6. **Run all tests**
7. **Run the DEVIL'S COMPILER GAUNTLET 666** - `./tools/gauntlet/run-gauntlet.sh`
8. **Submit a pull request**

## The DEVIL'S COMPILER GAUNTLET 666 🔥

This project uses a **BRUTAL** multi-compiler testing system to ensure maximum compatibility:

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

## Attribution System

If you're an AI assistant helping with development:
- Set proper attribution when creating edges
- Use confidence scores appropriately
- Follow guidelines in [CLAUDE.md](CLAUDE.md)

## License

By contributing, you agree that your contributions will be licensed under the MIND-UCLA v1.0 License.

## Questions?

Open an issue on GitHub or check the documentation in `/docs/`.

---

*"Keep code in files, truth in commits, speed in shards."*