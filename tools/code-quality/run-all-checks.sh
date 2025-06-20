#!/bin/bash
# SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0
# © 2025 J. Kirby Ross / Neuroglyph Collective

# Run all code quality checks

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
FAILED=0

echo "=== Running Git-Mind Code Quality Checks ==="
echo

# Make all scripts executable
chmod +x "$SCRIPT_DIR"/*.sh

# Run each check
checks=(
    "check-function-length.sh"
    "check-magic-values.sh"
    "check-output-control.sh"
    "check-dependency-injection.sh"
    "check-test-quality.sh"
)

for check in "${checks[@]}"; do
    echo ">>> Running $check"
    if "$SCRIPT_DIR/$check"; then
        echo
    else
        FAILED=$((FAILED + 1))
        echo
    fi
done

# Summary
echo "=== Summary ==="
if [ $FAILED -eq 0 ]; then
    echo "✅ All code quality checks passed!"
    exit 0
else
    echo "❌ $FAILED checks failed"
    echo
    echo "Run 'make fix-tech-debt' to see the task list"
    exit 1
fi