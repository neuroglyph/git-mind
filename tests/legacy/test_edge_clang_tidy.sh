#!/bin/bash
# SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0
# © 2025 J. Kirby Ross / Neuroglyph Collective

# This test ensures edge.c ALWAYS passes clang-tidy with ZERO warnings
# Run this in CI to prevent quality regression

set -e

echo "🔍 Running clang-tidy on edge.c..."

# Run clang-tidy and capture output
WARNINGS=$(clang-tidy ../src/edge/edge.c \
    --config-file=../../quality/.clang-tidy \
    -- -I../../include 2>&1 | grep -E "warning:|error:" | wc -l)

if [ "$WARNINGS" -ne "0" ]; then
    echo "❌ FAIL: edge.c has $WARNINGS warnings!"
    echo ""
    echo "Details:"
    clang-tidy ../src/edge/edge.c \
        --config-file=../../quality/.clang-tidy \
        -- -I../../include 2>&1 | grep -E "warning:|error:"
    exit 1
fi

echo "✅ PASS: edge.c has ZERO warnings!"
echo ""

# Also check against the LINUS config for bonus points
echo "🔥 Checking against .clang-tidy.linus (for the brave)..."
LINUS_WARNINGS=$(clang-tidy ../src/edge/edge.c \
    --config-file=../../quality/.clang-tidy.linus \
    -- -I../../include 2>&1 | grep -E "warning:|error:" | wc -l || true)

if [ "$LINUS_WARNINGS" -eq "0" ]; then
    echo "🏆 LEGENDARY: edge.c passes LINUS-LEVEL checks!"
else
    echo "📊 edge.c has $LINUS_WARNINGS warnings with Linus config"
    echo "   (This is optional - not a failure)"
fi

echo ""
echo "🛡️ Quality gate PASSED - edge.c is protected!"