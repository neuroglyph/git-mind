#!/bin/bash
# SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0
# © 2025 J. Kirby Ross / Neuroglyph Collective

# Check for violations of output control (--verbose/--porcelain)

set -e

VIOLATIONS=0

echo "Checking for direct output in CLI commands..."

# Check CLI files for direct printf/fprintf
find src/cli -name "*.c" | while read -r file; do
    # Skip main.c usage reporting
    if [[ "$file" == *"main.c" ]] && grep -q "usage()" "$file"; then
        continue
    fi
    
    # Look for direct output functions
    if grep -Hn -E '(printf|fprintf|puts|fputs|putchar)\s*\(' "$file" | \
       grep -v '(gm_output|output->|ctx->output)' | \
       grep -v '// OK:' ; then
        echo "❌ $file: Direct output without output control"
        VIOLATIONS=$((VIOLATIONS + 1))
    fi
done

echo "Checking library code for output violations..."

# Check non-CLI files - they should NEVER output
find src -name "*.c" | grep -v "src/cli" | while read -r file; do
    # Skip error.c which is allowed to log
    if [[ "$file" == *"error.c" ]]; then
        continue
    fi
    
    # Library code should be silent
    if grep -Hn -E '(printf|fprintf|puts|fputs|putchar|perror)\s*\(' "$file" | \
       grep -v '// OK:' ; then
        echo "❌ $file: Library code should be silent!"
        VIOLATIONS=$((VIOLATIONS + 1))
    fi
done

if [ $VIOLATIONS -gt 0 ]; then
    echo "❌ Found $VIOLATIONS output control violations"
    echo "CLI code must use gm_output_t, library code must be silent"
    exit 1
else
    echo "✅ Output control properly implemented"
fi