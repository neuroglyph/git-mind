---
title: SITREP
description: Situation reports and CI notes.
audience: [contributors]
domain: [quality]
tags: [sitrep]
status: archive
last_updated: 2025-09-15
---

# CI Infrastructure SITREP - 2025-06-24

## Executive Summary

__Overall Status__: OPERATIONAL with ISSUES  
__Threat Level__: AMBER  
__Recommendation__: Fix local environment before next push

## 1. Git Hooks Intelligence

### (i) What Do They Do

__Pre-commit Framework Configuration__:

1. __clang-format__ (v18.1.8)
   - Auto-formats C/C++ code
   - Ensures consistent code style

2. __include-what-you-use (iwyu)__
   - Validates header dependencies
   - Ensures minimal includes
   - __NOTE__: Excludes `core/` directory

3. __detect-secrets__ (v1.4.0)
   - Scans for potential secrets
   - Prevents credential leaks
   - Currently reporting false positives

__Git LFS Hooks__:

- post-checkout
- pre-push

### (ii) How Do They Work

Pre-commit hooks run automatically before each commit:

1. Scan changed files
2. Apply formatters/linters
3. Block commit if issues found
4. Can be bypassed with `--no-verify` (NOT RECOMMENDED)

### (iii) Do They Currently Pass on Main

__STATUS__: ❌ FAILING

__Test Results__:

- ❌ __clang-format__: Modified files (formatting issues)
- ❌ __iwyu__: Tool not installed locally
- ❌ __detect-secrets__: 7 false positives detected

__Critical Issues__:

1. Local environment missing `iwyu_tool`
2. False positive secrets in test vectors and constants
3. Code formatting not aligned with clang-format v18.1.8

## 2. GitHub Actions Intelligence

### (i) What Do They Do

__Active Workflows__:

1. __c_core.yml__ (C-Core Gate)
   - __Purpose__: Quality gate for PRs
   - __Checks__:
     - LLVM 20 clang-tidy with baseline enforcement
     - cppcheck static analysis
     - 60-second fuzzing test
   - __Trigger__: Pull requests only

2. __core-quality.yml__ (Core Quality Checks)
   - __Purpose__: Comprehensive quality enforcement
   - __Checks__:
     - NO TODO/FIXME/XXX comments allowed
     - clang-tidy with warnings-as-errors
     - AddressSanitizer (ASAN)
     - UndefinedBehaviorSanitizer (UBSAN)
     - MemorySanitizer (MSAN) if available
     - Coverage: 80% line, 70% branch minimum
   - __Trigger__: Push to main/dev/enforced, PRs to main

__Disabled Workflows__ (Migration Mode):

- ci.yml
- code-quality.yml
- release.yml

### (ii) How Do They Work

1. __Baseline Enforcement__ (c_core.yml):
   - Compares warnings against `tools/baseline.txt`
   - Fails if new warnings introduced
   - Uses `tools/enforce_no_new_warnings.sh`

2. __Sanitizer Suite__ (core-quality.yml):
   - Builds with different sanitizers
   - Runs full test suite under each
   - Zero-tolerance for sanitizer errors

### (iii) Do They Currently Pass on Main

__STATUS__: ✅ PASSING (last checked: main branch)

GitHub Actions are green on main branch. The two-track system effectively:

- Quarantines legacy code (11,951 warnings)
- Enforces strict quality on new code

## 3. Configuration Issues & Remediation

### Immediate Actions Required

1. __Install include-what-you-use__:

   ```bash
   # macOS
   brew install include-what-you-use
   
   # Ubuntu/Debian
   sudo apt-get install iwyu
   ```

2. __Fix Secret Detection False Positives__:
   - Add `.secrets.baseline` file
   - Or add pragma comments to test files

3. __Run Formatter__:

   ```bash
   pre-commit run clang-format --all-files
   ```

### Helper Scripts Available

- `run-all-hooks.sh` - Test all hooks locally
- `run-github-actions-locally.sh` - Simulate GH Actions with `act`

## 4. Strategic Assessment

### Strengths

- Two-track system allows progress while maintaining debt
- Comprehensive sanitizer coverage
- Baseline prevents regression
- LLVM 20 standardized across toolchain

### Vulnerabilities

- Local dev environment not matching CI
- iwyu excluding core/ (should it?)
- Secret detection needs tuning

### Recommendations

1. Fix local environment BEFORE next feature branch
2. Consider adding iwyu to core/ once stable
3. Update `.secrets.baseline` to reduce false positives
4. Document required local tools in `docs/DEV_SETUP.md`

## Mission Impact

Current CI infrastructure supports our zero-warning mission effectively. The two-track approach allows us to:

- Build clean foundations in `core/`
- Contain the 11,951-warning legacy disaster
- Prevent new technical debt

__Bottom Line__: CI is our ally in this fight, but local environment needs immediate attention.

---
_SITREP compiled by Claude on 2025-06-24_  
_Next review recommended after local environment fixes_
