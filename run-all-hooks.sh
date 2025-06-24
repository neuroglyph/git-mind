#!/bin/bash
# SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0
# © 2025 J. Kirby Ross / Neuroglyph Collective

set -euo pipefail

echo "🚀 Running ALL Git hooks and GitHub Actions locally in Docker"
echo "=================================================="
echo ""

# Check if Docker is running
if ! docker info >/dev/null 2>&1; then
    echo "❌ Docker is not running. Please start Docker and try again."
    exit 1
fi

echo "1️⃣ Running pre-commit hook..."
echo "------------------------"
if ! ./.git/hooks/pre-commit; then
    echo "❌ Pre-commit hook failed!"
    exit 1
fi
echo ""

echo "2️⃣ Running pre-push hook (FULL CI simulation)..."
echo "------------------------------------"
if ! ./.git/hooks/pre-push; then
    echo "❌ Pre-push hook failed!"
    exit 1
fi
echo ""

echo "3️⃣ Running GitHub Actions workflow (core-quality.yml)..."
echo "-------------------------------------------"
# This runs the EXACT GitHub Actions workflow in Docker
if command -v act &> /dev/null; then
    act push \
        --workflows .github/workflows/core-quality.yml \
        --platform ubuntu-latest=catthehacker/ubuntu:act-latest \
        --container-architecture linux/amd64 \
        -v
else
    echo "⚠️  'act' not installed, running make ci-full instead"
    echo "   To run the EXACT GitHub workflow, install act:"
    echo "   brew install act"
    echo ""
    make ci-full
fi

echo ""
echo "✅ ALL HOOKS AND CI CHECKS PASSED!"
echo "Your code is ready to push to GitHub."