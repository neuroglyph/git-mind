#!/bin/bash
# SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0
# © 2025 J. Kirby Ross / Neuroglyph Collective
#
# THE MIGRATION SCRIPT OF DOOM
# Touch it = Fix it COMPLETELY. No exceptions.
#

set -e

if [ $# -lt 2 ]; then
    echo "Usage: $0 <module-name> <destination>"
    echo "Example: $0 edge core/src/"
    echo "Example: $0 cli apps/cli/src/"
    exit 1
fi

MODULE=$1
DEST=$2

echo "🔥 MIGRATION SCRIPT OF DOOM 🔥"
echo "Module: $MODULE"
echo "Destination: $DEST"
echo ""

# Find all files for this module
echo "📋 Finding files for module '$MODULE'..."
MODULE_FILES=$(find src -name "${MODULE}*.[ch]" -o -path "*/src/${MODULE}/*" -name "*.[ch]" | sort)

if [ -z "$MODULE_FILES" ]; then
    echo "❌ No files found for module '$MODULE'"
    exit 1
fi

echo "Found files:"
echo "$MODULE_FILES" | sed 's/^/  - /'
echo ""

# Count current warnings
echo "📊 Analyzing current code quality..."
TOTAL_WARNINGS=0
for file in $MODULE_FILES; do
    if [[ "$file" == *.c ]]; then
        WARNINGS=$(docker compose run --rm -T dev bash -c "clang-tidy /workspace/$file -- -I/workspace/include 2>&1" | grep -c "warning:" || true)
        echo "  $file: $WARNINGS warnings"
        TOTAL_WARNINGS=$((TOTAL_WARNINGS + WARNINGS))
    fi
done
echo "Total warnings to fix: $TOTAL_WARNINGS"
echo ""

if [ $TOTAL_WARNINGS -gt 0 ]; then
    echo "⚠️  This module has $TOTAL_WARNINGS warnings that MUST be fixed!"
    echo ""
fi

# Create destination directory
mkdir -p "$DEST"

# Copy files to destination
echo "📦 Copying files to $DEST..."
for file in $MODULE_FILES; do
    cp "$file" "$DEST/$(basename "$file")"
    echo "  ✓ $(basename "$file")"
done
echo ""

# Now the real work begins
echo "🔨 TIME TO FIX ALL THE THINGS!"
echo ""
echo "You MUST fix ALL of these issues:"
echo "  1. ⚡ Run clang-format on all files"
echo "  2. 📏 Split any function > 15 lines"
echo "  3. 🔢 Replace ALL magic numbers with constants"
echo "  4. 📝 Fix ALL naming convention issues"
echo "  5. 🧪 Write comprehensive tests"
echo "  6. 📖 Add proper documentation"
echo ""

# Format files
echo "🎨 Auto-formatting files..."
for file in "$DEST"/*.[ch]; do
    if [ -f "$file" ]; then
        docker compose run --rm -T dev bash -c "clang-format -i /workspace/$file"
        echo "  ✓ Formatted $(basename "$file")"
    fi
done
echo ""

# Check with strict clang-tidy
echo "🔍 Checking against STRICT standards..."
FAILED=0
for file in "$DEST"/*.c; do
    if [ -f "$file" ]; then
        echo -n "  Checking $(basename "$file")... "
        if docker compose run --rm -T dev bash -c "clang-tidy /workspace/$file --config-file=/workspace/quality/.clang-tidy -- -I/workspace/include" 2>&1 | grep -E "warning:|error:" > /dev/null; then
            echo "❌ FAILED"
            FAILED=1
            # Show the issues
            docker compose run --rm -T dev bash -c "clang-tidy /workspace/$file --config-file=/workspace/quality/.clang-tidy -- -I/workspace/include 2>&1" | grep -E "warning:|error:" | head -10
            echo "  ... and more"
        else
            echo "✅ CLEAN!"
        fi
    fi
done
echo ""

if [ $FAILED -eq 1 ]; then
    echo "❌ Module does NOT meet quality standards!"
    echo ""
    echo "📋 TODO List:"
    echo "  1. Fix all clang-tidy warnings"
    echo "  2. Split large functions (use helper functions)"
    echo "  3. Define constants for magic values"
    echo "  4. Fix naming issues"
    echo ""
    echo "Run this to see all issues:"
    echo "  docker compose run --rm dev bash -c \"clang-tidy /workspace/$DEST/*.c --config-file=/workspace/quality/.clang-tidy -- -I/workspace/include\""
    echo ""
    echo "After fixing, run:"
    echo "  $0 $MODULE $DEST"
    echo ""
    echo "Remember: We're going from 11,000 warnings to ZERO. No compromises!"
else
    echo "✅ Module is CLEAN! You've slain the quality dragon!"
    echo ""
    echo "Next steps:"
    echo "  1. Write comprehensive tests in ${DEST}/../tests/"
    echo "  2. Update documentation"
    echo "  3. Remove original files from src/"
    echo "  4. Commit with: git commit -m \"refactor($MODULE): migrate to $DEST with full cleanup\""
fi

echo ""
echo "🎯 Progress: 1 module down, many to go!"
echo "🔥 11,000 warnings won't fix themselves!"