# Developer Setup Guide

## Required Tools

### LLVM / clang-tidy

We standardize on __LLVM 20.x__.

#### macOS (Homebrew)

```bash
brew install llvm
echo 'export PATH="/opt/homebrew/opt/llvm/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc
```

#### Ubuntu / Debian

```bash
wget -O - https://apt.llvm.org/llvm.sh | sudo bash -s -- 20
sudo apt-get install -y clang-tidy-20
sudo ln -sf /usr/bin/clang-tidy-20 /usr/local/bin/clang-tidy
```

#### Verify installation

```bash
clang-tidy --version   # LLVM version 20.1.x
```

## Pre-commit Hooks

Install pre-commit hooks to ensure code quality:

```bash
pre-commit install
```

## Docker Development (CI parity)

For a consistent development environment, build and test inside the CI container. Local LLVM installs are optional (editor tooling); CI and Docker scripts use LLVM 20 inside the container.

Install local Git hooks for the quick pre‑push gate and docs lint:

```bash
make install-hooks
```

To build and test in Docker:

```bash
make test-core  # Run core tests
make check      # Run quality checks
```
