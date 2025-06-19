#!/bin/bash
# SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0
# © 2025 J. Kirby Ross / Neuroglyph Collective

# Check for magic numbers and strings in C code

set -e

VIOLATIONS=0
TEMP_FILE="/tmp/magic-values-$$"

# Patterns that indicate magic values
# Exclude: 0, 1, -1, NULL, array indices, bit shifts, sizeof
MAGIC_NUMBER_PATTERN='[^a-zA-Z0-9_]([2-9][0-9]+|[1-9][0-9]{2,})[^0-9]'

# Check for magic numbers
echo "Checking for magic numbers..."
find src -name "*.c" | while read -r file; do
    # Skip constants.h and similar files
    if [[ "$file" == *"constants"* ]]; then
        continue
    fi
    
    # Find potential magic numbers
    grep -Hn -E "$MAGIC_NUMBER_PATTERN" "$file" 2>/dev/null | \
    grep -v -E '(#define|<<|>>|case [0-9]+:|\.h:|//|/\*|\[0\]|\[1\])' | \
    grep -v -E '(malloc|calloc|realloc|memset|memcpy|snprintf|sizeof)' | \
    while read -r line; do
        echo "$line" >> "$TEMP_FILE"
        VIOLATIONS=$((VIOLATIONS + 1))
    done
done

# Check for magic strings (quoted literals not in #define)
echo "Checking for magic strings..."
find src -name "*.c" | while read -r file; do
    # Skip test files
    if [[ "$file" == *"test"* ]]; then
        continue
    fi
    
    # Find string literals not in #defines or error messages
    grep -Hn '"[^"]\{3,\}"' "$file" 2>/dev/null | \
    grep -v -E '(#define|printf|fprintf|sprintf|snprintf|error|Error|ERROR|fail|Failed|FAILED)' | \
    grep -v -E '(\.h:|//|/\*|\%[sdxu])' | \
    while read -r line; do
        # Check if it's a format string
        if echo "$line" | grep -q '%'; then
            continue
        fi
        echo "$line" >> "$TEMP_FILE"
        VIOLATIONS=$((VIOLATIONS + 1))
    done
done

# Report results
if [ -f "$TEMP_FILE" ] && [ -s "$TEMP_FILE" ]; then
    echo "❌ Found magic values:"
    cat "$TEMP_FILE"
    rm -f "$TEMP_FILE"
    exit 1
else
    echo "✅ No obvious magic values found"
    rm -f "$TEMP_FILE"
fi