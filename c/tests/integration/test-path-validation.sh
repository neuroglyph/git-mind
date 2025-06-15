#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# © 2025 J. Kirby Ross / Neuroglyph Collective

set -uo pipefail  # Remove -e to allow tests to fail without exiting

# Source Docker guard - will exit if not in Docker
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/../scripts/docker-guard.sh"

echo "=== Path Validation Security Tests ==="
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

FAILED=0
PASSED=0

# Test function that uses git-mind link command
test_path() {
    local source_path="$1"
    local should_fail="$2"
    local description="$3"
    local error_pattern="$4"
    
    # Create test repo
    TEST_REPO=$(mktemp -d)
    cd "$TEST_REPO"
    git init --quiet
    git config user.name "Test User"
    git config user.email "test@example.com"
    touch README.md target.txt
    git add .
    git commit -m "Initial" --quiet
    git-mind init >/dev/null 2>&1
    
    # Create source file if it's a valid path
    if [[ ! "$source_path" =~ \.\. ]] && [[ ! "$source_path" =~ ^/ ]] && [[ -n "$source_path" ]]; then
        # Create directory structure if needed
        dir=$(dirname "$source_path")
        if [ "$dir" != "." ] && [ "$dir" != "" ]; then
            mkdir -p "$dir"
        fi
        touch "$source_path"
    fi
    
    echo -n "Testing $description... "
    
    # Run the link command
    output=$(git-mind link "$source_path" target.txt 2>&1)
    result=$?
    
    if [ "$should_fail" = "1" ]; then
        # Should fail
        if [ $result -ne 0 ] && echo "$output" | grep -q "$error_pattern"; then
            echo -e "${GREEN}✓${NC} (correctly rejected)"
            ((PASSED++))
        else
            echo -e "${RED}✗${NC} (should have been rejected)"
            echo "  Output: $output"
            ((FAILED++))
        fi
    else
        # Should succeed
        if [ $result -eq 0 ]; then
            echo -e "${GREEN}✓${NC}"
            ((PASSED++))
        else
            echo -e "${RED}✗${NC} (should have succeeded)"
            echo "  Output: $output"
            ((FAILED++))
        fi
    fi
    
    cd - >/dev/null
    rm -rf "$TEST_REPO"
}

echo "Testing path traversal protection..."
echo

# Valid paths
test_path "file.txt" 0 "simple filename" ""
test_path "src/main.c" 0 "nested path" ""
test_path "./file.txt" 0 "current directory prefix" ""
test_path "dir/file.txt" 0 "directory path" ""

# Invalid: Absolute paths
test_path "/etc/passwd" 1 "absolute Unix path" "Absolute paths not allowed"
test_path "/root/.ssh/id_rsa" 1 "sensitive absolute path" "Absolute paths not allowed"
test_path "C:\\Windows\\System32" 1 "Windows absolute path" "Absolute paths not allowed"

# Invalid: Parent directory references
test_path "../file" 1 "parent directory" "Path traversal not allowed"
test_path "../../etc/passwd" 1 "multiple parent refs" "Path traversal not allowed"
test_path "foo/../bar" 1 "parent ref in middle" "Path traversal not allowed"
test_path "foo/bar/../../../etc" 1 "complex traversal" "Path traversal not allowed"
test_path "foo/.." 1 "trailing parent ref" "Path traversal not allowed"
test_path ".." 1 "just parent ref" "Path traversal not allowed"
test_path "../../../../../../../etc/passwd" 1 "many parent refs" "Path traversal not allowed"

# Edge cases
test_path "" 1 "empty path" "Empty path"
test_path "..foo" 0 "dots at start of name" ""
test_path "foo..bar" 0 "dots in middle of name" ""
test_path "..." 0 "three dots" ""
test_path "...." 0 "four dots" ""

echo
echo "Summary:"
echo "  Passed: $PASSED"
echo "  Failed: $FAILED"

if [ $FAILED -gt 0 ]; then
    exit 1
fi

echo
echo -e "${GREEN}All path validation tests passed!${NC}"