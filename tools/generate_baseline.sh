#!/bin/bash
# Generate clang-tidy baseline with proper filtering

set -e

echo "🔨 Generating clang-tidy baseline..."

# Run clang-tidy in Docker exactly like CI does
docker run --rm -v "$(pwd):/workspace" -w /workspace/core ubuntu:22.04 bash -c '
    # Install dependencies
    apt-get update -qq && apt-get install -y -qq clang-tidy libsodium-dev > /dev/null 2>&1
    
    echo "🔍 Running clang-tidy on core/ files..."
    
    # Run clang-tidy on each file and collect warnings
    # Filter to only show warnings from our code, not system headers
    find . -name "*.c" -o -name "*.h" | grep -v build | while read file; do
        clang-tidy -warnings-as-errors="*" \
            -header-filter="^\./(include|src|tests)/.*" \
            "$file" -- -I./include -DGITMIND_CORE_BUILD 2>&1 || true
    done | grep -E "/workspace/core/(src|include|tests)/.*\.(c|h):[0-9]+:[0-9]+: (warning|error):" | \
           sed "s|/workspace/||g" > /workspace/tools/baseline.txt
    
    # Count warnings
    WARNING_COUNT=$(wc -l < /workspace/tools/baseline.txt)
    echo "✅ Baseline generated: $WARNING_COUNT warnings"
    
    # Show summary by category
    echo ""
    echo "📊 Warning summary:"
    grep -oE "\[(.*?)\]" /workspace/tools/baseline.txt | sort | uniq -c | sort -rn | head -10
'