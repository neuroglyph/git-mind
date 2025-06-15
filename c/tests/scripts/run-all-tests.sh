#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# Master test runner - runs ALL test suites

set -e

# Source Docker guard - will exit if not in Docker
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "$SCRIPT_DIR/docker-guard.sh"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo "🧪 === GitMind Complete Test Suite ==="
echo ""

# Track results
FAILED_TESTS=()
PASSED_TESTS=()

# Check for random order flag
RANDOM_ORDER=false
if [ "${1:-}" = "--random" ] || [ "${RANDOM_TESTS:-}" = "1" ]; then
    RANDOM_ORDER=true
    echo -e "${YELLOW}🎲 Running tests in random order${NC}"
    echo ""
fi

# Function to run a test suite
run_test() {
    local test_name="$1"
    local test_script="$2"
    
    echo -e "${YELLOW}>>> Running $test_name...${NC}"
    
    if [ ! -f "$test_script" ]; then
        echo -e "${RED}❌ Test script not found: $test_script${NC}"
        FAILED_TESTS+=("$test_name (not found)")
        return 1
    fi
    
    if ! [ -x "$test_script" ]; then
        echo -e "${YELLOW}⚠️  Making $test_script executable${NC}"
        chmod +x "$test_script"
    fi
    
    if $test_script; then
        echo -e "${GREEN}✅ $test_name passed${NC}"
        PASSED_TESTS+=("$test_name")
    else
        echo -e "${RED}❌ $test_name failed${NC}"
        FAILED_TESTS+=("$test_name")
    fi
    echo ""
}

# Define all test suites
INTEGRATION_DIR="$SCRIPT_DIR/../integration"
declare -a TEST_SUITES=(
    "Core Commands|$INTEGRATION_DIR/test.sh"
    "Graph Traversal|$INTEGRATION_DIR/test-traverse.sh"
    "Path Security|$INTEGRATION_DIR/test-path-validation.sh"
    "Output Modes|$INTEGRATION_DIR/test-output-modes.sh"
    "Regression Suite|$INTEGRATION_DIR/test-regression.sh"
    "Depth Errors|$INTEGRATION_DIR/test-depth-error.sh"
)

# Add memory test if valgrind available
if command -v valgrind >/dev/null 2>&1; then
    TEST_SUITES+=("Memory Leaks|$INTEGRATION_DIR/valgrind-test.sh")
else
    echo -e "${YELLOW}⚠️  Skipping memory tests (valgrind not available)${NC}"
fi

# Randomize order if requested
if [ "$RANDOM_ORDER" = true ]; then
    # Shuffle array using awk and RANDOM
    readarray -t TEST_SUITES < <(printf '%s\n' "${TEST_SUITES[@]}" | awk 'BEGIN{srand()} {print rand() "\t" $0}' | sort -n | cut -f2-)
fi

# Run all test suites
for suite in "${TEST_SUITES[@]}"; do
    IFS='|' read -r test_name test_script <<< "$suite"
    run_test "$test_name" "$test_script"
done

# Summary
echo "📊 === Test Summary ==="
echo -e "Passed: ${GREEN}${#PASSED_TESTS[@]}${NC} test suites"
echo -e "Failed: ${RED}${#FAILED_TESTS[@]}${NC} test suites"

if [ ${#PASSED_TESTS[@]} -gt 0 ]; then
    echo -e "\n${GREEN}✅ Passed tests:${NC}"
    for test in "${PASSED_TESTS[@]}"; do
        echo "   - $test"
    done
fi

if [ ${#FAILED_TESTS[@]} -gt 0 ]; then
    echo -e "\n${RED}❌ Failed tests:${NC}"
    for test in "${FAILED_TESTS[@]}"; do
        echo "   - $test"
    done
    echo ""
    echo "Please fix failing tests before pushing!"
    exit 1
fi

echo ""
echo -e "${GREEN}🎉 All test suites passed!${NC}"
echo ""

# Performance note
echo "💡 Tip: For quick testing during development:"
echo "   - Run './test.sh' for core functionality"
echo "   - Run './test-traverse.sh' for graph features"
echo "   - Run this script before pushing"