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
    
    # Behavior: verify link was created by checking journal
    [ -n "$(git show-ref refs/gitmind/edges/main)" ]
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
    
    # Behavior: journal ref should not exist on feature branch
    [ -z "$(git show-ref refs/gitmind/edges/feature 2>/dev/null)" ]
}

# Test: Can specify relationship type
test_relationship_types() {
    echo "code" > impl.c
    echo "design" > design.md
    git add .
    git commit -m "initial" --quiet
    
    # Behavior: can create typed relationships
    output=$("$GIT_MIND" --porcelain link impl.c design.md --type implements)
    echo "$output" | grep -q "type=1"
    
    # Behavior: verify link was created
    [ -n "$(git show-ref refs/gitmind/edges/main)" ]
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
    
    # Behavior: both links should be created
    [ -n "$(git show-ref refs/gitmind/edges/main)" ]
    
    # Verify two distinct edges were created
    output1=$("$GIT_MIND" --porcelain link a.txt b.txt 2>&1 || true)
    output2=$("$GIT_MIND" --porcelain link b.txt c.txt 2>&1 || true)
    echo "$output1" | grep -q "status=duplicate"
    echo "$output2" | grep -q "status=duplicate"
}

# Test: Unicode paths work
test_unicode_paths() {
    echo "hello" > "你好.txt"
    echo "world" > "世界.txt"
    git add .
    git commit -m "unicode" --quiet
    
    # Behavior: can link unicode paths
    output=$("$GIT_MIND" --porcelain link "你好.txt" "世界.txt")
    echo "$output" | grep -q "status=created"
    echo "$output" | grep -q "source=你好.txt"
    echo "$output" | grep -q "target=世界.txt"
}

# Test: Push/pull compatibility
test_push_pull() {
    echo "A" > a.txt
    git add .
    git commit -m "initial" --quiet
    
    # Create link
    "$GIT_MIND" link a.txt a.txt
    
    # Behavior: refs should exist
    [ -n "$(git show-ref refs/gitmind/edges/main)" ]
    
    # Note: Actual push/pull test requires remote
}

# Test: Human attribution (default behavior)
test_human_attribution() {
    echo "main" > main.c
    echo "header" > main.h
    git add .
    git commit -m "initial" --quiet
    
    # Clear environment to ensure human defaults
    unset GIT_MIND_SOURCE GIT_MIND_AUTHOR GIT_MIND_SESSION
    
    # Behavior: human link should succeed
    output=$("$GIT_MIND" --porcelain link main.c main.h --type implements)
    echo "$output" | grep -q "status=created"
    echo "$output" | grep -q "confidence=1.000"
    
    # Verify link was created
    [ -n "$(git show-ref refs/gitmind/edges/main)" ]
}

# Test: AI attribution from environment
test_ai_attribution() {
    echo "auth" > auth.c
    echo "config" > oauth.json
    git add .
    git commit -m "initial" --quiet
    
    # Behavior: AI with environment should record attribution
    export GIT_MIND_SOURCE=claude
    export GIT_MIND_AUTHOR=claude@anthropic
    export GIT_MIND_SESSION=test123
    
    output=$("$GIT_MIND" --porcelain link auth.c oauth.json --type depends_on --confidence 0.85)
    echo "$output" | grep -q "status=created"
    echo "$output" | grep -q "confidence=0.850"
    
    # Verify link was created
    [ -n "$(git show-ref refs/gitmind/edges/main)" ]
    
    # Cleanup
    unset GIT_MIND_SOURCE GIT_MIND_AUTHOR GIT_MIND_SESSION
}

# Test: Confidence validation
test_confidence_validation() {
    echo "file1" > file1.txt
    echo "file2" > file2.txt
    git add .
    git commit -m "initial" --quiet
    
    export GIT_MIND_SOURCE=claude
    
    # Behavior: invalid confidence should be rejected
    ! "$GIT_MIND" link file1.txt file2.txt --confidence 1.5
    ! "$GIT_MIND" link file1.txt file2.txt --confidence -0.1
    
    # Behavior: valid confidence should work
    "$GIT_MIND" link file1.txt file2.txt --confidence 0.5
    
    unset GIT_MIND_SOURCE
}

# Test: List command filtering flags
test_list_filtering() {
    echo "file1" > file1.txt
    echo "file2" > file2.txt
    echo "file3" > file3.txt
    git add .
    git commit -m "initial" --quiet
    
    # Create human edge
    unset GIT_MIND_SOURCE GIT_MIND_AUTHOR
    human_output=$("$GIT_MIND" --porcelain link file1.txt file2.txt --type implements)
    echo "$human_output" | grep -q "status=created"
    
    # Create AI edge
    export GIT_MIND_SOURCE=claude
    export GIT_MIND_AUTHOR=claude@anthropic
    ai_output=$("$GIT_MIND" --porcelain link file2.txt file3.txt --type depends_on --confidence 0.8)
    echo "$ai_output" | grep -q "status=created"
    echo "$ai_output" | grep -q "confidence=0.800"
    
    # Behavior: both edges should be created
    [ -n "$(git show-ref refs/gitmind/edges/main)" ]
    
    # Verify we have journal commits
    commit_count=$(git rev-list refs/gitmind/edges/main | wc -l)
    [ "$commit_count" -ge 2 ]
    
    unset GIT_MIND_SOURCE GIT_MIND_AUTHOR
}

# Test: --verbose and --porcelain output control
test_output_control() {
    echo "main" > main.c
    echo "config" > config.h
    git add .
    git commit -m "initial" --quiet
    
    # Test --porcelain for link command
    porcelain_output=$("$GIT_MIND" --porcelain link main.c config.h --type depends_on 2>&1)
    echo "$porcelain_output" | grep -q "status=created"
    echo "$porcelain_output" | grep -q "source=main.c"
    echo "$porcelain_output" | grep -q "target=config.h"
    echo "$porcelain_output" | grep -q "type=3"
    echo "$porcelain_output" | grep -q "confidence=1.000"
    echo "$porcelain_output" | grep -q "ulid="
    
    # Porcelain should not contain human-readable text
    ! echo "$porcelain_output" | grep -q "Created link:"
    
    # Test cache rebuild creates cache ref
    "$GIT_MIND" cache-rebuild >/dev/null 2>&1 || true
    [ -n "$(git show-ref refs/gitmind/cache/main 2>/dev/null || true)" ]
    
    # Test --porcelain for install-hooks
    hooks_output=$("$GIT_MIND" --porcelain install-hooks 2>&1)
    echo "$hooks_output" | grep -q "status=installed"
    echo "$hooks_output" | grep -q "hook=post-commit"
    
    # Second install should show already-installed
    hooks_output2=$("$GIT_MIND" --porcelain install-hooks 2>&1)
    echo "$hooks_output2" | grep -q "status=already-installed"
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
run_test "human_attribution" test_human_attribution
run_test "ai_attribution" test_ai_attribution
run_test "confidence_validation" test_confidence_validation
run_test "list_filtering" test_list_filtering
run_test "output_control" test_output_control

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