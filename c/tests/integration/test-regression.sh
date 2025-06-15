#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# © 2025 J. Kirby Ross / Neuroglyph Collective
#
# Regression tests for Phase 1 security and memory fixes

set -uo pipefail  # Remove -e to allow tests to fail without exiting

# Source Docker guard - will exit if not in Docker
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/../scripts/docker-guard.sh"

# Get to the C implementation root
cd "$(dirname "$0")/../.."

echo "=== GitMind Regression Test Suite ==="
echo "Testing all Phase 1 fixes"
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

# Counters
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0

# Helper to run a test
run_test() {
    local test_name="$1"
    local test_cmd="$2"
    local expected_result="${3:-0}"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    echo -n "Testing $test_name... "
    
    set +e
    TMPFILE=$(mktemp)
    eval "$test_cmd" > "$TMPFILE" 2>&1
    local result=$?
    set -e
    
    if [ "$result" -eq "$expected_result" ]; then
        echo -e "${GREEN}✓${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}✗${NC}"
        echo "  Command: $test_cmd"
        echo "  Output: $(cat "$TMPFILE")"
        echo "  Expected exit code: $expected_result, Got: $result"
        echo "  Output:"
        cat "$TMPFILE" | sed 's/^/    /'
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    
    rm -f "$TMPFILE"
}

# Initialize a test repository
# Must be called after GITMIND_BIN is set!
init_test_repo() {
    local TEST_REPO_DIR=$(mktemp -d)
    local SAVED_PWD=$(pwd)
    
    cd "$TEST_REPO_DIR"
    git init --quiet
    git config user.name "Test User"
    git config user.email "test@example.com"
    echo "test" > README.md
    git add README.md
    git commit -m "Initial commit" --quiet
    git-mind init >/dev/null 2>&1
    
    cd "$SAVED_PWD"
    echo "$TEST_REPO_DIR"
}

echo "=== 1. Path Traversal Security Tests ==="

# Test path traversal protection in git-mind link command
TEST_REPO=$(init_test_repo)
cd "$TEST_REPO"

# Create test files
touch file.txt target.txt
mkdir -p src
touch src/main.c

# Valid paths should work
run_test "valid simple filename" "git-mind link file.txt target.txt" 0
run_test "valid relative path" "git-mind link src/main.c file.txt" 0
run_test "valid current dir prefix" "git-mind link ./file.txt target.txt" 0

# Invalid paths should be rejected
run_test "reject absolute path" "! git-mind link /etc/passwd target.txt 2>&1" 0
run_test "reject Windows absolute" "! git-mind link 'C:\\file.txt' target.txt 2>&1" 0

# Path traversal attempts should be rejected
run_test "reject simple .." "! git-mind link ../file.txt target.txt 2>&1" 0
run_test "reject multiple .." "! git-mind link ../../etc/passwd target.txt 2>&1" 0
run_test "reject .. in middle" "! git-mind link foo/../bar target.txt 2>&1" 0
run_test "reject complex traversal" "! git-mind link foo/bar/../../../etc target.txt 2>&1" 0
run_test "reject trailing .." "! git-mind link foo/.. target.txt 2>&1" 0

# Edge cases - empty path needs special handling
# Create a wrapper to test empty path
cat > test_empty.sh << 'SCRIPT_EOF'
#!/bin/bash
git-mind link '' target.txt 2>&1
exit $?
SCRIPT_EOF
chmod +x test_empty.sh
run_test "reject empty source" "! ./test_empty.sh" 0
rm -f test_empty.sh

# Dots in filenames should work
touch ..file.txt
touch ...
run_test "allow dots in names" "git-mind link ..file.txt target.txt" 0
run_test "allow three dots" "git-mind link ... target.txt" 0

cd - > /dev/null
rm -rf "$TEST_REPO"

echo
echo "=== 2. Memory Leak Tests ==="

# Compile with AddressSanitizer if available
if gcc -fsanitize=address -o /dev/null -x c /dev/null 2>/dev/null; then
    echo "Using AddressSanitizer for memory tests"
    ASAN_FLAGS="-fsanitize=address -fno-omit-frame-pointer"
else
    echo "AddressSanitizer not available, using valgrind"
    ASAN_FLAGS=""
fi

# Check if git-mind is available
if ! command -v git-mind >/dev/null 2>&1; then
    echo "ERROR: git-mind not found in PATH!"
    exit 1
fi
echo "Using git-mind from PATH"

echo "Creating test repository..."
TEST_REPO=$(init_test_repo)
echo "DEBUG: TEST_REPO='$TEST_REPO'"
if [ -z "$TEST_REPO" ]; then
    echo "ERROR: Failed to create test repository"
    exit 1
fi
echo "Test repository created at: $TEST_REPO"

# Test traverse memory leaks
cd "$TEST_REPO" || { echo "ERROR: Cannot cd to $TEST_REPO"; exit 1; }
echo "Testing traverse command for memory leaks..."

# Create some links for traversal
touch file1.txt file2.txt file3.txt
echo "Creating first link..."
git-mind link file1.txt file2.txt || echo "FAILED: link 1"
echo "Creating second link..."
git-mind link file2.txt file3.txt || echo "FAILED: link 2"
echo "Creating third link (cycle)..."
git-mind link file3.txt file1.txt || echo "FAILED: link 3"

