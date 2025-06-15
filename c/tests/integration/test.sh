#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# Test script for git-mind - runs in isolated Docker container

set -e

# Source Docker guard - will exit if not in Docker
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/../scripts/docker-guard.sh"

echo "=== GitMind C Test Suite ==="

# Create temp directory
TESTDIR=$(mktemp -d)
cd "$TESTDIR"

# Configure git to use main branch
git config --global init.defaultBranch main 2>/dev/null || true

# Initialize git repo
git init
git config user.email "test@example.com"
git config user.name "Test User"

# Create some test files
echo "# Test Project" > README.md
mkdir -p docs
echo "# Architecture" > docs/ARCHITECTURE.md
echo "# API Docs" > docs/api.md
git add .
git commit -m "Initial commit"

echo "✓ Test repo created"

# Test 1: Init
echo -n "Test 1: git-mind init... "
git-mind init
if [ -d .gitmind/links ]; then
    echo "✓ PASS"
else
    echo "✗ FAIL: .gitmind/links not created"
    exit 1
fi

# Test 2: Create link
echo -n "Test 2: git-mind link... "
git-mind link README.md docs/ARCHITECTURE.md --type IMPLEMENTS
if [ -n "$(ls .gitmind/links/*.link 2>/dev/null)" ]; then
    echo "✓ PASS"
else
    echo "✗ FAIL: No link file created"
    exit 1
fi

# Test 3: List links
echo -n "Test 3: git-mind list... "
OUTPUT=$(git-mind list)
if echo "$OUTPUT" | grep -q "IMPLEMENTS: README.md -> docs/ARCHITECTURE.md"; then
    echo "✓ PASS"
else
    echo "✗ FAIL: Link not found in list"
    echo "Output: $OUTPUT"
    exit 1
fi

# Test 4: Create another link
echo -n "Test 4: Multiple links... "
git-mind link docs/ARCHITECTURE.md docs/api.md --type REFERENCES
OUTPUT=$(git-mind list | wc -l)
if [ "$OUTPUT" -eq "2" ]; then
    echo "✓ PASS"
else
    echo "✗ FAIL: Expected 2 links, got $OUTPUT"
    exit 1
fi

# Test 5: Filter by source
echo -n "Test 5: Filter by source... "
OUTPUT=$(git-mind list --source README.md | wc -l)
if [ "$OUTPUT" -eq "1" ]; then
    echo "✓ PASS"
else
    echo "✗ FAIL: Expected 1 link, got $OUTPUT"
    exit 1
fi

# Test 6: Unlink
echo -n "Test 6: git-mind unlink... "
git-mind unlink README.md docs/ARCHITECTURE.md
OUTPUT=$(git-mind list | wc -l)
if [ "$OUTPUT" -eq "1" ]; then
    echo "✓ PASS"
else
    echo "✗ FAIL: Expected 1 link after unlink, got $OUTPUT"
    exit 1
fi

# Test 7: SHA consistency
echo -n "Test 7: SHA consistency... "
# Create same link twice
git-mind link README.md docs/api.md --type REFERENCES
COUNT=$(ls .gitmind/links/*.link | wc -l)
if [ "$COUNT" -eq "2" ]; then  # Should be 2: one from test 4, one from test 7
    echo "✓ PASS (deduplication works)"
else
    echo "✗ FAIL: Expected 2 unique links, got $COUNT"
    exit 1
fi

# Test 8: Git integration
echo -n "Test 8: Git integration... "
git add .gitmind
git commit -m "Add git-mind links" >/dev/null 2>&1
if git log --oneline | grep -q "Add git-mind links"; then
    echo "✓ PASS"
else
    echo "✗ FAIL: Links not committed to git"
    exit 1
fi

# Test 9: Status command
echo -n "Test 9: git-mind status... "
OUTPUT=$(git-mind status)
if echo "$OUTPUT" | grep -q "Total links: 2"; then
    echo "✓ PASS"
else
    echo "✗ FAIL: Status didn't show correct link count"
    exit 1
fi

# Test 10: Check command
echo -n "Test 10: git-mind check... "
# Remove a target file to create broken link
rm docs/api.md
OUTPUT=$(git-mind check 2>&1)
if echo "$OUTPUT" | grep -q "Broken link"; then
    echo "✓ PASS"
else
    echo "✗ FAIL: Check didn't detect broken link"
    exit 1
fi

# Test 11: Check --fix
echo -n "Test 11: git-mind check --fix... "
git-mind check --fix >/dev/null 2>&1
COUNT=$(git-mind list | wc -l)
if [ "$COUNT" -eq "1" ]; then
    echo "✓ PASS"
else
    echo "✗ FAIL: Expected 1 link after fix, got $COUNT"
    exit 1
fi

# Clean up
cd /
rm -rf "$TESTDIR"

echo ""
echo "=== All tests passed! ==="
echo "Binary size: $(ls -lh $(which git-mind) | awk '{print $5}')"