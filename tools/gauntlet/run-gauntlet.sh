#!/bin/bash

# DEVIL'S COMPILER GAUNTLET 666 - Parallel C23 compiler tests
# This script runs your code through multiple compilers in parallel

set -euo pipefail

# Namespace/prefix to keep images tidy and easy to prune
GITMIND_NS=${GITMIND_NS:-gitmind}
IMAGE_PREFIX="${GITMIND_NS}/gauntlet"

echo "🔥 DEVIL'S COMPILER GAUNTLET 666 🔥"
echo "Building Docker images for 5 compilers in parallel..."

# Compilers to test
COMPILERS=("gcc-12" "gcc-13" "clang-18" "clang-19" "clang-20")

# Build all compiler images in parallel
echo "Building Docker images..."
for compiler in "${COMPILERS[@]}"; do
    echo "Building $compiler image..."
    docker build --target "$compiler" \
      -t "${IMAGE_PREFIX}:$compiler" \
      --label com.gitmind.project=git-mind \
      -f tools/gauntlet/Dockerfile.gauntlet . &
done

# Wait for all builds to complete
echo "⏳ Waiting for all Docker builds to complete..."
wait

echo ""
echo "🔫 Running GAUNTLET with 5 compilers in parallel..."
echo "  - GCC 12 (Pre-C23 - THE DEVIL)"
echo "  - GCC 13 (C23 support progression)"
echo "  - Clang 18, 19, 20 (C23 support progression)"
echo ""

# Run all compiler tests in parallel
PIDS=()
for compiler in "${COMPILERS[@]}"; do
    echo "Starting $compiler test..."
    docker run --rm --label com.gitmind.project=git-mind -v "$PWD":/workspace -w /workspace \
      "${IMAGE_PREFIX}:$compiler" > "$compiler.log" 2>&1 &
    PIDS+=($!)
done

# Wait for all tests to complete
echo "⏳ Waiting for all compiler tests to finish..."
for pid in "${PIDS[@]}"; do
    wait $pid
done

echo ""
echo "================================================"
echo "🏁 GAUNTLET RESULTS 🏁"
echo "================================================"

# Collect results
PASSED=0
FAILED=0

for compiler in "${COMPILERS[@]}"; do
    if [ -f $compiler.log ]; then
        if grep -q "PASS" $compiler.log; then
            echo "✅ $compiler: PASSED"
            PASSED=$((PASSED + 1))
        else
            echo "❌ $compiler: FAILED"
            FAILED=$((FAILED + 1))
            echo "   Check $compiler.log for details"
        fi
    else
        echo "❌ $compiler: NO_LOG"
        FAILED=$((FAILED + 1))
    fi
done

echo "================================================"
echo "Passed: $PASSED"
echo "Failed: $FAILED"
echo "================================================"

if [ $FAILED -gt 0 ]; then
    echo "😭 Some compilers made you cry!"
    echo "Check *.log files for details"
    exit 1
else
    echo "🔥 Your code is UNHOLY and BULLETPROOF! 🔥"
    echo "All compilers passed - you survived the DEVIL'S GAUNTLET 666!"
    exit 0
fi
