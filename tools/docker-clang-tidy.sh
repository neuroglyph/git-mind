#!/bin/bash
# Run clang-tidy in Docker to match CI environment exactly
set -euo pipefail

# Build the Docker image if needed (reuse the CI Dockerfile)
if [ ! -f .ci/Dockerfile ]; then
    echo "Creating CI Dockerfile..."
    mkdir -p .ci
    cat > .ci/Dockerfile << 'EOF'
FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC
RUN apt-get update && \
    apt-get install -y wget gnupg lsb-release python3-pip pkg-config software-properties-common && \
    wget -O - https://apt.llvm.org/llvm.sh | bash -s -- 20 && \
    apt-get install -y clang-20 clang-tidy-20 cppcheck libsodium-dev git ninja-build && \
    ln -sf /usr/bin/clang-tidy-20 /usr/local/bin/clang-tidy && \
    ln -sf /usr/bin/clang++-20    /usr/local/bin/clang++ && \
    ln -sf /usr/bin/clang-20      /usr/local/bin/clang && \
    pip3 install meson ninja
WORKDIR /workspace
EOF
fi

IMAGE=gitmind-ci:latest

# Build image if needed
if ! docker image inspect $IMAGE >/dev/null 2>&1; then
    echo "Building CI Docker image..."
    docker build -t $IMAGE .ci
fi

# Run clang-tidy exactly like CI does
echo "Running clang-tidy in Docker (matching CI environment)..."
docker run --rm -v "$PWD":/workspace -w /workspace $IMAGE bash -c '
    # Build to get compile_commands.json
    rm -rf build
    CC=clang meson setup build
    ninja -C build
    cp build/compile_commands.json .
    
    # Run clang-tidy exactly like CI
    clang-tidy -quiet -p . --config-file=quality/.clang-tidy \
        $(git ls-files "core/**/*.c" "core/**/*.h") \
        | tee clang-tidy-report-full.txt || true
    
    # Filter to just project warnings
    grep -E "^/.*core/|^core/" clang-tidy-report-full.txt > clang-tidy-report.txt || true
    
    # Count warnings
    echo ""
    echo "=== Results ==="
    python3 tools/count_warnings.py check clang-tidy-report.txt tools/baseline_count.txt || true
    echo "Total warnings: $(grep -c "warning:\|error:" clang-tidy-report.txt || echo 0)"
'