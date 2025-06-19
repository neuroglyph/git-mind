#!/bin/bash
# SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0
# © 2025 J. Kirby Ross / Neuroglyph Collective

# E2E Test Framework for git-mind
# No bullshit, just POSIX shell and raw performance

set -e

# Colors for output (disable if no tty)
if [ -t 1 ]; then
    RED='\033[0;31m'
    GREEN='\033[0;32m'
    YELLOW='\033[1;33m'
    NC='\033[0m'
else
    RED=''
    GREEN=''
    YELLOW=''
    NC=''
fi

# Test counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# Timing
START_TIME=$(date +%s)

# Test directory
TEST_DIR="/tmp/git-mind-test-$$"
ORIGINAL_DIR=$(pwd)

# Binary location
GIT_MIND="${GIT_MIND:-../../git-mind}"

# SAFETY CHECK: Never run tests in development repo!
if pwd | grep -q "git-mind" && [ -f "CLAUDE.md" ]; then
    echo "🚨🚨🚨 SAFETY VIOLATION! 🚨🚨🚨"
    echo ""
    echo "NEVER run tests in the git-mind development repository!"
    echo "This could create journal commits and corrupt the source."
    echo ""
    echo "Use 'make test' to run tests in Docker instead."
    echo ""
    exit 42
fi

# Ensure binary exists
if [ ! -x "$GIT_MIND" ]; then
    echo "ERROR: git-mind binary not found at $GIT_MIND"
    echo "Build it first or set GIT_MIND environment variable"
    exit 1
fi

# Helper: Create test repo
init_test_repo() {
    local name=${1:-test}
    mkdir -p "$TEST_DIR/$name"
    cd "$TEST_DIR/$name"
    git init --quiet
    git config user.email "test@git-mind.io"
    git config user.name "Test User"
}

# Helper: Create test file
create_file() {
    local file=$1
    local content=${2:-"content of $file"}
    mkdir -p "$(dirname "$file")"
    echo "$content" > "$file"
    git add "$file"
}

# Helper: Commit changes
commit() {
    local msg=${1:-"test commit"}
    git commit --quiet -m "$msg"
}

# Helper: Run git-mind command
gm() {
    "$GIT_MIND" "$@"
}

# Helper: Assert command succeeds
assert_success() {
    local cmd="$1"
    TESTS_RUN=$((TESTS_RUN + 1))
    
    if eval "$cmd" > /dev/null 2>&1; then
        TESTS_PASSED=$((TESTS_PASSED + 1))
        echo -e "${GREEN}✓${NC} $cmd"
    else
        TESTS_FAILED=$((TESTS_FAILED + 1))
        echo -e "${RED}✗${NC} $cmd"
        echo "  Expected success but got exit code $?"
    fi
}

# Helper: Assert command fails
assert_failure() {
    local cmd="$1"
    TESTS_RUN=$((TESTS_RUN + 1))
    
    if eval "$cmd" > /dev/null 2>&1; then
        TESTS_FAILED=$((TESTS_FAILED + 1))
        echo -e "${RED}✗${NC} $cmd"
        echo "  Expected failure but got success"
    else
        TESTS_PASSED=$((TESTS_PASSED + 1))
        echo -e "${GREEN}✓${NC} $cmd (expected failure)"
    fi
}

# Helper: Assert output contains string
assert_contains() {
    local cmd="$1"
    local expected="$2"
    TESTS_RUN=$((TESTS_RUN + 1))
    
    local output
    output=$(eval "$cmd" 2>&1)
    
    if echo "$output" | grep -q "$expected"; then
        TESTS_PASSED=$((TESTS_PASSED + 1))
        echo -e "${GREEN}✓${NC} $cmd | grep '$expected'"
    else
        TESTS_FAILED=$((TESTS_FAILED + 1))
        echo -e "${RED}✗${NC} $cmd | grep '$expected'"
        echo "  Output was: $output"
    fi
}

# Helper: Assert edge exists
assert_edge_exists() {
    local src="$1"
    local tgt="$2"
    local type="${3:-REFERENCES}"
    
    assert_contains "gm list" "$type: $src -> $tgt"
}

# Helper: Count edges
count_edges() {
    gm list | grep -E '^[A-Z_]+:' | wc -l
}

# Helper: Get journal commit count
journal_commits() {
    local branch=${1:-$(git branch --show-current)}
    git rev-list --count "refs/gitmind/edges/$branch" 2>/dev/null || echo 0
}

# Helper: Benchmark command
benchmark() {
    local name="$1"
    local cmd="$2"
    local target="$3"
    
    echo -e "${YELLOW}BENCHMARK:${NC} $name"
    
    local start=$(date +%s%N)
    eval "$cmd" > /dev/null
    local end=$(date +%s%N)
    
    local elapsed=$(( (end - start) / 1000000 ))
    echo "  Time: ${elapsed}ms (target: <${target}ms)"
    
    if [ "$elapsed" -lt "$target" ]; then
        echo -e "  ${GREEN}✓ PASS${NC}"
    else
        echo -e "  ${RED}✗ FAIL${NC} (too slow)"
    fi
}

# Helper: Memory usage
measure_memory() {
    local cmd="$1"
    local target="$2"
    
    if command -v /usr/bin/time >/dev/null 2>&1; then
        local output=$(/usr/bin/time -l "$cmd" 2>&1)
        local peak=$(echo "$output" | grep "peak memory" | awk '{print $1}')
        local mb=$((peak / 1024 / 1024))
        echo "  Peak memory: ${mb}MB (target: <${target}MB)"
    else
        echo "  Memory measurement not available"
    fi
}

# Cleanup function
cleanup() {
    cd "$ORIGINAL_DIR"
    rm -rf "$TEST_DIR"
}

# Set trap for cleanup
trap cleanup EXIT INT TERM

# Final report
report() {
    local end_time=$(date +%s)
    local duration=$((end_time - START_TIME))
    
    echo ""
    echo "========================================"
    echo "Test Results:"
    echo "  Total:  $TESTS_RUN"
    echo -e "  Passed: ${GREEN}$TESTS_PASSED${NC}"
    echo -e "  Failed: ${RED}$TESTS_FAILED${NC}"
    echo "  Time:   ${duration}s"
    echo "========================================"
    
    if [ "$TESTS_FAILED" -eq 0 ]; then
        echo -e "${GREEN}All tests passed!${NC}"
        exit 0
    else
        echo -e "${RED}Some tests failed!${NC}"
        exit 1
    fi
}