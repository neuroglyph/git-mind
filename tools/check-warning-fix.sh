#!/bin/bash
# Script to verify a warning has been fixed and update baseline if successful

set -e

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Get the warning pattern to check (optional first argument)
WARNING_PATTERN="$1"

echo -e "${BLUE}Running clang-tidy in Docker...${NC}"
./tools/docker-clang-tidy.sh >/dev/null 2>&1

# If a warning pattern was provided, check if it still exists
if [ -n "$WARNING_PATTERN" ]; then
    echo -e "${BLUE}Checking for warning: ${WARNING_PATTERN}${NC}"
    if grep -q "$WARNING_PATTERN" clang-tidy-report.txt; then
        echo -e "${RED}❌ Warning still exists!${NC}"
        grep "$WARNING_PATTERN" clang-tidy-report.txt | head -5
        exit 1
    else
        echo -e "${GREEN}✅ Warning eliminated!${NC}"
    fi
fi

# Check warning count against baseline
RESULT=$(python3 tools/count_warnings.py check clang-tidy-report.txt tools/baseline_count.txt)
echo "$RESULT"

# Extract current count from the result
CURRENT_COUNT=$(echo "$RESULT" | grep -oE '[0-9]+' | head -1)
BASELINE_COUNT=$(cat tools/baseline_count.txt)

# If warnings reduced, update baseline
if [ "$CURRENT_COUNT" -lt "$BASELINE_COUNT" ]; then
    echo -e "${GREEN}Updating baseline from $BASELINE_COUNT to $CURRENT_COUNT${NC}"
    echo "$CURRENT_COUNT" > tools/baseline_count.txt
    echo -e "${GREEN}🤩 From $BASELINE_COUNT to $CURRENT_COUNT warnings! Well done.${NC}"
elif [ "$CURRENT_COUNT" -eq "$BASELINE_COUNT" ]; then
    echo -e "${BLUE}👍 No new warnings.${NC}"
else
    echo -e "${RED}😱🚨 RUH-ROH. New errors detected!${NC}"
    echo -e "${RED}From $BASELINE_COUNT to $CURRENT_COUNT warnings!${NC}"
    exit 1
fi