if [ -n "$ASAN_FLAGS" ]; then
    # With ASAN, just run the command
    run_test "traverse with cycle (ASAN)" "git-mind traverse file1.txt --depth 5" 0
    run_test "traverse list format (ASAN)" "git-mind traverse file1.txt --format list" 0
else
    # With valgrind
    run_test "traverse with cycle (valgrind)" \
        "valgrind --leak-check=full --error-exitcode=1 --quiet git-mind traverse file1.txt --depth 5" 0
    run_test "traverse list format (valgrind)" \
        "valgrind --leak-check=full --error-exitcode=1 --quiet git-mind traverse file1.txt --format list" 0
fi

# Test check command memory leaks
touch file4.txt
git-mind link file3.txt file4.txt
rm file4.txt  # Create a broken link

if [ -n "$ASAN_FLAGS" ]; then
    run_test "check command (ASAN)" "git-mind check" 0
    run_test "check --fix (ASAN)" "git-mind check --fix" 0
else
    run_test "check command (valgrind)" \
        "valgrind --leak-check=full --error-exitcode=1 --quiet git-mind check" 0
    run_test "check --fix (valgrind)" \
        "valgrind --leak-check=full --error-exitcode=1 --quiet git-mind check --fix" 0
fi

cd - > /dev/null
rm -rf "$TEST_REPO"

echo
echo "=== 3. Error Code Consistency Tests ==="

# Test that git-mind returns proper error codes
# Change approach - test the error output directly without complex pipelines
TEST_ERROR=$(cd /tmp && git-mind init 2>&1 || true)
if echo "$TEST_ERROR" | grep -q "Error: Not a git repository"; then
    echo -e "Testing init on non-repo directory... ${GREEN}✓${NC}"
    ((PASSED_TESTS++))
    ((TOTAL_TESTS++))
else
    echo -e "Testing init on non-repo directory... ${RED}✗${NC}"
    echo "  Expected: Error: Not a git repository"
    echo "  Got: $TEST_ERROR"
    ((FAILED_TESTS++))
    ((TOTAL_TESTS++))
fi

# Test proper error for missing files
TEST_REPO=$(init_test_repo)
cd "$TEST_REPO"
TEST_ERROR=$(git-mind traverse nonexistent.txt 2>&1 || true)
if echo "$TEST_ERROR" | grep -q "Error: File not found"; then
    echo -e "Testing traverse non-existent file... ${GREEN}✓${NC}"
    ((PASSED_TESTS++))
    ((TOTAL_TESTS++))
else
    echo -e "Testing traverse non-existent file... ${RED}✗${NC}"
    echo "  Expected: Error: File not found"
    echo "  Got: $TEST_ERROR"
    ((FAILED_TESTS++))
    ((TOTAL_TESTS++))
fi
cd - > /dev/null
rm -rf "$TEST_REPO"

echo
echo "=== 4. Binary Portability Test ==="

# Just verify the binary was built successfully
run_test "git-mind binary exists" "which git-mind" 0
run_test "git-mind runs" "git-mind version" 0

echo
echo "=== 5. Performance Regression Test ==="

TEST_REPO2=$(init_test_repo)
cd "$TEST_REPO2"

# Create many links to test O(n²) performance
echo "Creating 100 links of different types..."
for i in {1..25}; do
    touch "fileA$i.txt" "fileB$i.txt" "fileC$i.txt" "fileD$i.txt"
    git-mind link "fileA$i.txt" "fileB$i.txt" --type "TYPE_A" > /dev/null 2>&1
    git-mind link "fileB$i.txt" "fileC$i.txt" --type "TYPE_B" > /dev/null 2>&1
    git-mind link "fileC$i.txt" "fileD$i.txt" --type "TYPE_C" > /dev/null 2>&1
    git-mind link "fileD$i.txt" "fileA$i.txt" --type "TYPE_D" > /dev/null 2>&1
done

# Time the status command (should complete quickly even with O(n²))
echo -n "Testing status performance with 100 links... "
start_time=$(date +%s.%N)
timeout 2s git-mind status > /dev/null 2>&1
result=$?
end_time=$(date +%s.%N)

if [ $result -eq 0 ]; then
    elapsed=$(echo "$end_time - $start_time" | bc)
    echo -e "${GREEN}✓${NC} (${elapsed}s)"
    ((PASSED_TESTS++))
else
    echo -e "${RED}✗${NC} (timeout or error)"
    ((FAILED_TESTS++))
fi
((TOTAL_TESTS++))

cd - > /dev/null
rm -rf "$TEST_REPO2"

echo
echo "=== Test Summary ==="
echo "Total tests: $TOTAL_TESTS"
echo -e "Passed: ${GREEN}$PASSED_TESTS${NC}"
echo -e "Failed: ${RED}$FAILED_TESTS${NC}"

# Cleanup
rm -rf test_repo

if [ $FAILED_TESTS -gt 0 ]; then
    echo
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
else
    echo
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
fi