#!/bin/bash
# Run clang-tidy in Docker to match CI environment exactly
set -euo pipefail

# Image naming (consistent and easy to prune)
GITMIND_NS=${GITMIND_NS:-gitmind}
CI_TAG=${CI_TAG:-clang-20}
IMAGE=${GITMIND_CI_IMAGE:-$GITMIND_NS/ci:$CI_TAG}

# Build the Docker image if needed (reuse the repo's CI Dockerfile)
mkdir -p .ci
echo "(Re)writing CI Dockerfile to ensure dependencies (incl. CRoaring) are present..."
cat > .ci/Dockerfile << 'EOF'
FROM ubuntu:22.04
LABEL com.gitmind.project="git-mind" \
      org.opencontainers.image.title="git-mind-ci"
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC
RUN apt-get update && \
    apt-get install -y --no-install-recommends wget gnupg lsb-release python3-pip pkg-config software-properties-common ca-certificates build-essential cmake git && \
    rm -rf /var/lib/apt/lists/* /var/cache/apt/archives/* && \
    wget -O - https://apt.llvm.org/llvm.sh | bash -s -- 20 && \
    apt-get clean && rm -rf /var/lib/apt/lists/* /var/cache/apt/archives/* && \
    apt-get update && \
    apt-get install -y --no-install-recommends clang-20 clang-tidy-20 cppcheck libsodium-dev libgit2-dev libroaring-dev git ninja-build pkg-config && \
    apt-get clean && rm -rf /var/lib/apt/lists/* /var/cache/apt/archives/* && \
    bash -lc 'if ! pkg-config --exists roaring; then echo "Building CRoaring from source (fallback)..."; git clone --depth 1 https://github.com/RoaringBitmap/CRoaring.git /tmp/CRoaring && cd /tmp/CRoaring && cmake -S . -B build -DENABLE_ROARING_TESTS=OFF -DENABLE_ROARING_BENCHMARKS=OFF >/dev/null && cmake --build build -j >/dev/null && cmake --install build >/dev/null && ldconfig; fi' && \
    ln -sf /usr/bin/clang-tidy-20 /usr/local/bin/clang-tidy && \
    ln -sf /usr/bin/clang++-20    /usr/local/bin/clang++ && \
    ln -sf /usr/bin/clang-20      /usr/local/bin/clang && \
    pip3 install --no-cache-dir meson ninja
WORKDIR /workspace
EOF

# Build image if needed
echo "Building CI Docker image ($IMAGE)..."
docker build -t "$IMAGE" --label com.gitmind.project=git-mind .ci >/dev/null

# Run clang-tidy exactly like CI does
echo "Running clang-tidy in Docker (matching CI environment)..."
docker run --rm --label com.gitmind.project=git-mind -v "$PWD":/workspace -w /workspace -e GITMIND_DOCKER=1 "$IMAGE" bash -c '
    # Build to get compile_commands.json
    rm -rf build
    CC=clang meson setup build
    ninja -C build
    cp build/compile_commands.json .
    
    # Run clang-tidy exactly like CI (excluding test files)
    clang-tidy -quiet -p . --config-file=quality/.clang-tidy \
        $(git ls-files "core/**/*.c" "core/**/*.h" | grep -v -E "(test_|_test\.|/tests/|core/src/hooks/)" ) \
        | tee clang-tidy-report-full.txt || true
    
    # Filter to just project warnings (handles both absolute /workspace/core/ and relative ../core/ paths)
    grep -E "^/.*core/|^core/|^\.\./core/" clang-tidy-report-full.txt > clang-tidy-report.txt || true
    
    # Count warnings
    echo ""
    echo "=== Results ==="
    python3 tools/count_warnings.py check clang-tidy-report.txt tools/baseline_count.txt || true
    echo "Total warnings: $(grep -c "warning:\|error:" clang-tidy-report.txt || echo 0)"
'
