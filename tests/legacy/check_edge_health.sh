#!/bin/bash
# SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0
# © 2025 J. Kirby Ross / Neuroglyph Collective

# Health check, not prison guard!

echo "🏥 Edge Module Health Check"
echo "=========================="
echo ""

# Track overall health
HEALTH_SCORE=100

# Check for critical issues only
echo "🔍 Checking for critical issues..."
CRITICAL=$(clang-tidy ../src/edge/edge.c \
    --checks='-*,bugprone-*,cert-*,-cert-err34-c' \
    -- -I../../include 2>&1 | grep -E "error:" | wc -l)

if [ "$CRITICAL" -gt "0" ]; then
    echo "❌ Found $CRITICAL critical issues that need fixing"
    HEALTH_SCORE=$((HEALTH_SCORE - 50))
else
    echo "✅ No critical issues!"
fi

# Check for warnings (just inform, don't fail)
echo ""
echo "📊 Code quality metrics:"
WARNINGS=$(clang-tidy ../src/edge/edge.c \
    --config-file=../../quality/.clang-tidy \
    -- -I../../include 2>&1 | grep -E "warning:" | wc -l || true)

if [ "$WARNINGS" -eq "0" ]; then
    echo "   🌟 PERFECT: Zero warnings!"
elif [ "$WARNINGS" -lt "5" ]; then
    echo "   ✅ Very Good: $WARNINGS minor warnings"
    HEALTH_SCORE=$((HEALTH_SCORE - 5))
elif [ "$WARNINGS" -lt "10" ]; then
    echo "   ⚠️  Okay: $WARNINGS warnings (consider cleanup)"
    HEALTH_SCORE=$((HEALTH_SCORE - 10))
else
    echo "   🔥 Needs Work: $WARNINGS warnings"
    HEALTH_SCORE=$((HEALTH_SCORE - 20))
fi

# Check complexity (informational)
echo ""
echo "🧠 Complexity check:"
FUNCTIONS=$(grep -c "^[a-zA-Z_].*(" ../src/edge/edge.c || echo "0")
LOC=$(wc -l < ../src/edge/edge.c)
echo "   - Lines of code: $LOC"
echo "   - Number of functions: $FUNCTIONS"
echo "   - Average function size: $((LOC / FUNCTIONS)) lines"

if [ $((LOC / FUNCTIONS)) -gt 30 ]; then
    echo "   💡 Tip: Consider breaking up larger functions"
fi

# Innovation-friendly metrics
echo ""
echo "🚀 Innovation readiness:"
echo -n "   - Has TODO/FIXME comments: "
if grep -q "TODO\|FIXME" ../src/edge/edge.c; then
    echo "Yes (that's okay!)"
else
    echo "No"
fi

echo -n "   - Uses dependency injection: "
if grep -q "ctx->" ../src/edge/edge.c; then
    echo "✅ Yes!"
else
    echo "❌ No (limits testability)"
fi

echo -n "   - Has extension points: "
if grep -q "switch.*rel_type" ../src/edge/edge.c; then
    echo "✅ Yes (can add edge types)"
else
    echo "🤔 Maybe add some?"
fi

# Final score
echo ""
echo "═══════════════════════════════"
echo "📈 Overall Health Score: $HEALTH_SCORE/100"

if [ "$HEALTH_SCORE" -ge 90 ]; then
    echo "🏆 Excellent! Keep up the great work!"
elif [ "$HEALTH_SCORE" -ge 70 ]; then
    echo "👍 Good shape! Minor improvements possible."
elif [ "$HEALTH_SCORE" -ge 50 ]; then
    echo "⚠️  Needs attention, but not urgent."
else
    echo "🚨 Critical issues need fixing!"
    exit 1  # Only fail on critical issues
fi

echo ""
echo "Remember: Perfect is the enemy of good!"
echo "Innovation > Perfection 🚀"