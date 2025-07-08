#!/bin/bash

# GNU CRY GAUNTLET - Run parallel C23 compiler tests
# This script runs your code through multiple compilers in parallel

set -euo pipefail

echo "🔥 DEVIL'S COMPILER GAUNTLET 666 🔥"
echo "Building Docker image for multi-compiler testing..."

# Build the GAUNTLET image
GAUNTLET_IMAGE="gnu-cry-gauntlet:latest"

# Check if we need to build/rebuild
if ! docker image inspect $GAUNTLET_IMAGE >/dev/null 2>&1; then
    echo "Building GAUNTLET Docker image..."
    docker build -t $GAUNTLET_IMAGE -f tools/gauntlet/Dockerfile.gauntlet .
else
    echo "GAUNTLET Docker image already exists"
fi

# Run the GAUNTLET
echo ""
echo "🔫 Running GAUNTLET with 5 compilers in parallel..."
echo "  - GCC 12 (Pre-C23 - THE DEVIL)"
echo "  - GCC 13 (C23 support progression)"
echo "  - Clang 18, 19, 20 (C23 support progression)"
echo ""

# Run the container
docker run --rm -v "$PWD":/workspace -w /workspace $GAUNTLET_IMAGE

echo ""
echo "🏁 GAUNTLET COMPLETE!"