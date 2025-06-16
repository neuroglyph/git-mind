# SPDX-License-Identifier: Apache-2.0
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
	@docker compose run --rm dev bash

# Special case: shell is an alias for dev
.PHONY: shell
shell: dev

# Special case: clean should work on host to remove artifacts
.PHONY: clean
clean:
	@echo "🧹 Cleaning build artifacts..."
	@rm -f git-mind
	@find src -name "*.o" -delete 2>/dev/null || true
	@docker compose down 2>/dev/null || true

# Everything else gets forwarded to Docker
%:
	@docker compose run --rm dev make -f Makefile.docker $@