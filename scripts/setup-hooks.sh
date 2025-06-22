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