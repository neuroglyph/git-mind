# Code Quality Tools for Git-Mind

This directory contains automated tools to enforce code quality standards and prevent technical debt.

## Quick Start

```bash
# Run all quality checks
make check-quality

# Run linting with multiple tools
make lint

# Auto-format code
make format

# Install git hooks for pre-commit checks
make install-hooks
```

## Available Checks

### 1. Function Length Check (`check-function-length.sh`)

- Enforces 15-line maximum for all functions
- Catches deeply nested code (>3 levels)
- Reports exact line numbers and function names

### 2. Magic Values Check (`check-magic-values.sh`)

- Detects hardcoded numbers > 99
- Finds string literals that should be constants
- Excludes legitimate uses (array indices, bit shifts)

### 3. Output Control Check (`check-output-control.sh`)

- Ensures CLI respects `--verbose` and `--porcelain`
- Verifies library code is silent by default
- Catches direct printf/fprintf usage

### 4. Dependency Injection Check (`check-dependency-injection.sh`)

- Detects direct system calls (time, rand, file I/O)
- Ensures Git operations use abstraction layer
- Promotes testable architecture

### 5. Test Quality Check (`check-test-quality.sh`)

- Ensures tests check behavior, not output
- Detects stdout/stderr assertions
- Verifies test organization by feature

## Linting Tools

### Clang-Format

- Enforces consistent code style
- 80 column limit
- 4-space indentation
- Configured in `.clang-format`

### Clang-Tidy

- Static analysis for common bugs
- Enforces 15-line function limit
- Catches magic numbers
- Configured in `.clang-tidy`

### Cppcheck

- Additional static analysis
- Memory leak detection
- Undefined behavior checks
- Performance suggestions

### Lizard (Cyclomatic Complexity)

- Measures code complexity
- Enforces maximum complexity of 10
- Tracks function arguments (<= 3)

## Git Hooks

The pre-commit hook runs quick checks on staged files:

- Function length validation
- Magic number detection
- CLI output control verification

Install with: `make install-hooks`

## CI Integration

All checks run automatically on:

- Push to main/dev branches
- Pull requests
- GitHub Actions workflow in `.github/workflows/code-quality.yml`

## Fixing Issues

When checks fail:

1. __Function too long__: Break into smaller functions

   ```c
   // Before: 50-line function
   void process_all() { /* lots of code */ }
   
   // After: Multiple focused functions
   void validate_input() { /* 10 lines */ }
   void transform_data() { /* 12 lines */ }
   void save_results() { /* 8 lines */ }
   ```

2. __Magic values__: Define constants

   ```c
   // Before
   if (size > 8192) { /* bad */ }
   
   // After
   #define MAX_BUFFER_SIZE 8192
   if (size > MAX_BUFFER_SIZE) { /* good */ }
   ```

3. __Output control__: Use abstraction

   ```c
   // Before
   printf("Processing...\n");
   
   // After
   gm_output_print(ctx->output, "Processing...");
   ```

4. __Direct system calls__: Use injection

   ```c
   // Before
   time_t now = time(NULL);
   
   // After
   time_t now = ctx->time_ops->time(NULL);
   ```

## Gradual Adoption

For existing code with many violations:

1. Run `make fix-tech-debt` to see full task list
2. Fix critical functions first (>100 lines)
3. Add constants for magic values
4. Implement output abstraction
5. Add dependency injection gradually

## Configuration

- `.clang-format`: Code style rules
- `.clang-tidy`: Static analysis rules
- `.githooks/pre-commit`: Git hook configuration
- `tools/code-quality/*.sh`: Individual check scripts

## Adding New Checks

To add a new quality check:

1. Create script in `tools/code-quality/`
2. Add to `run-all-checks.sh`
3. Update pre-commit hook if needed
4. Add to CI workflow
5. Document in this README

## Exemptions

If you must violate a rule, mark it clearly:

```c
// OK: Performance critical inner loop
for (int i = 0; i < 1000000; i++) { /* ... */ }

// OK: Direct output for emergency errors
fprintf(stderr, "FATAL: %s\n", error);
```

## Goal

Zero quality violations across the codebase, enabling:

- Easy maintenance
- Confident refactoring
- Fast onboarding
- Reliable testing
- Clean architecture
