#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# Run tests in Docker container

set -e

echo "Building Docker test image..."
docker build -f ../../Dockerfile.test -t git-mind-test ../..

echo "Running tests in container..."
docker run --rm git-mind-test /build/tests/integration/test-all.sh