FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC
RUN apt-get update && \
    apt-get install -y wget gnupg lsb-release python3-pip pkg-config software-properties-common && \
    wget -O - https://apt.llvm.org/llvm.sh | bash -s -- 20 && \
    apt-get install -y clang-20 clang-tidy-20 cppcheck libsodium-dev git ninja-build gcovr && \
    ln -sf /usr/bin/clang-tidy-20 /usr/local/bin/clang-tidy && \
    ln -sf /usr/bin/clang++-20    /usr/local/bin/clang++ && \
    ln -sf /usr/bin/clang-20      /usr/local/bin/clang && \
    pip3 install --break-system-packages meson ninja
WORKDIR /workspace