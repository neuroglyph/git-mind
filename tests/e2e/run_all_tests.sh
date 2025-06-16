#!/bin/bash
# SPDX-License-Identifier: Apache-2.0

# Run all E2E tests

set -e

SCRIPT_DIR="$(dirname "$0")"
cd "$SCRIPT_DIR"

# Colors
if [ -t 1 ]; then
    BLUE='\033[0;34m'
    GREEN='\033[0;32m'
    RED='\033[0;31m'
    NC='\033[0m'
else
    BLUE=''
    GREEN=''
    RED=''
    NC=''
fi

echo -e "${BLUE}===================================="
echo "git-mind End-to-End Test Suite"
echo "====================================${NC}"
echo ""

# Check if git-mind is built
if [ ! -x "../../git-mind" ]; then
    echo -e "${RED}ERROR: git-mind not built!${NC}"
    echo "Run 'make build' first"
    exit 1
fi

# Export binary location
export GIT_MIND="$(cd ../.. && pwd)/git-mind"

# Run test suites
FAILED=0

echo -e "${BLUE}Running Core Functionality Tests...${NC}"
if bash 01_core_functionality.sh; then
    echo -e "${GREEN}✓ Core tests passed${NC}"
else
    echo -e "${RED}✗ Core tests failed${NC}"
    FAILED=1
fi

echo ""
echo -e "${BLUE}Running Edge Case Tests...${NC}"
if bash 02_edge_cases.sh; then
    echo -e "${GREEN}✓ Edge case tests passed${NC}"
else
    echo -e "${RED}✗ Edge case tests failed${NC}"
    FAILED=1
fi

echo ""
echo -e "${BLUE}Running Performance Benchmarks...${NC}"
if bash 03_performance_benchmarks.sh; then
    echo -e "${GREEN}✓ Benchmarks passed${NC}"
else
    echo -e "${RED}✗ Benchmarks failed${NC}"
    FAILED=1
fi

# Summary
echo ""
echo -e "${BLUE}===================================="
if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}ALL TESTS PASSED!${NC}"
    echo -e "git-mind is ready to ship! 🚀"
else
    echo -e "${RED}SOME TESTS FAILED!${NC}"
    echo -e "Fix the failures before shipping."
fi
echo -e "====================================${NC}"

exit $FAILED