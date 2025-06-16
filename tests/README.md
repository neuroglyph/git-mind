# GitMind Test Suite

This directory contains all tests for the GitMind C implementation.

## Structure

```
tests/
├── unit/                    # C unit tests (future)
│   ├── test_path_validation.c
│   ├── test_memory_leaks.c
│   └── CMakeLists.txt
├── integration/             # Shell-based integration tests
│   ├── test.sh             # Core functionality tests
│   ├── test-traverse.sh    # Graph traversal tests
│   ├── test-path-validation.sh  # Security tests
│   ├── test-output-modes.sh     # CLI output format tests
│   ├── test-regression.sh       # Regression test suite
│   ├── test-depth-error.sh      # Error handling tests
│   └── valgrind-test.sh        # Memory leak detection
├── scripts/                 # Test infrastructure
│   ├── run-all-tests.sh   # Main test runner
│   ├── docker-guard.sh    # Ensures tests run in Docker
│   └── add-docker-guards.sh # Helper to add guards
├── tools/                   # Special testing tools
│   ├── fuzz-test.sh       # AFL++ fuzzing
│   ├── test-multi-compiler.sh  # Cross-compiler testing
│   └── docker-test.sh     # Docker environment test
└── fixtures/               # Test data (future)
```

## Running Tests

**IMPORTANT**: All tests MUST be run inside Docker for safety and consistency.

### Run all tests:
```bash
# From repository root
make test

# From c/ directory
make test
```

### Run specific test suites:
```bash
# Run only integration tests
docker compose run --rm test ./tests/integration/test.sh

# Run path validation tests
docker compose run --rm test ./tests/integration/test-path-validation.sh
```

### Run tests in random order:
```bash
make test-random
```

## Docker Safety

All test scripts include `docker-guard.sh` which prevents accidental execution outside Docker. This is critical because:

1. Tests create real Git repositories
2. Tests perform real Git operations  
3. Running tests on your working repo could be destructive

If you see this error:
```
❌ FATAL ERROR: This script must be run inside Docker!
```

Use `make test` instead of running scripts directly.

## Writing New Tests

### Integration Tests

1. Create a new script in `tests/integration/`
2. Start with the Docker guard:
   ```bash
   #!/bin/bash
   source "$SCRIPT_DIR/../scripts/docker-guard.sh"
   ```
3. Use the test helpers and patterns from existing tests
4. Add your test to `run-all-tests.sh`

### Unit Tests

Unit tests in C are planned for the future using a framework like cmocka or Unity.

## Test Coverage

Current test coverage includes:

- **Core Commands**: init, link, list, unlink, status
- **Graph Traversal**: BFS traversal with cycle detection
- **Path Security**: Path traversal and injection protection
- **Output Formats**: Porcelain and human-readable modes
- **Memory Safety**: Valgrind leak detection
- **Error Handling**: Proper error codes and messages
- **Regression Suite**: Tests for all security fixes

## Continuous Integration

Tests are run automatically on:
- Every push via GitHub Actions
- Pull request checks
- Pre-push Git hooks (if enabled)

## Performance

The test suite is designed to be fast:
- Most tests complete in < 1 second
- Full suite runs in < 30 seconds
- Parallel execution planned for future