#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# Test script for git-mind traverse command - runs in isolated Docker container

set -e

# Source Docker guard - will exit if not in Docker
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/../scripts/docker-guard.sh"

echo "=== GitMind Traverse Test Suite ==="

# Create temp directory
TESTDIR=$(mktemp -d)
cd "$TESTDIR"

# Initialize git repo
git init
git config user.email "test@example.com"
git config user.name "Test User"

# Create test files for a realistic graph structure
mkdir -p src docs tests
echo "# Project" > README.md
echo "# Architecture" > docs/architecture.md
echo "# API Design" > docs/api-design.md
echo "# Database Schema" > docs/db-schema.md
echo "// Main implementation" > src/main.c
echo "// Database module" > src/database.c
echo "// API handlers" > src/api.c
echo "// Test main" > tests/main.test.c
echo "// Test API" > tests/api.test.c
git add .
git commit -m "Initial test structure"

echo "✓ Test repo created"

# Initialize git-mind
git-mind init

# Create a graph with multiple levels of connections
# README -> architecture -> api-design -> api.c
# README -> architecture -> db-schema -> database.c
# main.c -> database.c
# api.c -> tests/api.test.c
# main.c -> tests/main.test.c

git-mind link README.md docs/architecture.md --type REFERENCES
git-mind link docs/architecture.md docs/api-design.md --type DETAILS
git-mind link docs/architecture.md docs/db-schema.md --type DETAILS
git-mind link docs/api-design.md src/api.c --type IMPLEMENTS
git-mind link docs/db-schema.md src/database.c --type IMPLEMENTS
git-mind link src/main.c src/database.c --type DEPENDS_ON
git-mind link src/api.c tests/api.test.c --type TESTED_BY
git-mind link src/main.c tests/main.test.c --type TESTED_BY

echo "✓ Test graph created"

# Test 1: Basic traversal (depth 1)
echo -n "Test 1: Traverse depth 1... "
OUTPUT=$(git-mind traverse README.md --depth 1)
if echo "$OUTPUT" | grep -q "docs/architecture.md"; then
    if echo "$OUTPUT" | grep -q "docs/api-design.md"; then
        echo "✗ FAIL: Depth 1 should not show api-design.md"
        exit 1
    fi
    echo "✓ PASS"
else
    echo "✗ FAIL: Should show architecture.md at depth 1"
    exit 1
fi

# Test 2: Traverse depth 2
echo -n "Test 2: Traverse depth 2... "
OUTPUT=$(git-mind traverse README.md --depth 2)
if echo "$OUTPUT" | grep -q "docs/architecture.md" && \
   echo "$OUTPUT" | grep -q "docs/api-design.md" && \
   echo "$OUTPUT" | grep -q "docs/db-schema.md"; then
    if echo "$OUTPUT" | grep -q "src/api.c"; then
        echo "✗ FAIL: Depth 2 should not show api.c"
        exit 1
    fi
    echo "✓ PASS"
else
    echo "✗ FAIL: Should show all docs at depth 2"
    exit 1
fi

# Test 3: Traverse depth 3
echo -n "Test 3: Traverse depth 3... "
OUTPUT=$(git-mind traverse README.md --depth 3)
if echo "$OUTPUT" | grep -q "src/api.c" && \
   echo "$OUTPUT" | grep -q "src/database.c"; then
    echo "✓ PASS"
else
    echo "✗ FAIL: Should reach implementation files at depth 3"
    exit 1
fi

# Test 4: Default depth (should be 1)
echo -n "Test 4: Default depth... "
OUTPUT1=$(git-mind traverse README.md)
OUTPUT2=$(git-mind traverse README.md --depth 1)
if [ "$OUTPUT1" = "$OUTPUT2" ]; then
    echo "✓ PASS"
else
    echo "✗ FAIL: Default depth should be 1"
    exit 1
fi

# Test 5: Cycle detection
echo -n "Test 5: Cycle detection... "
# Create a cycle: A -> B -> C -> A
echo "A" > A.txt
echo "B" > B.txt
echo "C" > C.txt
git-mind link A.txt B.txt --type REFS
git-mind link B.txt C.txt --type REFS
git-mind link C.txt A.txt --type REFS
OUTPUT=$(git-mind traverse A.txt --depth 10)
# Count occurrences of each file - should be exactly once
COUNT_A=$(echo "$OUTPUT" | grep -c "A.txt" || true)
COUNT_B=$(echo "$OUTPUT" | grep -c "B.txt" || true)
COUNT_C=$(echo "$OUTPUT" | grep -c "C.txt" || true)
if [ "$COUNT_A" -eq 1 ] && [ "$COUNT_B" -eq 1 ] && [ "$COUNT_C" -eq 1 ]; then
    echo "✓ PASS"
else
    echo "✗ FAIL: Cycle not handled correctly"
    echo "A.txt appeared $COUNT_A times"
    echo "B.txt appeared $COUNT_B times"
    echo "C.txt appeared $COUNT_C times"
    exit 1
fi

# Test 6: Tree format
echo -n "Test 6: Tree format output... "
OUTPUT=$(git-mind traverse README.md --depth 2 --format tree)
if echo "$OUTPUT" | grep -q "+->" || echo "$OUTPUT" | grep -q "\\\->"; then
    echo "✓ PASS"
else
    echo "✗ FAIL: Tree format should use tree characters"
    echo "Output: $OUTPUT"
    exit 1
fi

# Test 7: List format
echo -n "Test 7: List format output... "
OUTPUT=$(git-mind traverse src/main.c --depth 2 --format list)
# Should be simple list without tree formatting
if echo "$OUTPUT" | grep -q "├─→"; then
    echo "✗ FAIL: List format should not have tree characters"
    exit 1
fi
if echo "$OUTPUT" | grep -q "src/database.c" && \
   echo "$OUTPUT" | grep -q "tests/main.test.c"; then
    echo "✓ PASS"
else
    echo "✗ FAIL: List format should show connected nodes"
    exit 1
fi

# Test 8: Connection count
echo -n "Test 8: Connection count display... "
OUTPUT=$(git-mind traverse README.md --depth 3)
if echo "$OUTPUT" | grep -q "direct" && echo "$OUTPUT" | grep -q "total"; then
    echo "✓ PASS"
else
    echo "✗ FAIL: Should show connection counts"
    echo "Output: $OUTPUT"
    exit 1
fi

# Test 9: Maximum depth limit
echo -n "Test 9: Maximum depth limit... "
if git-mind traverse README.md --depth 11 2>&1 | grep -qi "depth.*between.*1.*10"; then
    echo "✓ PASS"
else
    echo "✗ FAIL: Should enforce maximum depth of 10"
    exit 1
fi

# Test 10: Non-existent starting node
echo -n "Test 10: Non-existent starting node... "
if git-mind traverse nonexistent.md 2>&1 | grep -q "not found"; then
    echo "✓ PASS"
else
    echo "✗ FAIL: Should handle non-existent starting nodes"
    exit 1
fi

# Clean up
cd /
rm -rf "$TESTDIR"

echo ""
echo "=== All traverse tests passed! ==="