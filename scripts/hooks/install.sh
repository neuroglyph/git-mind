#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# Installs git-mind hooks into a repository

set -e

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Check if we're in a git repository
if ! git rev-parse --git-dir > /dev/null 2>&1; then
    echo -e "${RED}Error: Not in a git repository${NC}"
    exit 1
fi

GIT_DIR=$(git rev-parse --git-dir)
HOOKS_DIR="$GIT_DIR/hooks"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "Installing git-mind hooks..."

# List of hooks to install
HOOKS=(
    "post-commit"
    "post-merge"
    "post-checkout"
)

# Create hooks directory if it doesn't exist
mkdir -p "$HOOKS_DIR"

# Install each hook
for hook in "${HOOKS[@]}"; do
    SOURCE="$SCRIPT_DIR/$hook"
    DEST="$HOOKS_DIR/$hook"
    
    if [ -f "$SOURCE" ]; then
        # Check if hook already exists
        if [ -f "$DEST" ]; then
            if [ -L "$DEST" ] && [ "$(readlink "$DEST")" = "$SOURCE" ]; then
                echo -e "${GREEN}✓${NC} $hook already installed"
                continue
            else
                echo -e "${YELLOW}⚠${NC}  Existing $hook hook found"
                
                # Back up existing hook
                BACKUP="$DEST.pre-gitmind"
                cp "$DEST" "$BACKUP"
                echo "    Backed up to $hook.pre-gitmind"
                
                # Try to merge with existing hook
                if grep -q "git-mind" "$DEST"; then
                    echo "    Existing hook already has git-mind integration"
                else
                    # Append our hook to existing
                    echo "" >> "$DEST"
                    echo "# git-mind integration" >> "$DEST"
                    cat "$SOURCE" | grep -v "^#!/bin/bash" >> "$DEST"
                    chmod +x "$DEST"
                    echo -e "${GREEN}✓${NC} $hook updated with git-mind integration"
                fi
                continue
            fi
        fi
        
        # Create symlink to our hook
        ln -sf "$SOURCE" "$DEST"
        chmod +x "$SOURCE"
        echo -e "${GREEN}✓${NC} $hook installed"
    else
        echo -e "${RED}✗${NC} $hook not found in $SCRIPT_DIR"
    fi
done

echo ""
echo -e "${GREEN}git-mind hooks installation complete!${NC}"
echo ""
echo "Hooks will:"
echo "  • Update links when files are renamed (post-commit)"
echo "  • Check link integrity after merges (post-merge)"
echo "  • Verify links when switching branches (post-checkout)"
echo ""
echo "To enable automatic link extraction from markdown:"
echo "  export GITMIND_AUTO_EXTRACT=1"
echo ""
echo "To uninstall, run: $SCRIPT_DIR/uninstall.sh"