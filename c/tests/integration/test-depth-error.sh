#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# Test depth error message

set -e

# Source Docker guard - will exit if not in Docker
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/../scripts/docker-guard.sh"

TESTDIR=$(mktemp -d)
cd "$TESTDIR"
git init -q
git config user.email "test@example.com"
git config user.name "Test User"
echo "README" > README.md
git add . && git commit -q -m "Initial"
git-mind init

echo "Testing depth 11:"
set +e  # Allow the command to fail
git-mind traverse README.md --depth 11 2>&1
EXIT_CODE=$?
set -e

# The command should fail with an error
if [ $EXIT_CODE -ne 0 ]; then
    echo "✓ Command correctly failed with exit code $EXIT_CODE"
    cd /
    rm -rf "$TESTDIR"
    exit 0
else
    echo "✗ Command should have failed but didn't"
    cd /
    rm -rf "$TESTDIR"
    exit 1
fi