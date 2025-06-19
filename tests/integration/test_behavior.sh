#!/bin/bash
# SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0
# © 2025 J. Kirby Ross / Neuroglyph Collective

# Integration test suite - Test BEHAVIOR not implementation

set -e

# Test directory
TEST_DIR="/tmp/gm-test-$$"
GIT_MIND="${GIT_MIND:-../../git-mind}"

# SAFETY CHECK: Never run in dev repo!
if pwd | grep -q "git-mind" && [ -f "CLAUDE.md" ]; then
    echo "🚨 SAFETY: Cannot run tests in development repo!"
    echo "Use 'make test' instead."
    exit 42
fi

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

# Counters
PASSED=0
FAILED=0

# Helper: Run test
run_test() {
    local name="$1"
    local test_func="$2"
    
    echo -n "Testing $name... "
    
    if (
        set -e
        mkdir -p "$TEST_DIR/$name"
        cd "$TEST_DIR/$name"
        git init --quiet
        git config user.email "test@example.com"
        git config user.name "Test"
        $test_func
    ) 2>&1 | tee /tmp/test-$name.log >/dev/null; then
        echo -e "${GREEN}PASS${NC}"
        PASSED=$((PASSED + 1))
    else
        echo -e "${RED}FAIL${NC}"
        FAILED=$((FAILED + 1))
    fi
}

# Test: Can create a link between two files
test_link_creation() {
    echo "A" > a.txt
    echo "B" > b.txt
    git add .
    git commit -m "initial" --quiet
    
    # Behavior: linking two files should succeed
    "$GIT_MIND" link a.txt b.txt
    
    # Behavior: list should show the link
    "$GIT_MIND" list | grep -q "a.txt -> b.txt"
}

# Test: Cannot link non-existent files
test_link_nonexistent() {
    echo "A" > a.txt
    git add .
    git commit -m "initial" --quiet
    
    # Behavior: linking to non-existent file should fail
    ! "$GIT_MIND" link a.txt missing.txt
}

# Test: Links are branch-specific
test_branch_isolation() {
    echo "A" > a.txt
    git add .
    git commit -m "initial" --quiet
    
    # Create link on main
    "$GIT_MIND" link a.txt a.txt
    
    # Create new branch
    git checkout -b feature --quiet
    
    # Behavior: link from main should not appear on feature
    local count=$("$GIT_MIND" list | grep -c "a.txt -> a.txt" || true)
    [ "$count" -eq 0 ]
}

# Test: Can specify relationship type
test_relationship_types() {
    echo "code" > impl.c
    echo "design" > design.md
    git add .
    git commit -m "initial" --quiet
    
    # Behavior: can create typed relationships
    "$GIT_MIND" link impl.c design.md --type implements
    
    # Behavior: list shows the relationship type
    "$GIT_MIND" list | grep -q "IMPLEMENTS"
}

# Test: Can filter by path
test_list_filter() {
    echo "A" > a.txt
    echo "B" > b.txt  
    echo "C" > c.txt
    git add .
    git commit -m "initial" --quiet
    
    "$GIT_MIND" link a.txt b.txt
    "$GIT_MIND" link b.txt c.txt
    
    # Behavior: filtering by path shows only relevant links
    local count=$("$GIT_MIND" list b.txt | grep -E "^[A-Z_]+:" | wc -l)
    [ "$count" -eq 2 ]
}

# Test: Unicode paths work
test_unicode_paths() {
    echo "hello" > "你好.txt"
    echo "world" > "世界.txt"
    git add .
    git commit -m "unicode" --quiet
    
    # Behavior: can link unicode paths
    "$GIT_MIND" link "你好.txt" "世界.txt"
    
    # Behavior: unicode paths display correctly
    "$GIT_MIND" list | grep -q "你好.txt -> 世界.txt"
}

# Test: Push/pull compatibility
test_push_pull() {
    echo "A" > a.txt
    git add .
    git commit -m "initial" --quiet
    
    # Create link
    "$GIT_MIND" link a.txt a.txt
    
    # Behavior: refs should exist
    git show-ref | grep -q "refs/gitmind/edges"
    
    # Note: Actual push/pull test requires remote
}

# Run all tests
echo "Running git-mind integration tests..."
echo ""

run_test "link_creation" test_link_creation
run_test "link_nonexistent" test_link_nonexistent  
run_test "branch_isolation" test_branch_isolation
run_test "relationship_types" test_relationship_types
run_test "list_filter" test_list_filter
run_test "unicode_paths" test_unicode_paths
run_test "push_pull" test_push_pull

# Cleanup
rm -rf "$TEST_DIR"

# Summary
echo ""
echo "Results: $PASSED passed, $FAILED failed"

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
fi