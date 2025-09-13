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

## Docker Development (CI parity)

Always work via the CI container for builds/tests:
```bash
# Open CI dev shell (Meson/Ninja, LLVM 20, libsodium, libgit2, CRoaring)
make dev

# Build and test inside Docker
make build-docker
make test-docker

# Clang‑tidy parity
./tools/docker-clang-tidy.sh
# Or only changed core files vs main
./tools/tidy-diff-docker.sh origin/main

# Optional: strict multi‑compiler gauntlet
./tools/gauntlet/run-gauntlet.sh
```

Tip: install local Git hooks for a quick pre‑push gate that runs the fast Docker checks:
```bash
make install-hooks
```

For details, see: docs/quality/local-ci-parity.md
