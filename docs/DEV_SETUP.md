---
title: Developer Setup Guide
description: Required tools and environment configuration for contributors.
audience: [contributors, developers]
domain: [operations]
tags: [dev-setup, clang-tidy]
status: stable
last_updated: 2025-09-15
---

# Developer Setup Guide

## Required Tools

### LLVM / clang-tidy

We standardize on **LLVM 20.x**.

#### macOS (Homebrew):
```bash
brew install llvm
echo 'export PATH="/opt/homebrew/opt/llvm/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

#### Ubuntu / Debian:
```bash
wget -O - https://apt.llvm.org/llvm.sh | sudo bash -s -- 20
sudo apt-get install -y clang-tidy-20
sudo ln -sf /usr/bin/clang-tidy-20 /usr/local/bin/clang-tidy
```

#### Verify installation:
```bash
clang-tidy --version   # LLVM version 20.1.x
```

## Pre-commit Hooks

Install pre-commit hooks to ensure code quality:
```bash
pre-commit install
```

## Docker Development

For consistent development environment:
```bash
make dev  # Opens a shell in the Docker container
```

## Building

Always build in Docker to match CI:
```bash
make test-core  # Run core tests
make check      # Run quality checks
```

## Build Feature Toggles (Meson)

The library exposes a small set of build‑time feature toggles (Meson options) to control which optional public headers are pulled in by the umbrella `include/gitmind.h`.

- `-Denable_io=true|false` (default: `false`)
  - Defines `GITMIND_ENABLE_IO` and includes `gitmind/io/io.h` from the umbrella header.
- `-Denable_time=true|false` (default: `false`)
  - Defines `GITMIND_ENABLE_TIME` and includes `gitmind/time/time.h`.
- `-Denable_util=true|false` (default: `false`)
  - Defines `GITMIND_ENABLE_UTIL` and includes `gitmind/util/memory.h`.
- `-Denable_utf8=true|false` (default: `true`)
  - Defines `GITMIND_ENABLE_UTF8` and includes `gitmind/utf8/validate.h`.

Example:

```bash
meson setup build -Denable_io=true -Denable_time=true
ninja -C build
```

These options only affect umbrella header aggregation and do not gate the core library build. They exist to keep downstreams’ public include surface minimal and configurable.
