# SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0
# © 2025 J. Kirby Ross / Neuroglyph Collective

# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=gnu11 -O2
CFLAGS += -I./include

# Directories
SRC_DIR = src
TEST_DIR = tests
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin

# Source files
ERROR_SRCS = $(SRC_DIR)/error/error.c
ALL_SRCS = $(ERROR_SRCS)

# Object files
ERROR_OBJS = $(ERROR_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
ALL_OBJS = $(ERROR_OBJS)

# Test files
TEST_ERROR = $(TEST_DIR)/unit/test_error.c

# Targets
.PHONY: all clean test

all: dirs test

dirs:
	@mkdir -p $(OBJ_DIR)/error
	@mkdir -p $(BIN_DIR)

# Build object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Build and run tests
test: dirs $(ALL_OBJS)
	$(CC) $(CFLAGS) $(TEST_ERROR) $(ERROR_OBJS) -o $(BIN_DIR)/test_error
	@echo "Running tests..."
	@$(BIN_DIR)/test_error

clean:
	rm -rf $(BUILD_DIR)

.PHONY: check
check:
	@echo "🔍 Running quality checks on core/..."
	@echo "✓ clang-format check"
	@find . -name "*.c" -o -name "*.h" | xargs clang-format --dry-run --Werror || (echo "❌ Format check failed" && exit 1)
	@echo "✓ All checks passed!"