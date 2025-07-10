#!/bin/bash
# Run clang-tidy EXACTLY the same way as CI
# This ensures CI and manual runs produce identical results

set -e

# Add LLVM to PATH if on Mac
export PATH="/opt/homebrew/opt/llvm/bin:$PATH"

# Ensure we have a build directory with compile_commands.json
if [ ! -f "build/compile_commands.json" ]; then
    echo "Setting up build directory..."
    CC=clang meson setup build >/dev/null 2>&1 || true
    ninja -C build
fi

echo "Running clang-tidy..."

# Run clang-tidy EXACTLY like CI (excluding test files)
clang-tidy -quiet -p . --config-file=quality/.clang-tidy \
    $(git ls-files 'core/**/*.c' 'core/**/*.h' | grep -v -E '(test_|_test\.|/tests/)') \
    | tee clang-tidy-report-full.txt || true

# Filter to just project warnings (handles all path formats)
grep -E "^/.*core/|^core/" clang-tidy-report-full.txt > clang-tidy-report.txt || true

echo "Clang-tidy complete. Results in clang-tidy-report.txt"
echo "Found $(wc -l < clang-tidy-report.txt) warnings"

# Check against baseline (same as CI)
python3 tools/count_warnings.py check clang-tidy-report.txt tools/baseline_count.txt