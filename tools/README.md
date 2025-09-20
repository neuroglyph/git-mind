# Tools Directory Map of Contents (MOC)

## Overview

This directory contains build, CI, and development tools for the git-mind project.

## Files

### 📊 `baseline_count.txt`

- __Purpose__: Tracks the current warning count baseline
- __Current Value__: 353
- __Usage__: Used by CI to ensure no new warnings are introduced
- __Updated__: Automatically by `check-warning-fix.sh` when warnings decrease

### 🐳 `docker-clang-tidy.sh`

- __Purpose__: Runs clang-tidy in a Docker container matching CI environment
- __Why Docker__: Ensures identical results between local (macOS/Windows) and CI (Linux)
- __Usage__: `./tools/docker-clang-tidy.sh`
- __Output__:
  - `clang-tidy-report-full.txt` - Complete output
  - `clang-tidy-report.txt` - Filtered to project warnings only
- __Images__: Tags under `${GITMIND_NS:-gitmind}/ci:<version>` (labeled `com.gitmind.project=git-mind`), built locally from `.ci/Dockerfile` when needed.

### 🎉 `check-warning-fix.sh`

- __Purpose__: Verifies warning fixes and celebrates progress
- __Features__:
  - Optionally checks if specific warning pattern is eliminated
  - Compares current count to baseline
  - Automatically updates baseline when warnings decrease
  - Provides celebratory messages:
    - 🤩 "From X to Y warnings! Well done." (when reduced)
    - 👍 "No new warnings." (when unchanged)
    - 😱🚨 "RUH-ROH. New errors detected!" (when increased)
- __Usage__:
  - `./tools/check-warning-fix.sh` - Check overall count
  - `./tools/check-warning-fix.sh "pattern"` - Check specific warning

### 📝 `count_warnings.py`

- __Purpose__: Simple Python script to count warnings in clang-tidy output
- __Usage__: Called by other scripts, not used directly
- __Function__: Counts lines containing "warning:" or "error:"

### 🔍 `parse_warnings.py`

- __Purpose__: More sophisticated warning parser (deprecated)
- __Note__: Replaced by simpler `count_warnings.py`

### 🧹 `docker-clean.sh`

- __Purpose__: Removes only git-mind containers/images and prunes builder cache
- __Usage__: `./tools/docker-clean.sh`
- __Scope__: Targets images labeled `com.gitmind.project=git-mind` and repositories prefixed with `${GITMIND_NS:-gitmind}/...`

## Workflow

1. __Check Current State__

   ```bash
   ./tools/docker-clang-tidy.sh
   ```

2. __Fix Warnings__
   - Make code changes
   - Use specific headers instead of umbrella includes
   - Add missing includes for IWYU compliance
   - Fix naming violations

3. __Verify Fix__

   ```bash
   ./tools/check-warning-fix.sh "optional-pattern"
   ```

4. __Celebrate Progress__
   - Script automatically updates baseline
   - Shows motivational messages
   - Tracks warning reduction over time

## Docker Image

The Docker image (`${GITMIND_NS:-gitmind}/ci:clang-20`) is built locally from `.ci/Dockerfile` on first use and includes:

- Ubuntu 22.04 (matching GitHub Actions)
- LLVM/Clang 20
- libsodium-dev
- meson/ninja
- All tools needed for CI parity

## Tips

- Always run linting in Docker for CI-accurate results
- The baseline should decrease with every warning-fixing commit
- Use `check-warning-fix.sh` instead of manually updating baseline
- Celebrate every warning eliminated! 🎯

### Image naming and cleanup

- All project images use a consistent namespace: `${GITMIND_NS:-gitmind}`
- CI image: `${GITMIND_NS}/ci:clang-20`
- Gauntlet images: `${GITMIND_NS}/gauntlet:<compiler>` (e.g., `gcc-13`, `clang-20`)
- Clean up safely: `./tools/docker-clean.sh`
