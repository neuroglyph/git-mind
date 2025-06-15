#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# Run AFL++ fuzzing in Docker

set -e

echo "🔨 Building fuzzing image..."
# Get to project root
cd "$(dirname "$0")/../.."
docker build -f Dockerfile.fuzz -t git-mind-fuzz .

echo "🎯 Starting fuzzer (press Ctrl+C to stop)..."
echo "Results will be in the container at /fuzz/findings"
docker run -it --rm git-mind-fuzz