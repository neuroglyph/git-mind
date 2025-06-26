# Claude Project Overview & Handoff Document

## 🚨 CRITICAL INSTRUCTIONS FOR CLAUDE

1. **NEVER make git commits** - Always pause at commit boundaries
2. **ALWAYS suggest commit messages** - Let the user commit
3. **UPDATE this document** as you work - This is your persistent memory
4. **READ this first** before doing anything in the project

## 📍 Project Status (2025-06-27)

**git-mind**: A Git-native tool for versioning your understanding of code
- **Current warnings**: 345 (reduced from 401 → 388 → 375 → 363 → 353 → 345)
- **Original warnings**: 11,951 → 410 → 401 → 345
- **Build system**: meson/ninja (Makefiles deprecated)
- **Architecture**: Quarantined legacy code in `src/`, clean new code in `core/`
- **Test separation**: Test backends isolated in `core/tests/backends/`

## 🎯 Current TODO List

### Immediate Tasks
- [x] Fix global const warnings in backend.c - COMPLETE
- [x] Evict test backend from production code - COMPLETE
- [ ] Fix remaining global variables (g_siphash_key in id.c)
- [ ] Fix naming convention violations for `gm_result_*` typedefs
- [ ] Fix missing result.h includes in other files
- [ ] Reduce function complexity below thresholds
- [ ] Fix security warnings (unchecked return values)

### Next Up
- [ ] Reduce warnings from 345 → 0
- [ ] Complete migration of remaining modules from `src/` to `core/`
- [ ] Add Result types to remaining functions that can fail
- [ ] Split path.c (1,156 lines) into smaller modules

### Completed Recently
- [x] Removed pre-push hooks entirely (PR #135 & #140)
- [x] Disabled macro-to-enum checks for C code
- [x] Created Docker-based clang-tidy runner
- [x] Replaced all sodium.h with specific headers (26 warnings eliminated)
- [x] Added all missing stdint.h includes (22 warnings eliminated)
- [x] Created check-warning-fix.sh script with celebratory messages
- [x] Evicted test backend from production (test code now in core/tests/backends/)
- [x] Made crypto backends const (API now returns const pointers)

## 🛠️ Development Workflow

### Environment Setup
```bash
# Build
meson setup build
ninja -C build

# Test
ninja -C build test

# Check warnings (run in Docker to match CI)
./tools/docker-clang-tidy.sh
```

### Warning Fix Procedure
1. Start from main: `git checkout main && git pull`
2. Run clang-tidy to see current state: `./tools/docker-clang-tidy.sh`
3. Pick a warning type to fix: `grep -E "warning:|error:" clang-tidy-report.txt | head -20`
4. Create appropriately named branch: `git checkout -b fix/specific-warning-type`
5. Fix the warnings in the code
6. Verify fix: `./tools/check-warning-fix.sh "optional-warning-pattern"`
7. See celebration message! 🤩 👍 😱🚨
8. Stage files INCLUDING baseline: `git add <files> tools/baseline_count.txt`
9. Suggest commit message and wait for user
10. Push and wait for merge

### Key Commands
- **Build**: `ninja -C build`
- **Test**: `ninja -C build test`
- **Clean**: `rm -rf build`
- **Lint in Docker**: `./tools/docker-clang-tidy.sh`
- **Check fix & update baseline**: `./tools/check-warning-fix.sh [optional-pattern]`

### Git Workflow
1. `git checkout main && git pull`
2. **RUN CLANG-TIDY FIRST** to see current warnings
3. `git checkout -b fix/descriptive-name` based on warnings found
4. Make changes to eliminate specific warnings
5. Run `./tools/check-warning-fix.sh` to verify fix
6. **PAUSE** - Suggest commit message
7. User commits
8. Push (git config push.autoSetupRemote enabled)
9. CI validates

## 📂 Project Structure

```
git-mind/
├── core/           # New clean code (345 warnings)
│   ├── src/        # Implementation
│   ├── include/    # Headers
│   └── tests/      # Unit tests
├── src/            # Legacy code (11,951 warnings - DO NOT TOUCH)
├── tools/          # Build and CI tools
│   ├── baseline_count.txt      # Current: 345
│   ├── docker-clang-tidy.sh    # Run linting in CI environment
│   └── check-warning-fix.sh    # Verify fixes & celebrate progress
├── core/tests/backends/        # Test-only crypto backends
│   └── test_backend.c          # Deterministic test implementation
└── quality/        # Linting configs
    └── .clang-tidy # Strict checks (macro-to-enum disabled)
```

## 🚧 Known Issues

### Clang-tidy Warnings (345 total)
1. **Global variables** (~20): Non-const globals that should be const
2. **Missing includes** (~150): IWYU compliance (result.h, stdarg.h, etc.)
3. **Naming violations** (~112): `gm_result_*` typedef case style
4. **Function complexity** (~8): Exceeds size/cognitive thresholds
5. **Security** (~20): Unchecked return values (cert-err33-c)
6. **Parameter naming** (~30): Short names like 'a', 'b', 'id'
7. **Misc** (~13): Recursion warnings, cognitive complexity

### CI Status
- **c_core.yml**: Uses baseline_count.txt (345)
- **core-quality.yml**: Runs full quality checks
- Coverage: 83.1% line, 54.1% branch (needs 70% branch)

## 📋 Code Standards

### REQUIRED for ALL Code
- Zero magic numbers (define all constants)
- Zero TODOs (implement or delete)
- Zero placeholders (real implementations only)
- Result types for fallible functions
- Direct includes only (IWYU)
- TDD: Test first, then implement

### Forbidden
- Touching `src/` directory (legacy quarantine)
- Local builds outside Docker for CI checks
- Commits without user approval
- Pre-push hooks (removed in PR #135)

## 🔄 Handoff Notes

**Last Session Summary** (2025-06-27):
- Eliminated all sodium.h warnings by using specific headers
- Fixed all stdint.h missing include warnings 
- Created check-warning-fix.sh script with celebratory emojis
- Evicted test backend from production code per Central Command
- Made crypto backends const with API returning const pointers
- Reduced warnings from 401 → 345 (56 warnings eliminated!)
- Learned: Test code belongs in tests/, not production

**Next Steps**:
1. Fix g_siphash_key globals in id.c (use atomic operations)
2. Add missing result.h includes in remaining files
3. Fix parameter naming violations (3+ character names)
4. Continue systematic warning reduction to zero

## 💡 Tips for Next Claude

1. **Always run linting in Docker**: `./tools/docker-clang-tidy.sh`
2. **Never trust local clang-tidy**: macOS vs Linux = different results
3. **Use check-warning-fix.sh**: It updates baseline & celebrates!
4. **Check CI logs**: `gh run list --workflow=c_core.yml`
5. **This is your memory**: Update TODO list after every task
6. **Run clang-tidy FIRST**: See warnings before naming branches
7. **Some warnings are false positives**: But fix the code, not the tool
8. **Update baseline with EVERY commit**: Track progress accurately

---
*Remember: You're fixing 20 years of technical debt. Every warning removed is a victory.*