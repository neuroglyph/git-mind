#!/usr/bin/env bash
set -euo pipefail

echo "🔄  Regenerating baseline using CI Docker image…"

IMAGE=gitmind-ci:latest

# Build the image
docker build -q -t $IMAGE .ci 2>/dev/null || docker build -t $IMAGE .ci

# Run clang-tidy and generate baseline
docker run --rm -t \
  -v "$PWD":/workspace \
  -w /workspace \
  $IMAGE \
  bash -c '
    set -e
    rm -rf build-container
    meson setup build-container
    ninja -C build-container
    
    # Run clang-tidy
    clang-tidy -quiet -p . --config-file=quality/.clang-tidy \
      $(git ls-files "core/**/*.c" "core/**/*.h") \
      | tee clang-tidy-report-full.txt || true
    
    # Filter to just project warnings
    grep -E "^/.*core/|^core/" clang-tidy-report-full.txt > clang-tidy-report.txt || true
    
    # Create baseline
    python3 tools/parse_warnings.py create-baseline clang-tidy-report.txt tools/baseline_grouped.txt
    
    echo "✅  Baseline regenerated with $(wc -l < clang-tidy-report.txt) warnings"
'