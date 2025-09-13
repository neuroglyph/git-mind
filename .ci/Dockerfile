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
