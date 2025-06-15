#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# © 2025 J. Kirby Ross / Neuroglyph Collective
#
# Test all output modes: silent, verbose, porcelain

set -uo pipefail  # Remove -e to allow tests to continue on failure

# Source Docker guard - will exit if not in Docker
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/../scripts/docker-guard.sh"

# Get to the C implementation root
cd "$(dirname "$0")/../.."

echo "=== GitMind Output Modes Test Suite ==="
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

FAILED=0
PASSED=0

# Test function
test_output() {
    local description="$1"
    local cmd="$2"
    local expected_pattern="$3"
    local should_match="${4:-yes}"  # yes or no
    
    echo -n "Testing $description... "
    
    # Run command and capture output
    output=$($cmd 2>&1 || true)
    
    if [ "$should_match" = "yes" ]; then
        if echo "$output" | grep -q "$expected_pattern"; then
            echo -e "${GREEN}✓${NC}"
            ((PASSED++))
        else
            echo -e "${RED}✗${NC}"
            echo "  Expected to find: '$expected_pattern'"
            echo "  Got: '$output'"
            ((FAILED++))
        fi
    else
        if echo "$output" | grep -q "$expected_pattern"; then
            echo -e "${RED}✗${NC}"
            echo "  Expected NOT to find: '$expected_pattern'"
            echo "  Got: '$output'"
            ((FAILED++))
        else
            echo -e "${GREEN}✓${NC}"
            ((PASSED++))
        fi
    fi
}

# Create test repository
TEST_DIR=$(mktemp -d)
cd "$TEST_DIR"
git init --quiet
git config user.name "Test User"
git config user.email "test@example.com"
echo "test" > README.md
echo "content" > test.txt
git add README.md test.txt
git commit -m "Initial commit" --quiet

# Initialize git-mind first
git-mind init >/dev/null 2>&1

echo "=== 1. Silent Mode (default) ==="

# Init again - should be silent (idempotent)
test_output "init silent" "git-mind init" "." "no"

# Link - should be silent
test_output "link silent" "git-mind link README.md test.txt" "." "no"

# List - should show output (query commands always output)
test_output "list output" "git-mind list" "README.md" "yes"

# Unlink - should be silent
test_output "unlink silent" "git-mind unlink README.md test.txt" "." "no"

echo
echo "=== 2. Verbose Mode (-v) ==="

# Init - should show message
test_output "init verbose" "git-mind -v init" "Initialized git-mind" "yes"

# Link - should show message
test_output "link verbose" "git-mind -v link README.md test.txt" "Created link" "yes"

# Unlink - should show message
test_output "unlink verbose" "git-mind -v unlink README.md test.txt" "Removed link" "yes"

echo
echo "=== 3. Porcelain Mode (--porcelain) ==="

# Init - should show porcelain format
test_output "init porcelain" "git-mind --porcelain init" "init:ok" "yes"

# Link - should show porcelain format
test_output "link porcelain" "git-mind --porcelain link README.md test.txt --type DEPENDS" "link:created:README.md:test.txt:DEPENDS" "yes"

# List - should show porcelain format
test_output "list porcelain" "git-mind --porcelain list" "link:DEPENDS:README.md:test.txt:" "yes"

# Unlink - should show porcelain format
test_output "unlink porcelain" "git-mind --porcelain unlink README.md test.txt" "link:removed:README.md:test.txt" "yes"

echo
echo "=== 4. Combined Modes ==="

# Both verbose and porcelain should prefer porcelain
test_output "both flags prefer porcelain" "git-mind -v --porcelain init" "init:ok" "yes"
test_output "both flags not verbose" "git-mind -v --porcelain init" "Initialized git-mind" "no"

echo
echo "=== 5. Error Output ==="

# Errors should always be shown
test_output "error shown in silent" "git-mind link" "Error:" "yes"
test_output "error shown in verbose" "git-mind -v link" "Error:" "yes"
test_output "error shown in porcelain" "git-mind --porcelain link" "Error:" "yes"

# Cleanup
cd ..
rm -rf "$TEST_DIR"

echo
echo "Summary: $PASSED passed, $FAILED failed"

if [ $FAILED -gt 0 ]; then
    exit 1
fi