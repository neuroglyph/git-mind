---
title: Standard Tools vs Custom Checks
description: Replace bespoke linters with robust, configurable standard tools.
audience: [contributors, developers]
domain: [quality]
tags: [lint, tooling]
status: stable
last_updated: 2025-09-15
---

# Standard Tools vs Custom Checks

Table of Contents

- [What We're Replacing](#what-were-replacing)
- [New Tool Stack](#new-tool-stack)
- [Performance Comparison](#performance-comparison)
- [Migration Plan](#migration-plan)
- [Pre-commit Strategy](#pre-commit-strategy)
- [Benefits of Standard Tools](#benefits-of-standard-tools)
- [What We Keep Custom](#what-we-keep-custom)
- [Summary](#summary)

## What We're Replacing

### ❌ Custom Function Length Checker → ✅ Clang-Tidy

__Why__: Our regex-based parser missed many functions. Clang-tidy properly parses C and catches:

- Functions over 15 lines
- Excessive nesting (>3 levels)
- Too many parameters (>5)
- Cyclomatic complexity

__Config__:

```yaml
readability-function-size.LineThreshold: 15
readability-function-size.BranchThreshold: 5
readability-function-size.NestingThreshold: 3
```

### ❌ Custom Magic Values Checker → ✅ Clang-Tidy

__Why__: Clang-tidy's `cppcoreguidelines-avoid-magic-numbers` is more sophisticated:

- Understands context (array indices, bit operations)
- Configurable ignored values
- Better false-positive handling

__Config__:

```yaml
cppcoreguidelines-avoid-magic-numbers.IgnoredIntegerValues: '0;1;2;-1'
```

### ✅ Custom Output Control Checker → Keep It

__Why__: This is specific to our project requirement that CLI respects `--verbose/--porcelain`. No standard tool checks this.

### ✅ Custom Dependency Injection Checker → Keep It

__Why__: Our DI requirements are project-specific. No standard tool enforces "no direct system calls".

### ✅ Custom Test Quality Checker → Keep It

__Why__: Our requirement that tests check behavior not output is unique. Standard tools don't enforce this.

## New Tool Stack

### 1. __Clang-Tidy__ (Primary)

- Function size/complexity
- Magic numbers
- Bug detection
- Security issues
- Style enforcement

### 2. __Cppcheck__ (Secondary)

- Memory leaks
- Buffer overruns
- Null pointer derefs
- Undefined behavior

### 3. __Lizard__ (Complexity)

- Cyclomatic complexity
- Function length
- Parameter count
- Token count

### 4. __Flawfinder__ (Security)

- Unsafe function usage
- Buffer overflow risks
- Format string issues

### 5. __Our Custom Checks__ (Project-Specific)

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

1. __Formatting__ - clang-format (fast)
2. __Function size__ - clang-tidy subset (medium)
3. __Magic numbers__ - clang-tidy subset (medium)
4. __CLI output__ - custom check (fast)

Full checks run in CI/CD.

## Benefits of Standard Tools

1. __Accuracy__: Proper C parsing, not regex
2. __Maintenance__: Tools maintained by others
3. __Features__: More checks available
4. __Integration__: IDE support, CI/CD plugins
5. __Documentation__: Well-documented warnings

## What We Keep Custom

1. __Output Control__: No tool checks --verbose/--porcelain
2. __Dependency Injection__: Our DI rules are specific
3. __Test Quality__: Behavior testing is our requirement
4. __Git-Mind Specific__: Project conventions

## Summary

- Replace broken regex-based checks with clang-tidy
- Keep project-specific requirement checks
- Use established tools where possible
- Custom only for unique requirements

This gives us the best of both worlds: reliable standard tools for common checks, custom tools for project-specific requirements.
