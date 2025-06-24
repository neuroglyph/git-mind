# SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0
# © 2025 J. Kirby Ross / Neuroglyph Collective

# Compiler settings
CC ?= gcc
CFLAGS = -Wall -Wextra -Werror -std=gnu11 -O2
CFLAGS += -I./include
LDFLAGS = -lsodium

# Coverage settings
COVERAGE ?= 0
ifeq ($(COVERAGE),1)
	CFLAGS += --coverage -fprofile-arcs -ftest-coverage
	LDFLAGS += --coverage -lgcov
endif

# Sanitizer settings
SANITIZE ?= 0
ifeq ($(SANITIZE),1)
	CFLAGS += -fsanitize=address,undefined -fno-omit-frame-pointer -g
	LDFLAGS += -fsanitize=address,undefined
endif

# Directories
SRC_DIR = src
TEST_DIR = tests
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin

# Source files
ERROR_SRCS = $(SRC_DIR)/error/error.c
TYPES_SRCS = $(SRC_DIR)/types/id.c $(SRC_DIR)/types/string.c $(SRC_DIR)/types/string_core.c $(SRC_DIR)/types/string_utf8.c $(SRC_DIR)/types/path.c
CRYPTO_SRCS = $(SRC_DIR)/crypto/sha256.c $(SRC_DIR)/crypto/random.c $(SRC_DIR)/crypto/backend.c
UTF8_SRCS = $(SRC_DIR)/utf8/validate.c
ALL_SRCS = $(ERROR_SRCS) $(TYPES_SRCS) $(CRYPTO_SRCS) $(UTF8_SRCS)

# Object files
ERROR_OBJS = $(ERROR_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
TYPES_OBJS = $(TYPES_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
CRYPTO_OBJS = $(CRYPTO_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
UTF8_OBJS = $(UTF8_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
ALL_OBJS = $(ERROR_OBJS) $(TYPES_OBJS) $(CRYPTO_OBJS) $(UTF8_OBJS)

# Test files
TEST_ERROR = $(TEST_DIR)/unit/test_error.c
TEST_ID = $(TEST_DIR)/unit/test_id.c
TEST_STRING = $(TEST_DIR)/unit/test_string.c
TEST_PATH = $(TEST_DIR)/unit/test_path.c
TEST_SHA256 = $(TEST_DIR)/unit/test_sha256.c
TEST_RANDOM = $(TEST_DIR)/unit/test_random.c
TEST_UTF8 = $(TEST_DIR)/unit/test_utf8.c
TEST_CRYPTO_BACKEND = $(TEST_DIR)/unit/test_crypto_backend.c

# Targets
.PHONY: all clean test test-all

all: dirs test

test-all: test check test-sanitize coverage-text

dirs:
	@mkdir -p $(OBJ_DIR)/error
	@mkdir -p $(OBJ_DIR)/types
	@mkdir -p $(OBJ_DIR)/crypto
	@mkdir -p $(OBJ_DIR)/utf8
	@mkdir -p $(BIN_DIR)

# Build object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Build and run tests
test: dirs $(ALL_OBJS)
	$(CC) $(CFLAGS) $(TEST_ERROR) $(ERROR_OBJS) -o $(BIN_DIR)/test_error $(LDFLAGS)
	$(CC) $(CFLAGS) $(TEST_ID) $(TYPES_OBJS) $(CRYPTO_OBJS) $(UTF8_OBJS) $(ERROR_OBJS) -o $(BIN_DIR)/test_id $(LDFLAGS)
	$(CC) $(CFLAGS) $(TEST_STRING) $(TYPES_OBJS) $(CRYPTO_OBJS) $(UTF8_OBJS) $(ERROR_OBJS) -o $(BIN_DIR)/test_string $(LDFLAGS)
	$(CC) $(CFLAGS) $(TEST_PATH) $(TYPES_OBJS) $(CRYPTO_OBJS) $(UTF8_OBJS) $(ERROR_OBJS) -o $(BIN_DIR)/test_path $(LDFLAGS)
	$(CC) $(CFLAGS) $(TEST_SHA256) $(CRYPTO_OBJS) $(ERROR_OBJS) -o $(BIN_DIR)/test_sha256 $(LDFLAGS)
	$(CC) $(CFLAGS) $(TEST_RANDOM) $(CRYPTO_OBJS) $(ERROR_OBJS) -o $(BIN_DIR)/test_random $(LDFLAGS)
	$(CC) $(CFLAGS) $(TEST_UTF8) $(UTF8_OBJS) -o $(BIN_DIR)/test_utf8 $(LDFLAGS)
	$(CC) $(CFLAGS) $(TEST_CRYPTO_BACKEND) $(CRYPTO_OBJS) $(ERROR_OBJS) -o $(BIN_DIR)/test_crypto_backend $(LDFLAGS)
	@echo "Running tests..."
	@$(BIN_DIR)/test_error
	@$(BIN_DIR)/test_id
	@$(BIN_DIR)/test_string
	@$(BIN_DIR)/test_path
	@$(BIN_DIR)/test_sha256
	@$(BIN_DIR)/test_random
	@$(BIN_DIR)/test_utf8
	@$(BIN_DIR)/test_crypto_backend

clean:
	rm -rf $(BUILD_DIR)
	rm -f *.gcda *.gcno *.gcov
	find . -name "*.gcda" -o -name "*.gcno" -o -name "*.gcov" | xargs rm -f

coverage: clean
	@echo "📊 Building with coverage enabled..."
	@$(MAKE) COVERAGE=1 test
	@echo "📊 Generating coverage report..."
	@gcovr -r . --html --html-details -o $(BUILD_DIR)/coverage.html \
		--exclude 'tests/*' \
		--exclude 'build/*' \
		--fail-under-line 80 \
		--fail-under-branch 70
	@echo "✅ Coverage report generated: $(BUILD_DIR)/coverage.html"
	@gcovr -r . --exclude 'tests/*' --exclude 'build/*'

coverage-text: clean
	@$(MAKE) COVERAGE=1 test
	@gcovr -r . --exclude 'tests/*' --exclude 'build/*' --fail-under-line 80 --fail-under-branch 70

test-sanitize: clean
	@echo "🧪 Running tests with sanitizers..."
	@$(MAKE) SANITIZE=1 test

lint:
	@echo "🔍 Running clang-tidy..."
	@find src include -name "*.c" -o -name "*.h" | \
		xargs clang-tidy -warnings-as-errors='*' \
		-header-filter='.*' \
		-- -I./include -DGITMIND_CORE_BUILD || \
		(echo "❌ Lint check failed" && exit 1)
	@echo "✅ Lint check passed!"

.PHONY: check
check: lint
	@echo "🔍 Running quality checks on core/..."
	@echo "✓ clang-format check"
	@find . -name "*.c" -o -name "*.h" | xargs clang-format --dry-run --Werror || (echo "❌ Format check failed" && exit 1)
	@echo "✓ All checks passed!"

.PHONY: clean coverage coverage-text test-sanitize lint check