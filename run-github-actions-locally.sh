#!/bin/bash
# SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0
# © 2025 J. Kirby Ross / Neuroglyph Collective

set -euo pipefail

# Check if act is installed
if ! command -v act &> /dev/null; then
    echo "❌ 'act' is not installed. Install it with:"
    echo "  brew install act (macOS)"
    echo "  or download from: https://github.com/nektos/act"
    exit 1
fi

# Run the core-quality workflow exactly as GitHub would
echo "🚀 Running GitHub Actions workflow locally in Docker..."
echo "This runs the EXACT same steps as GitHub Actions"
echo ""

# Run with the same Ubuntu image GitHub uses
act push \
    --workflows .github/workflows/core-quality.yml \
    --platform ubuntu-latest=catthehacker/ubuntu:act-latest \
    --container-architecture linux/amd64 \
    -v

echo ""
echo "✅ GitHub Actions workflow completed"