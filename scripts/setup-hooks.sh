#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# © 2025 J. Kirby Ross / Neuroglyph Collective

# Setup git hooks for the project

echo "🪝 Setting up git hooks..."

# Configure git to use our hooks directory
git config core.hooksPath .githooks

echo "✅ Git hooks configured!"
echo ""
echo "The pre-commit hook will now prevent:"
echo "  - Committing .o files"
echo "  - Committing build artifacts"
echo "  - Committing the binary"
echo ""
echo "This enforces our Docker-only build rule! 🐳"