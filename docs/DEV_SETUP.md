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