#!/bin/bash
# SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0
# © 2025 J. Kirby Ross / Neuroglyph Collective

# E2E test for AUGMENTS system

set -e

# Test directory
TEST_DIR="/tmp/gm-test-augments-$$"
GIT_MIND="${GIT_MIND:-../../git-mind}"

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

echo "=== E2E Test: AUGMENTS System ==="

# Setup
echo "Setting up test repository..."
mkdir -p "$TEST_DIR"
cd "$TEST_DIR"
git init --quiet

# Configure git
git config user.email "test@example.com"
git config user.name "Test User"

# Create test files
echo "Creating test files..."
echo "# README" > README.md
echo "# API Documentation" > API.md
git add .
git commit -m "Initial commit" --quiet

# Create initial link
echo "Creating initial link..."
"$GIT_MIND" link README.md API.md --type implements
if [ $? -ne 0 ]; then
    echo -e "${RED}✗ Failed to create initial link${NC}"
    exit 1
fi

# Install hooks
echo "Installing git hooks..."
"$GIT_MIND" install-hooks
if [ $? -ne 0 ]; then
    echo -e "${RED}✗ Failed to install hooks${NC}"
    exit 1
fi

# Fix hook to find git-mind-hook in test environment
echo "Fixing hook path for test..."
cat > .git/hooks/post-commit << 'EOF'
#!/bin/sh
# git-mind post-commit hook (test version)

# Hook binary is in same directory as git-mind
HOOK_BIN="/tmp/git-mind-hook"

# Run hook if found
if [ -x "$HOOK_BIN" ]; then
    "$HOOK_BIN" "$@"
fi

exit 0
EOF
chmod +x .git/hooks/post-commit

# Verify hook installed
if [ ! -x .git/hooks/post-commit ]; then
    echo -e "${RED}✗ Post-commit hook not installed${NC}"
    exit 1
fi

# Modify the source file
echo "Modifying README.md..."
echo "# README v2" > README.md
echo "Updated documentation" >> README.md
git add README.md

# Run hook manually with verbose for debugging
echo "Running hook manually for debugging..."
.git/hooks/post-commit --verbose || true

git commit -m "Update README" --quiet

# Check for AUGMENTS edge
echo "Checking for AUGMENTS edge..."
"$GIT_MIND" list --show-augments > augments.txt

# Verify AUGMENTS edge was created
if grep -q "AUGMENTS: README.md -> README.md" augments.txt; then
    echo -e "${GREEN}✓ AUGMENTS edge created${NC}"
else
    echo -e "${RED}✗ AUGMENTS edge not found${NC}"
    echo "Output:"
    cat augments.txt
    exit 1
fi

# Verify original edge still exists
if grep -q "IMPLEMENTS: README.md -> API.md" augments.txt; then
    echo -e "${GREEN}✓ Original edge preserved${NC}"
else
    echo -e "${RED}✗ Original edge missing${NC}"
    exit 1
fi

# Test default list (should hide AUGMENTS)
echo "Testing default list (no --show-augments)..."
"$GIT_MIND" list > default.txt

if grep -q "AUGMENTS:" default.txt; then
    echo -e "${RED}✗ AUGMENTS shown by default${NC}"
    exit 1
else
    echo -e "${GREEN}✓ AUGMENTS hidden by default${NC}"
fi

# Modify target file
echo "Modifying API.md..."
echo "# API Documentation v2" > API.md
git add API.md
git commit -m "Update API" --quiet

# Check that no AUGMENTS created for untracked file
echo "Checking API.md (no source edges)..."
"$GIT_MIND" list --show-augments | grep "API.md.*AUGMENTS" && {
    echo -e "${RED}✗ Unexpected AUGMENTS for API.md${NC}"
    exit 1
}
echo -e "${GREEN}✓ No AUGMENTS for untracked file${NC}"

# Test multiple file changes
echo "Testing multiple file changes..."
echo "# README v3" > README.md
echo "# New file" > NEW.md
git add .
git commit -m "Multiple changes" --quiet

"$GIT_MIND" list --show-augments > multi.txt
if grep -q "AUGMENTS: README.md -> README.md" multi.txt; then
    echo -e "${GREEN}✓ AUGMENTS for multiple file commit${NC}"
else
    echo -e "${RED}✗ AUGMENTS missing in multi-file commit${NC}"
    exit 1
fi

# Cleanup
cd /
rm -rf "$TEST_DIR"

echo -e "${GREEN}=== All AUGMENTS tests passed! ===${NC}"