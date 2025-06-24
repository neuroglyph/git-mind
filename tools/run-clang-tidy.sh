#!/bin/bash
# Run clang-tidy EXACTLY the same way everywhere
# This ensures CI and manual runs produce identical results

set -e

# Add LLVM to PATH if on Mac
export PATH="/opt/homebrew/opt/llvm/bin:$PATH"

# Ensure we have a build directory with compile_commands.json
if [ ! -f "build-push/compile_commands.json" ]; then
    echo "Setting up build directory..."
    meson setup build-push >/dev/null 2>&1 || true
    ninja -C build-push
fi

echo "Running clang-tidy..."

# Run clang-tidy EXACTLY like CI
clang-tidy -quiet -p build-push --config-file=quality/.clang-tidy \
    $(git ls-files 'core/**/*.c' 'core/**/*.h') \
    | tee /tmp/clang-tidy-report-full.txt || true

# Filter to just project warnings (handles all path formats)
grep -E "^\.\./core/|^/.*core/|^core/" /tmp/clang-tidy-report-full.txt > /tmp/clang-tidy-report.txt || true

echo "Clang-tidy complete. Results in /tmp/clang-tidy-report.txt"
echo "Found $(wc -l < /tmp/clang-tidy-report.txt) warnings"

# If requested, create baseline
if [ "$1" = "--create-baseline" ]; then
    echo "Creating baseline..."
    python3 tools/parse_warnings.py create-baseline /tmp/clang-tidy-report.txt tools/baseline_grouped.txt
fi

# If requested, check against baseline
if [ "$1" = "--check" ]; then
    echo "Checking against baseline..."
    python3 tools/parse_warnings.py check /tmp/clang-tidy-report.txt tools/baseline_grouped.txt
fi