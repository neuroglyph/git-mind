#!/bin/bash
# SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0
# © 2025 J. Kirby Ross / Neuroglyph Collective

# Setup git hooks for the git-mind project

REPO_ROOT=$(git rev-parse --show-toplevel 2>/dev/null)
if [ -z "$REPO_ROOT" ]; then
    echo "❌ Error: Not in a git repository"
    exit 1
fi

HOOKS_DIR="$REPO_ROOT/.git/hooks"
PRE_COMMIT_HOOK="$HOOKS_DIR/pre-commit"

echo "🔧 Setting up git hooks for git-mind..."

# Create pre-commit hook
cat > "$PRE_COMMIT_HOOK" << 'EOF'
#!/bin/sh
# SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0
# © 2025 J. Kirby Ross / Neuroglyph Collective

# Pre-commit hook to ban TODO/FIXME/XXX comments in core/

echo "🔍 Checking for forbidden TODO/FIXME/XXX comments in core/..."

# Check for TODO/FIXME/XXX in staged files within core/
staged_files=$(git diff --cached --name-only --diff-filter=ACM | grep '^core/.*\.[ch]$')

if [ -n "$staged_files" ]; then
    for file in $staged_files; do
        if git diff --cached "$file" | grep -E '^\+.*\b(TODO|FIXME|XXX)\b'; then
            echo "❌ NO TODOs ALLOWED in core/"
            echo "Found forbidden comment in: $file"
            echo ""
            echo "The Enforcer demands ZERO placeholders. Either:"
            echo "1. Implement it properly NOW"
            echo "2. Remove the comment entirely"
            echo ""
            echo "Remember: 11,951 warnings came from 'temporary' TODOs"
            exit 1
        fi
    done
fi

echo "✅ No TODOs found in staged files"

# Check if we have any C/C++ files in core/ being committed
if [ -n "$staged_files" ]; then
    echo ""
    echo "🔍 Running clang-tidy on staged core/ files in Docker..."
    
    # Get repository root
    REPO_ROOT=$(git rev-parse --show-toplevel)
    
    # Run clang-tidy in Docker for each file
    failed=0
    for file in $staged_files; do
        echo "Checking $file..."
        
        # Run clang-tidy for this file in Docker
        echo "🐳 Starting Docker container..."
        if ! docker run --rm -v "$REPO_ROOT":/workspace -w /workspace ubuntu:latest bash -c "
            echo '📦 Installing dependencies...' && 
            apt-get update -qq && 
            apt-get install -y -qq clang clang-tidy libsodium-dev && 
            echo '🔍 Running clang-tidy...' &&
            clang-tidy -warnings-as-errors='*' -header-filter='.*' '$file' -- -I./core/include -DGITMIND_CORE_BUILD 2>&1
        " | head -50; then
            echo "❌ clang-tidy found issues in: $file"
            failed=1
        fi
    done
    
    if [ $failed -eq 1 ]; then
        echo ""
        echo "❌ clang-tidy checks failed. Fix the issues before committing."
        echo ""
        echo "To run clang-tidy on a specific file:"
        echo "  docker run --rm -v \$(pwd):/workspace -w /workspace ubuntu:latest bash -c \"apt-get update -qq && apt-get install -y -qq clang clang-tidy libsodium-dev && clang-tidy -warnings-as-errors='*' -header-filter='.*' <FILE> -- -I./core/include -DGITMIND_CORE_BUILD\""
        exit 1
    fi
    
    echo "✅ clang-tidy checks passed"
fi

exit 0
EOF

# Make hook executable
chmod +x "$PRE_COMMIT_HOOK"

echo "✅ Pre-commit hook installed successfully!"
echo ""
echo "The hook will:"
echo "- Block any commits that add TODO/FIXME/XXX comments to core/"
echo "- Enforce the ZERO placeholders policy"
echo ""
echo "To bypass the hook in emergencies (NOT RECOMMENDED):"
echo "  git commit --no-verify"
echo ""
echo "Remember: Every TODO becomes someone's technical debt!"