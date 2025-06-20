# Standard Tools vs Custom Checks

## What We're Replacing

### ❌ Custom Function Length Checker → ✅ Clang-Tidy
**Why**: Our regex-based parser missed many functions. Clang-tidy properly parses C and catches:
- Functions over 15 lines
- Excessive nesting (>3 levels)
- Too many parameters (>5)
- Cyclomatic complexity

**Config**:
```yaml
readability-function-size.LineThreshold: 15
readability-function-size.BranchThreshold: 5
readability-function-size.NestingThreshold: 3
```

### ❌ Custom Magic Values Checker → ✅ Clang-Tidy
**Why**: Clang-tidy's `cppcoreguidelines-avoid-magic-numbers` is more sophisticated:
- Understands context (array indices, bit operations)
- Configurable ignored values
- Better false-positive handling

**Config**:
```yaml
cppcoreguidelines-avoid-magic-numbers.IgnoredIntegerValues: '0;1;2;-1'
```

### ✅ Custom Output Control Checker → Keep It!
**Why**: This is specific to our project requirement that CLI respects `--verbose/--porcelain`. No standard tool checks this.

### ✅ Custom Dependency Injection Checker → Keep It!
**Why**: Our DI requirements are project-specific. No standard tool enforces "no direct system calls".

### ✅ Custom Test Quality Checker → Keep It!
**Why**: Our requirement that tests check behavior not output is unique. Standard tools don't enforce this.

## New Tool Stack

### 1. **Clang-Tidy** (Primary)
- Function size/complexity
- Magic numbers
- Bug detection
- Security issues
- Style enforcement

### 2. **Cppcheck** (Secondary)
- Memory leaks
- Buffer overruns
- Null pointer derefs
- Undefined behavior

### 3. **Lizard** (Complexity)
- Cyclomatic complexity
- Function length
- Parameter count
- Token count

### 4. **Flawfinder** (Security)
- Unsafe function usage
- Buffer overflow risks
- Format string issues

### 5. **Our Custom Checks** (Project-Specific)
- Output control (--verbose/--porcelain)
- Dependency injection
- Test quality (no stdout testing)

## Performance Comparison

| Check Type | Custom Tool | Standard Tool | Speed |
|------------|-------------|---------------|-------|
| Function Length | 0.5s (broken) | 2s (accurate) | Slower but correct |
| Magic Numbers | 1s | 2s | Similar |
| All Checks | 3s | 8s | Slower but comprehensive |

## Migration Plan

### Phase 1: Dual Mode (Current)
- Keep custom checks as `check-quality-legacy`
- New standard tools as `check-quality`
- Compare results

### Phase 2: Deprecate Broken Checks
- Remove custom function length checker
- Remove custom magic number checker
- Keep project-specific checks

### Phase 3: Optimize
- Use clang-tidy cache
- Incremental checking
- Pre-commit uses quick subset

## Pre-commit Strategy

Instead of running all checks (slow), pre-commit runs:
1. **Formatting** - clang-format (fast)
2. **Function size** - clang-tidy subset (medium)
3. **Magic numbers** - clang-tidy subset (medium)
4. **CLI output** - custom check (fast)

Full checks run in CI/CD.

## Benefits of Standard Tools

1. **Accuracy**: Proper C parsing, not regex
2. **Maintenance**: Tools maintained by others
3. **Features**: More checks available
4. **Integration**: IDE support, CI/CD plugins
5. **Documentation**: Well-documented warnings

## What We Keep Custom

1. **Output Control**: No tool checks --verbose/--porcelain
2. **Dependency Injection**: Our DI rules are specific
3. **Test Quality**: Behavior testing is our requirement
4. **Git-Mind Specific**: Project conventions

## Summary

- Replace broken regex-based checks with clang-tidy
- Keep project-specific requirement checks
- Use established tools where possible
- Custom only for unique requirements

This gives us the best of both worlds: reliable standard tools for common checks, custom tools for project-specific requirements.