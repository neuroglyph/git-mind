# SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0
# © 2025 J. Kirby Ross / Neuroglyph Collective

# 🐳 DOCKER-ENFORCED MAKEFILE 🐳
# This Makefile ALWAYS runs commands in Docker
# Direct compilation on host is IMPOSSIBLE!

.PHONY: all
all: help

.PHONY: help
help:
	@echo "🐳 ALL COMMANDS RUN IN DOCKER AUTOMATICALLY 🐳"
	@echo ""
	@echo "Usage:"
	@echo "  make dev     - Start development environment"
	@echo "  make test    - Run all tests in Docker"
	@echo "  make build   - Build release binary in Docker"
	@echo "  make clean   - Clean build artifacts"
	@echo "  make shell   - Get a shell in the dev container"
	@echo ""
	@echo "Examples:"
	@echo "  make test              # Run full test suite"
	@echo "  make dev               # Start coding environment"
	@echo "  make build             # Create release binary"
	@echo ""
	@echo "✅ Direct compilation is IMPOSSIBLE - everything runs in Docker!"

# Special case: dev gives you a shell
.PHONY: dev
dev:
	@echo "🐳 Starting development environment..."
	@COMPOSE_BAKE=true DOCKER_BUILDKIT=1 docker compose run --rm dev bash

# Special case: shell is an alias for dev
.PHONY: shell
shell: dev

# Special case: clean should work on host to remove artifacts
.PHONY: clean
clean:
	@echo "🧹 Cleaning build artifacts..."
	@rm -rf git-mind build/
	@docker compose down 2>/dev/null || true

# Build the new journal-based implementation
.PHONY: build
build:
	@echo "🔨 Building git-mind..."
	@COMPOSE_BAKE=true DOCKER_BUILDKIT=1 docker compose run --rm -T dev make -C src
	@echo "✅ Built git-mind successfully!"
	@echo "Binary is available inside Docker at /workspace/build/bin/git-mind"

# Run tests SAFELY in Docker
.PHONY: test
test:
	@echo "🧪 Running tests in Docker (SAFE)..."
	@COMPOSE_BAKE=true DOCKER_BUILDKIT=1 docker compose run --rm -T dev bash -c "make -C /workspace/src && cd /tmp && cp -r /workspace/tests . && cp /workspace/build/bin/git-mind . && export GIT_MIND=/tmp/git-mind && cd tests && bash integration/test_behavior.sh"
	@echo "✅ Tests completed!"

# Run tests EXACTLY like GitHub Actions CI
.PHONY: test-ci
test-ci:
	@echo "🧪 Running CI simulation (EXACTLY like GitHub Actions)..."
	@echo "Building Docker image..."
	@COMPOSE_BAKE=true DOCKER_BUILDKIT=1 docker compose build
	@echo "Running tests in container (no nested Docker)..."
	@COMPOSE_BAKE=true DOCKER_BUILDKIT=1 docker compose run --rm -T dev bash -c "make -C /workspace/src && cd /tmp && cp -r /workspace/tests . && cp /workspace/build/bin/git-mind . && export GIT_MIND=/tmp/git-mind && cd tests && bash integration/test_behavior.sh"
	@echo "✅ CI simulation completed!"

# Run E2E tests
.PHONY: test-e2e
test-e2e:
	@echo "🧪 Running E2E tests in Docker..."
	@COMPOSE_BAKE=true DOCKER_BUILDKIT=1 docker compose run --rm -T dev bash -c "make -C /workspace/src && cd /tmp && cp -r /workspace/tests . && cp /workspace/build/bin/git-mind . && cd tests/e2e && ./run_all_tests.sh"

# Code quality checks
.PHONY: lint
lint:
	@echo "🔍 Running code quality checks..."
	@COMPOSE_BAKE=true DOCKER_BUILDKIT=1 docker compose run --rm -T dev bash -c "cd /workspace && ./tools/code-quality/lint.sh"

.PHONY: check-quality
check-quality:
	@echo "🔍 Running all quality checks (v2 - using standard tools)..."
	@COMPOSE_BAKE=true DOCKER_BUILDKIT=1 docker compose run --rm -T dev bash -c "cd /workspace && chmod +x ./tools/code-quality/*.sh && ./tools/code-quality/run-all-checks-v2.sh"

.PHONY: check-quality-legacy
check-quality-legacy:
	@echo "🔍 Running legacy custom checks..."
	@COMPOSE_BAKE=true DOCKER_BUILDKIT=1 docker compose run --rm -T dev bash -c "cd /workspace && ./tools/code-quality/run-all-checks.sh"

.PHONY: format
format:
	@echo "🎨 Formatting code with clang-format..."
	@COMPOSE_BAKE=true DOCKER_BUILDKIT=1 docker compose run --rm -T dev bash -c "find /workspace/src -name '*.c' -o -name '*.h' | xargs clang-format -i"
	@echo "✅ Code formatted!"

.PHONY: fix-tech-debt
fix-tech-debt:
	@echo "📋 Technical Debt Task List:"
	@echo ""
	@cat TECH_DEBT_TASKLIST.md | head -50
	@echo ""
	@echo "... (see TECH_DEBT_TASKLIST.md for full list)"

# Install git hooks
.PHONY: install-hooks
install-hooks:
	@echo "🪝 Installing git hooks..."
	@git config core.hooksPath .githooks
	@chmod +x .githooks/*
	@echo "✅ Git hooks installed!"

# Everything else gets forwarded to Docker
%:
	@COMPOSE_BAKE=true DOCKER_BUILDKIT=1 docker compose run --rm dev make -f Makefile.docker $@