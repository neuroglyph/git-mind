#!/bin/bash
# SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0
# © 2025 J. Kirby Ross / Neuroglyph Collective

# Check test quality - ensure behavior testing, not implementation

set -e

VIOLATIONS=0

echo "Checking shell tests for stdout/stderr assertions..."

# Check shell tests for output checking
find tests -name "*.sh" | while read -r file; do
    # Look for grep on command output (bad practice)
    if grep -Hn '\$([^)]*git.mind[^)]*) *| *grep' "$file" | \
       grep -v '# OK:' ; then
        echo "❌ $file: Testing stdout with grep"
        VIOLATIONS=$((VIOLATIONS + 1))
    fi
    
    # Look for echo "$output" patterns
    if grep -Hn 'echo.*\$[a-z_]*.*| *grep' "$file" | \
       grep -v '# OK:' ; then
        echo "❌ $file: Testing captured output"
        VIOLATIONS=$((VIOLATIONS + 1))
    fi
done

echo "Checking C tests for printf usage..."

# Check C tests for printf (should use assertions)
find tests -name "*.c" | while read -r file; do
    if grep -Hn 'printf\s*(' "$file" | \
       grep -v '// OK:' ; then
        echo "❌ $file: Tests should not use printf"
        VIOLATIONS=$((VIOLATIONS + 1))
    fi
done

echo "Checking for test organization..."

# Check if tests are organized by feature
if [ ! -d "tests/features" ]; then
    echo "⚠️  Tests not organized by feature (tests/features/ missing)"
    VIOLATIONS=$((VIOLATIONS + 1))
fi

if [ $VIOLATIONS -gt 0 ]; then
    echo "❌ Found $VIOLATIONS test quality violations"
    echo "Tests should check behavior/state, not output"
    exit 1
else
    echo "✅ Test quality checks passed"
fi