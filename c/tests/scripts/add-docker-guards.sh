#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# Add Docker guards to all test scripts

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

echo "Adding Docker guards to all test scripts..."

# Find all .sh files in integration directory
for script in "$SCRIPT_DIR/integration"/*.sh; do
    if [ -f "$script" ]; then
        script_name=$(basename "$script")
        
        # Skip if already has docker-guard
        if grep -q "docker-guard.sh" "$script"; then
            echo "✓ $script_name already has Docker guard"
            continue
        fi
        
        # Skip docker-guard.sh itself
        if [ "$script_name" = "docker-guard.sh" ]; then
            continue
        fi
        
        echo "Adding Docker guard to $script_name..."
        
        # Create temp file with docker guard added after shebang and comments
        awk '
            BEGIN { printed = 0 }
            /^#!/ { print; next }
            /^#/ && !printed { print; next }
            !printed {
                print "# Source Docker guard - will exit if not in Docker"
                print "SCRIPT_DIR=\"$(cd \"$(dirname \"${BASH_SOURCE[0]}\")\" && pwd)\""
                print "source \"$SCRIPT_DIR/../docker-guard.sh\""
                print ""
                printed = 1
            }
            { print }
        ' "$script" > "$script.tmp"
        
        # Replace original
        mv "$script.tmp" "$script"
        chmod +x "$script"
    fi
done

echo ""
echo "✅ Done! All test scripts now have Docker guards."
echo ""
echo "Test scripts will now refuse to run outside Docker with a helpful error."