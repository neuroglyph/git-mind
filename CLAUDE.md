# Claude Project Overview & Handoff Document

## 🚨 CRITICAL INSTRUCTIONS FOR CLAUDE

1. **NEVER make git commits** - Always pause at commit boundaries
2. **ALWAYS suggest commit messages** - Let the user commit
3. **UPDATE this document** as you work - This is your persistent memory
4. **READ this first** before doing anything in the project

## 📍 Project Status (2025-06-27)

**git-mind**: A Git-native tool for versioning your understanding of code
- **Current warnings**: 235 (reduced from 243 after fixing result types and parameter names)
- **Original warnings**: 11,951 → 410 → 401 → 345 → 306 → 279 → 243 → 235
- **Build system**: meson/ninja (Makefiles deprecated)
- **Architecture**: Quarantined legacy code in `src/`, clean new code in `core/`
- **Test separation**: Test backends isolated in `core/tests/backends/`
- **CRITICAL FIX**: Fixed result type names (missing _t suffix) that was causing build failure

## 🎯 Current TODO List

### Immediate Tasks
- [x] Fix global const warnings in backend.c - COMPLETE
- [x] Evict test backend from production code - COMPLETE
- [x] Fix ALL naming convention violations - COMPLETE
- [x] Fix ALL missing includes - COMPLETE
- [x] Remove ALL NOLINT comments - COMPLETE
- [x] Fix result type names (missing _t suffix) - COMPLETE (CRITICAL BUILD FIX)
- [ ] Fix remaining global variables (g_siphash_key in id.c)
- [ ] Reduce function complexity below thresholds
- [ ] Fix security warnings (unchecked return values)
- [ ] Fix parameter naming violations (3+ character names)

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
- [x] Fixed missing result.h, stddef.h, and stdarg.h includes (39 warnings eliminated)
- [x] Migrated id.c from pthread to C11 threads (call_once)
- [x] Converted test_id_thread_safety.c to C11 threads
- [x] Rewrote memory.h macros as inline functions
- [x] Achieved ZERO include-cleaner warnings (66 warnings eliminated)
- [x] Fixed ALL global constant naming (UPPER_CASE with GM_ prefix)
- [x] Fixed ALL parameter/variable naming (3+ characters)
- [x] Removed ALL NOLINT comments
- [x] Fixed GM_RESULT_DEF macro to emit _t suffixed typedefs
- [x] Bulk renamed all Result types to comply with naming convention

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

### Clang-tidy Warnings (243 total)
1. **Global variables** (~20): Non-const globals that should be const
2. **Missing includes**: ZERO! All include-cleaner warnings eliminated
3. **Naming violations**: ZERO! All identifier naming warnings eliminated
4. **Function complexity** (~8): Exceeds size/cognitive thresholds
5. **Security** (~20): Unchecked return values (cert-err33-c)
6. **Parameter naming** (~30): Short names like 'a', 'b', 'id'
7. **Misc** (~13): Recursion warnings, cognitive complexity

### CI Status
- **c_core.yml**: Uses baseline_count.txt (243)
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
- Fixed ALL include-cleaner warnings - ZERO remaining!
- Fixed ALL naming violations - ZERO remaining!
- Migrated to C11 threads (pthread → threads.h)
- Converted memory.h macros to inline functions
- Fixed GM_RESULT_DEF macro and bulk renamed all Result types
- Removed ALL NOLINT comments - no suppressions allowed!
- Reduced warnings from 401 → 243 (158 warnings eliminated total!)
- Learned: Don't blame the tool - fix the code! The tool is always right!

**Current Session** (2025-06-27):
- CRITICAL: Fixed build-breaking issue - result types missing _t suffix in function declarations
- Updated all header files to use correct type names (gm_result_void_t, gm_result_string_t, etc.)
- Fixed corresponding .c files to match
- Warning count temporarily increased to 248 as build fix revealed hidden warnings
- Fixed parameter naming violations (s → state, a/b → view1/view2, r1/r2/r3 → rand1/rand2/rand3, etc.)
- Reduced warnings from 243 → 235 (8 warnings eliminated!)

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