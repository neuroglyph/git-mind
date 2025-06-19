#!/bin/bash
# SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0
# © 2025 J. Kirby Ross / Neuroglyph Collective

# Run all checks using established tools instead of custom scripts

set -e

echo "=== Git-Mind Code Quality Checks (v2) ==="
echo "Using industry-standard tools instead of custom scripts"
echo

FAILED=0
REPORT_DIR="quality-reports"
mkdir -p "$REPORT_DIR"

# 1. Function length and complexity - USE CLANG-TIDY
if command -v clang-tidy &> /dev/null; then
    echo ">>> Checking function length with clang-tidy..."
    
    # Generate compile_commands.json for better analysis
    if command -v bear &> /dev/null; then
        bear -- make -C src clean all &> /dev/null || true
    fi
    
    # Run clang-tidy on all source files
    find src -name "*.c" -o -name "*.h" | \
    xargs clang-tidy -checks='readability-function-size,google-readability-function-size' \
                      -config-file=.clang-tidy \
                      -- -I./include 2>&1 | \
    tee "$REPORT_DIR/clang-tidy-function-size.log"
    
    if grep -q "warning:" "$REPORT_DIR/clang-tidy-function-size.log"; then
        echo "❌ Function size violations found"
        FAILED=$((FAILED + 1))
    else
        echo "✅ All functions within size limits"
    fi
    echo
else
    echo "⚠️  clang-tidy not found - skipping function size check"
fi

# 2. Magic numbers - USE CLANG-TIDY
if command -v clang-tidy &> /dev/null; then
    echo ">>> Checking magic numbers with clang-tidy..."
    
    find src -name "*.c" | \
    xargs clang-tidy -checks='cppcoreguidelines-avoid-magic-numbers' \
                      -config-file=.clang-tidy \
                      -- -I./include 2>&1 | \
    tee "$REPORT_DIR/clang-tidy-magic-numbers.log"
    
    if grep -q "warning:" "$REPORT_DIR/clang-tidy-magic-numbers.log"; then
        echo "❌ Magic numbers found"
        FAILED=$((FAILED + 1))
    else
        echo "✅ No magic numbers"
    fi
    echo
fi

# 3. General code quality - USE CPPCHECK
if command -v cppcheck &> /dev/null; then
    echo ">>> Running cppcheck..."
    
    cppcheck --enable=all \
             --error-exitcode=1 \
             --suppress=missingIncludeSystem \
             --suppress=unusedFunction \
             --inline-suppr \
             -I./include \
             --report-progress \
             src/ 2>&1 | tee "$REPORT_DIR/cppcheck.log"
             
    if [ ${PIPESTATUS[0]} -ne 0 ]; then
        echo "❌ Cppcheck found issues"
        FAILED=$((FAILED + 1))
    else
        echo "✅ Cppcheck passed"
    fi
    echo
fi

# 4. Complexity analysis - USE LIZARD
if command -v lizard &> /dev/null; then
    echo ">>> Checking cyclomatic complexity with lizard..."
    
    lizard src/ -C 10 -L 15 -a 5 -E ns -w 2>&1 | tee "$REPORT_DIR/lizard.log"
    
    if [ ${PIPESTATUS[0]} -ne 0 ]; then
        echo "❌ Complexity violations found"
        FAILED=$((FAILED + 1))
    else
        echo "✅ Complexity within limits"
    fi
    echo
fi

# 5. Security issues - USE FLAWFINDER
if command -v flawfinder &> /dev/null; then
    echo ">>> Checking security with flawfinder..."
    
    flawfinder --minlevel=2 --quiet src/ 2>&1 | tee "$REPORT_DIR/flawfinder.log"
    
    if [ -s "$REPORT_DIR/flawfinder.log" ]; then
        echo "❌ Security issues found"
        FAILED=$((FAILED + 1))
    else
        echo "✅ No security issues"
    fi
    echo
fi

# 6. Our custom checks that don't have tool equivalents
echo ">>> Running git-mind specific checks..."

# Output control check (specific to our --verbose/--porcelain requirement)
./tools/code-quality/check-output-control.sh || FAILED=$((FAILED + 1))

# Dependency injection check (specific to our DI requirements)  
./tools/code-quality/check-dependency-injection.sh || FAILED=$((FAILED + 1))

# Test quality check (specific to our behavior testing requirement)
./tools/code-quality/check-test-quality.sh || FAILED=$((FAILED + 1))

echo

# Summary
echo "=== Quality Check Summary ==="
if [ $FAILED -eq 0 ]; then
    echo "✅ All quality checks passed!"
    echo
    echo "Reports saved in: $REPORT_DIR/"
else
    echo "❌ $FAILED quality checks failed"
    echo
    echo "Fix issues by running:"
    echo "  make format         # Auto-fix formatting"
    echo "  make fix-tech-debt  # See manual fixes needed"
    echo
    echo "Reports saved in: $REPORT_DIR/"
    exit 1
fi