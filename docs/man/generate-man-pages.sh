#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
# Generate man pages from Markdown documentation

set -e

# Check for pandoc
if ! command -v pandoc &> /dev/null; then
    echo "pandoc is required to generate man pages"
    echo "Install with: apt-get install pandoc (or brew install pandoc)"
    exit 1
fi

# Create man page directory
MANDIR="../../man"
mkdir -p "$MANDIR/man1"

# Convert each markdown file to man page
for md in *.1.md; do
    if [ -f "$md" ]; then
        base=$(basename "$md" .md)
        echo "Generating $base..."
        pandoc -s -t man "$md" -o "$MANDIR/man1/$base"
    fi
done

echo "✅ Man pages generated in $MANDIR/man1/"
echo ""
echo "To view a man page:"
echo "  man -l $MANDIR/man1/gitmind.1"
echo ""
echo "To install system-wide:"
echo "  sudo cp $MANDIR/man1/* /usr/local/share/man/man1/"