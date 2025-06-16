#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# Behavior-based test script for git-mind - tests what users can do, not how it's implemented

set -e

# Source Docker guard - will exit if not in Docker
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/../scripts/docker-guard.sh"

# Path to git-mind binary
GITMIND="$SCRIPT_DIR/../../git-mind"

echo "=== GitMind Behavior Test Suite ==="

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

# Test 1: Initialize git-mind
echo -n "Test 1: Can initialize git-mind... "
if $GITMIND init 2>&1; then
    echo "✓ PASS"
else
    echo "✗ FAIL: git-mind init failed"
    exit 1
fi

# Test 2: Create a link between files
echo -n "Test 2: Can create a link... "
if $GITMIND link README.md docs/ARCHITECTURE.md --type IMPLEMENTS 2>&1; then
    echo "✓ PASS"
else
    echo "✗ FAIL: $GITMIND link failed"
    exit 1
fi

# Test 3: List shows the link we created
echo -n "Test 3: Can list links... "
OUTPUT=$($GITMIND list)
if echo "$OUTPUT" | grep -q "README.md.*docs/ARCHITECTURE.md"; then
    echo "✓ PASS"
else
    echo "✗ FAIL: Link not visible in list"
    echo "Output: $OUTPUT"
    exit 1
fi

# Test 4: Can create multiple links
echo -n "Test 4: Can create multiple links... "
$GITMIND link docs/ARCHITECTURE.md docs/api.md --type REFERENCES
OUTPUT=$($GITMIND list | grep -c -- "->")
if [ "$OUTPUT" -ge "2" ]; then
    echo "✓ PASS"
else
    echo "✗ FAIL: Expected at least 2 links, got $OUTPUT"
    exit 1
fi

# Test 5: Can filter links by source
echo -n "Test 5: Can filter by source... "
OUTPUT=$($GITMIND list --source README.md)
if echo "$OUTPUT" | grep -q "README.md" && ! echo "$OUTPUT" | grep -q "docs/api.md"; then
    echo "✓ PASS"
else
    echo "✗ FAIL: Filter didn't work correctly"
    echo "Output: $OUTPUT"
    exit 1
fi

# Test 6: Status shows link count
echo -n "Test 6: Status shows links... "
OUTPUT=$($GITMIND status)
if echo "$OUTPUT" | grep -q "[0-9]"; then
    echo "✓ PASS"
else
    echo "✗ FAIL: Status didn't show link information"
    echo "Output: $OUTPUT"
    exit 1
fi

# Test 7: Can traverse from source to target
echo -n "Test 7: Can traverse links... "
OUTPUT=$($GITMIND traverse README.md)
if echo "$OUTPUT" | grep -q "docs/ARCHITECTURE.md"; then
    echo "✓ PASS"
else
    echo "✗ FAIL: Traverse didn't find linked file"
    echo "Output: $OUTPUT"
    exit 1
fi

# Test 8: Handles non-existent files gracefully
echo -n "Test 8: Handles missing files... "
if ! $GITMIND link nonexistent.txt docs/api.md 2>/dev/null; then
    echo "✓ PASS"
else
    echo "✗ FAIL: Should have failed for non-existent file"
    exit 1
fi

# Test 9: Can detect broken links
echo -n "Test 9: Can detect broken links... "
# First create a link to a file that will be deleted
echo "temp content" > temp.txt
git add temp.txt
git commit -m "Add temp file"
$GITMIND link README.md temp.txt --type REFERENCES
rm temp.txt
OUTPUT=$($GITMIND check)
if echo "$OUTPUT" | grep -q "broken\|invalid\|missing"; then
    echo "✓ PASS"
else
    echo "✗ FAIL: Didn't detect broken link"
    echo "Output: $OUTPUT"
    exit 1
fi

# Test 10: Links persist across git operations
echo -n "Test 10: Links persist... "
# Create a new branch
git checkout -b test-branch
OUTPUT_BEFORE=$($GITMIND list | grep -c -- "->")
git checkout main
OUTPUT_AFTER=$($GITMIND list | grep -c -- "->")
if [ "$OUTPUT_BEFORE" -eq "$OUTPUT_AFTER" ]; then
    echo "✓ PASS"
else
    echo "✗ FAIL: Links changed across branches"
    exit 1
fi

echo
echo "=== All behavior tests passed ==="
echo "Users can:"
echo "  - Initialize $GITMIND in a repo"
echo "  - Create links between files"
echo "  - List and filter links"
echo "  - Traverse link relationships"
echo "  - Check for broken links"
echo "  - Links persist with git"

cd /
rm -rf "$TESTDIR"