#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# Uninstalls git-mind hooks from a repository

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

echo "Uninstalling git-mind hooks..."

# List of hooks to uninstall
HOOKS=(
    "post-commit"
    "post-merge"
    "post-checkout"
)

# Uninstall each hook
for hook in "${HOOKS[@]}"; do
    HOOK_FILE="$HOOKS_DIR/$hook"
    BACKUP_FILE="$HOOK_FILE.pre-gitmind"
    
    if [ -f "$HOOK_FILE" ]; then
        # Check if it's a symlink to our hook
        if [ -L "$HOOK_FILE" ]; then
            rm "$HOOK_FILE"
            echo -e "${GREEN}✓${NC} Removed $hook symlink"
            
            # Restore backup if it exists
            if [ -f "$BACKUP_FILE" ]; then
                mv "$BACKUP_FILE" "$HOOK_FILE"
                echo -e "${GREEN}✓${NC} Restored original $hook"
            fi
        else
            # Check if hook contains git-mind integration
            if grep -q "git-mind" "$HOOK_FILE"; then
                # Try to remove just our section
                if [ -f "$BACKUP_FILE" ]; then
                    # Restore from backup
                    mv "$BACKUP_FILE" "$HOOK_FILE"
                    echo -e "${GREEN}✓${NC} Restored original $hook"
                else
                    echo -e "${YELLOW}⚠${NC}  $hook contains git-mind code but no backup found"
                    echo "    Manual cleanup may be required"
                fi
            else
                echo "  $hook has no git-mind integration"
            fi
        fi
    else
        echo "  No $hook hook found"
    fi
done

echo ""
echo -e "${GREEN}git-mind hooks uninstalled${NC}"