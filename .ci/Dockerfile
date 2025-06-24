FROM ubuntu:22.04
RUN apt-get update && \
    apt-get install -y \
        clang clang-tidy ninja-build libsodium-dev git \
        python3 python3-pip pkg-config && \
    pip3 install meson
WORKDIR /workspace