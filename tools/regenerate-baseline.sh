#!/usr/bin/env bash
set -euo pipefail

echo "🔄  Regenerating baseline using CI Docker image…"

GITMIND_NS=${GITMIND_NS:-gitmind}
CI_TAG=${CI_TAG:-clang-20}
IMAGE=${GITMIND_CI_IMAGE:-$GITMIND_NS/ci:$CI_TAG}

# Build the image
docker build -q -t "$IMAGE" --label com.gitmind.project=git-mind .ci 2>/dev/null || \
docker build -t "$IMAGE" --label com.gitmind.project=git-mind .ci

# Run clang-tidy and generate baseline
docker run --rm -t --label com.gitmind.project=git-mind \
  -v "$PWD":/workspace \
  -w /workspace \
  "$IMAGE" \
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
    python3 tools/count_warnings.py create-baseline clang-tidy-report.txt tools/baseline_count.txt
    
    echo "✅  Baseline regenerated"
'
