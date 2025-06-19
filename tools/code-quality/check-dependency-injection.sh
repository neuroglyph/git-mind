#!/bin/bash
# SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0
# © 2025 J. Kirby Ross / Neuroglyph Collective

# Check for dependency injection violations

set -e

VIOLATIONS=0

echo "Checking for direct system calls..."

# List of system calls that should be injected
SYSTEM_CALLS="(time|clock_gettime|rand|srand|fopen|fclose|fread|fwrite|mkdir|rmdir|unlink|system|popen)"

# Check for direct system calls
find src -name "*.c" | while read -r file; do
    # Skip test files and default implementations
    if [[ "$file" == *"test"* ]] || [[ "$file" == *"_default.c" ]]; then
        continue
    fi
    
    # Look for direct system calls
    if grep -Hn -E "${SYSTEM_CALLS}\s*\(" "$file" | \
       grep -v '(ctx->|ops->|->io->|->time->|->random->)' | \
       grep -v '// OK:' ; then
        echo "❌ $file: Direct system call without injection"
        VIOLATIONS=$((VIOLATIONS + 1))
    fi
done

echo "Checking for direct Git operations..."

# Check for direct git_* calls outside of Git abstraction layer
find src -name "*.c" | grep -v "src/git" | while read -r file; do
    if grep -Hn 'git_[a-z_]*\s*(' "$file" | \
       grep -v '(ctx->git|git_ops->)' | \
       grep -v '// OK:' ; then
        echo "❌ $file: Direct Git operation without abstraction"
        VIOLATIONS=$((VIOLATIONS + 1))
    fi
done

if [ $VIOLATIONS -gt 0 ]; then
    echo "❌ Found $VIOLATIONS dependency injection violations"
    echo "Use injected providers instead of direct system calls"
    exit 1
else
    echo "✅ Dependency injection properly used"
fi