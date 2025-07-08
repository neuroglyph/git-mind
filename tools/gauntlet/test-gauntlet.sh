#!/bin/bash

# Quick test script for the GNU CRY GAUNTLET
# This just runs a subset to test the infrastructure

set -euo pipefail

echo "🧪 Testing GNU CRY GAUNTLET infrastructure..."

# Run with just GCC 14 and Clang 20 to test quickly
GAUNTLET_IMAGE="gnu-cry-gauntlet:latest"

# Build if needed
if ! docker image inspect $GAUNTLET_IMAGE >/dev/null 2>&1; then
    echo "Building GAUNTLET Docker image..."
    docker build -t $GAUNTLET_IMAGE -f tools/gauntlet/Dockerfile.gauntlet .
fi

# Test run with a subset of compilers
echo "Testing with GCC 14 and Clang 20..."
docker run --rm -v "$PWD":/workspace -w /workspace $GAUNTLET_IMAGE bash -c '
    echo "🧪 GAUNTLET INFRASTRUCTURE TEST"
    echo "Running subset: GCC 14 and Clang 20"
    echo ""
    
    # Test GCC 14
    /gauntlet/test-compiler.sh gcc-14 &
    PID_GCC=$!
    
    # Test Clang 20
    /gauntlet/test-compiler.sh clang-20 &
    PID_CLANG=$!
    
    # Wait for both
    wait $PID_GCC
    wait $PID_CLANG
    
    echo "Results:"
    echo "GCC 14: $(cat result-gcc-14.txt)"
    echo "Clang 20: $(cat result-clang-20.txt)"
    
    # Check if both passed
    if [ "$(cat result-gcc-14.txt)" = "PASS" ] && [ "$(cat result-clang-20.txt)" = "PASS" ]; then
        echo "✅ Infrastructure test PASSED"
        exit 0
    else
        echo "❌ Infrastructure test FAILED"
        exit 1
    fi
'