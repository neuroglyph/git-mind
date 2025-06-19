#!/bin/bash
# SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0
# © 2025 J. Kirby Ross / Neuroglyph Collective

# Use clang-tidy for all checks it can handle

set -e

echo "=== Running clang-tidy checks ==="

# Check if clang-tidy is available
if ! command -v clang-tidy &> /dev/null; then
    echo "❌ clang-tidy not found. Rebuild Docker image with:"
    echo "   docker compose build"
    exit 1
fi

FAILED=0

# Run clang-tidy on all C files
find src -name "*.c" | while read -r file; do
    echo -n "Checking $file... "
    
    # Run clang-tidy with our config
    if clang-tidy "$file" -- -I./include 2>&1 | grep -E "(warning|error)" > /tmp/clang-tidy-$$.log; then
        echo "❌ Issues found:"
        cat /tmp/clang-tidy-$$.log
        FAILED=$((FAILED + 1))
    else
        echo "✅"
    fi
done

rm -f /tmp/clang-tidy-$$*.log

if [ $FAILED -gt 0 ]; then
    echo "❌ clang-tidy found issues in $FAILED files"
    exit 1
else
    echo "✅ All files pass clang-tidy checks"
fi