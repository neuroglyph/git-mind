#!/bin/bash
# SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0
# © 2025 J. Kirby Ross / Neuroglyph Collective

# Comprehensive linting with multiple tools

set -e

echo "=== Git-Mind Lint Suite ==="
echo

FAILED=0

# Check if tools are installed
check_tool() {
    if ! command -v "$1" &> /dev/null; then
        echo "⚠️  $1 not found. Install with: $2"
        return 1
    fi
    return 0
}

# 1. Clang-format check
if check_tool "clang-format" "apt install clang-format"; then
    echo ">>> Running clang-format check..."
    if find src -name "*.c" -o -name "*.h" | \
       xargs clang-format --dry-run --Werror 2>&1 | \
       grep -q "error"; then
        echo "❌ Formatting issues found. Run: make format"
        FAILED=$((FAILED + 1))
    else
        echo "✅ Code formatting OK"
    fi
    echo
fi

# 2. Clang-tidy check  
if check_tool "clang-tidy" "apt install clang-tidy"; then
    echo ">>> Running clang-tidy..."
    # Run on a few key files to test
    if ! clang-tidy src/cli/main.c -- -I./include 2>/dev/null; then
        echo "❌ Clang-tidy found issues"
        FAILED=$((FAILED + 1))
    else
        echo "✅ Clang-tidy passed"
    fi
    echo
fi

# 3. Cppcheck
if check_tool "cppcheck" "apt install cppcheck"; then
    echo ">>> Running cppcheck..."
    if ! cppcheck --enable=all --error-exitcode=1 \
         --suppress=missingIncludeSystem \
         --suppress=unusedFunction \
         -I./include \
         src/ 2>&1 | tee /tmp/cppcheck.log; then
        echo "❌ Cppcheck found issues"
        FAILED=$((FAILED + 1))
    else
        echo "✅ Cppcheck passed"
    fi
    echo
fi

# 4. PC-lint Plus (if available)
if check_tool "pclp" "commercial tool - see gimpel.com"; then
    echo ">>> Running PC-lint Plus..."
    # PC-lint configuration would go here
    echo "⚠️  PC-lint Plus configuration needed"
fi

# 5. Our custom checks
echo ">>> Running custom Git-Mind checks..."
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if ! "$SCRIPT_DIR/run-all-checks.sh"; then
    FAILED=$((FAILED + 1))
fi

# Summary
echo
echo "=== Lint Summary ==="
if [ $FAILED -eq 0 ]; then
    echo "✅ All lint checks passed!"
else
    echo "❌ $FAILED lint tools reported issues"
    exit 1
fi