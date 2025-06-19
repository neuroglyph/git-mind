#!/bin/bash
# SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0
# © 2025 J. Kirby Ross / Neuroglyph Collective

# Check C function length - enforce 15 line maximum

set -e

MAX_LINES=15
VIOLATIONS=0

# Find all C source files
find src -name "*.c" | while read -r file; do
    # Use awk to find functions and count their lines
    awk -v max="$MAX_LINES" -v file="$file" '
    /^[a-zA-Z_][a-zA-Z0-9_]*\s*\(.*\)\s*{/ || /^static\s+[a-zA-Z_][a-zA-Z0-9_]*\s+[a-zA-Z_][a-zA-Z0-9_]*\s*\(.*\)\s*{/ {
        # Found function start
        func_name = $0
        gsub(/\s*{.*/, "", func_name)
        gsub(/^.*\s+/, "", func_name)
        gsub(/\(.*/, "", func_name)
        
        start_line = NR
        brace_count = 1
        line_count = 0
        
        # Count lines until closing brace
        while (brace_count > 0 && getline > 0) {
            line_count++
            # Count braces
            for (i = 1; i <= length($0); i++) {
                char = substr($0, i, 1)
                if (char == "{") brace_count++
                else if (char == "}") brace_count--
            }
        }
        
        if (line_count > max) {
            printf "%s:%d: %s() is %d lines (max: %d)\n", 
                   file, start_line, func_name, line_count, max
            exit 1
        }
    }
    ' "$file" || VIOLATIONS=$((VIOLATIONS + 1))
done

if [ $VIOLATIONS -gt 0 ]; then
    echo "❌ Found $VIOLATIONS functions exceeding $MAX_LINES lines"
    exit 1
else
    echo "✅ All functions are $MAX_LINES lines or less"
fi