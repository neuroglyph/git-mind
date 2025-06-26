# Tools Directory Map of Contents (MOC)

## Overview
This directory contains build, CI, and development tools for the git-mind project.

## Files

### 📊 `baseline_count.txt`
- **Purpose**: Tracks the current warning count baseline
- **Current Value**: 353
- **Usage**: Used by CI to ensure no new warnings are introduced
- **Updated**: Automatically by `check-warning-fix.sh` when warnings decrease

### 🐳 `docker-clang-tidy.sh`
- **Purpose**: Runs clang-tidy in a Docker container matching CI environment
- **Why Docker**: Ensures identical results between local (macOS/Windows) and CI (Linux)
- **Usage**: `./tools/docker-clang-tidy.sh`
- **Output**: 
  - `clang-tidy-report-full.txt` - Complete output
  - `clang-tidy-report.txt` - Filtered to project warnings only

### 🎉 `check-warning-fix.sh`
- **Purpose**: Verifies warning fixes and celebrates progress
- **Features**:
  - Optionally checks if specific warning pattern is eliminated
  - Compares current count to baseline
  - Automatically updates baseline when warnings decrease
  - Provides celebratory messages:
    - 🤩 "From X to Y warnings! Well done." (when reduced)
    - 👍 "No new warnings." (when unchanged)
    - 😱🚨 "RUH-ROH. New errors detected!" (when increased)
- **Usage**: 
  - `./tools/check-warning-fix.sh` - Check overall count
  - `./tools/check-warning-fix.sh "pattern"` - Check specific warning

### 📝 `count_warnings.py`
- **Purpose**: Simple Python script to count warnings in clang-tidy output
- **Usage**: Called by other scripts, not used directly
- **Function**: Counts lines containing "warning:" or "error:" 

### 🔍 `parse_warnings.py`
- **Purpose**: More sophisticated warning parser (deprecated)
- **Note**: Replaced by simpler `count_warnings.py`

## Workflow

1. **Check Current State**
   ```bash
   ./tools/docker-clang-tidy.sh
   ```

2. **Fix Warnings**
   - Make code changes
   - Use specific headers instead of umbrella includes
   - Add missing includes for IWYU compliance
   - Fix naming violations

3. **Verify Fix**
   ```bash
   ./tools/check-warning-fix.sh "optional-pattern"
   ```

4. **Celebrate Progress**
   - Script automatically updates baseline
   - Shows motivational messages
   - Tracks warning reduction over time

## Docker Image

The Docker image (`gitmind-ci:latest`) is built on first run and includes:
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