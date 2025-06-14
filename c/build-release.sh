#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# Build optimized release binaries for different platforms

set -euo pipefail

echo "🚀 Building Release Binaries"
echo "==========================="

# Clean first
rm -f gitmind gitmind-* src/*.o

# Linux x86_64 with GCC (static, musl)
echo "Building Linux x86_64..."
docker compose run --rm dev sh -c "
    DOCKER_BUILD=1 \
    CC=gcc \
    CFLAGS='-O3 -Wall -Wextra -std=c99 -Iinclude -march=x86-64 -mtune=generic' \
    LDFLAGS='-static -s' \
    make clean all && \
    mv gitmind gitmind-linux-x86_64
"

# macOS with Clang (if on macOS)
if [[ "$OSTYPE" == "darwin"* ]]; then
    echo "Building macOS..."
    DOCKER_BUILD=1 \
    CC=clang \
    CFLAGS='-O3 -Wall -Wextra -std=c99 -Iinclude' \
    LDFLAGS='-Wl,-dead_strip' \
    make clean all
    mv gitmind gitmind-macos
fi

# List results
echo ""
echo "✅ Release builds complete:"
ls -lh gitmind-* 2>/dev/null || echo "No binaries found"