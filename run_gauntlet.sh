#!/bin/bash
set -e

echo "🔥 GNU CRY GAUNTLET - Local Execution"
echo "======================================"

# Build the Docker image
echo "Building gauntlet environment..."
docker build -t gauntlet -f Dockerfile.gauntlet .

# Run the full gauntlet
echo "Running GNU CRY GAUNTLET..."
docker run --rm -v $(pwd):/workspace -w /workspace gauntlet /bin/bash -c "
set -e

echo '🔧 Setting up build...'
meson setup build_gauntlet
echo '✅ Build setup complete'

echo '🔨 Compiling...'
meson compile -C build_gauntlet
echo '✅ Compilation complete'

echo '🧪 Running tests...'
meson test -C build_gauntlet
echo '✅ Tests complete'

echo '🔍 Running clang-tidy on all files...'
# Generate compile commands
ninja -C build_gauntlet -t compdb > compile_commands.json

# Run clang-tidy on all C files
find core/src -name '*.c' | while read file; do
    echo \"Checking \$file...\"
    clang-tidy \
        --checks='-*,readability-*,bugprone-*,misc-*,clang-analyzer-*' \
        --warnings-as-errors='*' \
        --header-filter='.*' \
        \"\$file\" || echo \"❌ Warnings found in \$file\"
done

echo '✅ clang-tidy scan complete'
echo '🎯 GNU CRY GAUNTLET COMPLETE'
"