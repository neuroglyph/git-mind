---
title: Static Analysis Tools for Git-Mind
description: Overview of clang-tidy and related tools for static analysis in this repo.
audience: [contributors, developers]
domain: [quality]
tags: [clang-tidy, static-analysis]
status: draft
last_updated: 2025-09-15
---

# Static Analysis Tools for Git-Mind

Table of Contents

- [Overview of Static Analysis Tools](#overview-of-static-analysis-tools)
- [Currently Configured Tools](#currently-configured-tools-)
- [Additional Tools Available](#additional-tools-available)
- [Comparison Matrix](#comparison-matrix)
- [Recommended Setup](#recommended-setup)
- [Integration with Git-Mind](#integration-with-git-mind)
- [Dealing with False Positives](#dealing-with-false-positives)

## Overview of Static Analysis Tools

Static analysis examines code without executing it, finding bugs that testing might miss. Here's what we have configured and what's available:

## Currently Configured Tools ✅

### 1. __Clang-Tidy__ (CONFIGURED)

- __What it catches__: Style violations, common bugs, performance issues
- __Strengths__: Highly configurable, integrates with build system
- __Our config__: Enforces 15-line functions, catches magic numbers
- __Run__: `make lint`

### 2. __Cppcheck__ (CONFIGURED)

- __What it catches__: Memory leaks, buffer overruns, null derefs
- __Strengths__: Zero false positives philosophy, very reliable
- __Our config__: All checks enabled except system headers
- __Run__: `make lint`

### 3. __Custom Checkers__ (CONFIGURED)

- Function length enforcement
- Magic value detection
- Output control verification
- Dependency injection checking
- __Run__: `make check-quality`

## Additional Tools Available

### 4. __Clang Static Analyzer__ (scan-build)

```bash
# Install
apt install clang-tools

# Run deep analysis
scan-build -enable-checker alpha.security make -C src

# View results
scan-view /tmp/scan-build-*
```

__Catches__: Deep path-sensitive bugs, security vulnerabilities

### 5. __GCC Static Analyzer__ (-fanalyzer)

```bash
# Requires GCC 10+
make -C src CFLAGS="-fanalyzer -Wanalyzer-too-complex"
```

__Catches__: Buffer overflows, use-after-free, taint analysis

### 6. __PVS-Studio__ (Commercial, Free for OSS)

```bash
# Register for free OSS license
# https://pvs-studio.com/en/order/open-source-license/

# Install and run
pvs-studio-analyzer analyze -o pvs.log
plog-converter -a GA:1,2 -t errorfile pvs.log -o report.txt
```

__Catches__: Advanced bugs, typos, security issues
__Strengths__: Very low false positive rate

### 7. __Coverity__ (Free for OSS)

```bash
# Register at https://scan.coverity.com/
# Download Coverity Build Tool

cov-build --dir cov-int make -C src
tar czf git-mind.tgz cov-int
# Upload to Coverity Scan
```

__Catches__: Complex interprocedural bugs, security vulnerabilities
__Used by__: Linux kernel, Firefox, Python

### 8. __CodeQL__ (GitHub)

```yaml
# .github/workflows/codeql.yml
name: "CodeQL"
on: [push, pull_request]
jobs:
  analyze:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v3
    - uses: github/codeql-action/init@v2
      with:
        languages: cpp
    - run: make -C src
    - uses: github/codeql-action/analyze@v2
```

__Catches__: Security vulnerabilities, code patterns
__Strengths__: Semantic code analysis, custom queries

### 9. __Infer__ (Facebook)

```bash
# Install
brew install infer  # macOS
apt install infer   # Linux

# Run
infer run -- make -C src
```

__Catches__: Null derefs, memory leaks, race conditions
__Strengths__: Interprocedural, incremental analysis

### 10. __Flawfinder__ (Security-focused)

```bash
# Install
pip install flawfinder

# Run
flawfinder --html --context src/ > security-report.html
```

__Catches__: Security vulnerabilities, unsafe functions

## Comparison Matrix

| Tool | Type | False Positives | Speed | Focus |
|------|------|----------------|-------|-------|
| Clang-Tidy | Style + Bugs | Medium | Fast | General |
| Cppcheck | Bugs | Very Low | Fast | Reliability |
| Clang Analyzer | Deep Bugs | Low | Slow | Security |
| GCC Analyzer | Deep Bugs | Medium | Slow | Memory |
| PVS-Studio | All | Very Low | Medium | Quality |
| Coverity | Deep Bugs | Low | Slow | Enterprise |
| CodeQL | Security | Low | Medium | Patterns |
| Infer | Memory/Null | Low | Fast | Facebook |
| Flawfinder | Security | High | Very Fast | OWASP |

## Recommended Setup

### For Development (Fast Feedback)

```bash
# Quick checks on every commit
make lint              # Clang-tidy + Cppcheck
make check-quality     # Custom checks
```

### For CI/CD (Thorough)

```yaml
# GitHub Actions
- Run clang-tidy
- Run cppcheck  
- Run custom checks
- Run CodeQL
- Run gcc -fanalyzer
```

### For Releases (Deep Analysis)

```bash
# Run everything
./tools/code-quality/static-analysis.sh

# Plus cloud services
- Coverity Scan (weekly)
- PVS-Studio (monthly)
```

## Integration with Git-Mind

### Pre-commit (Quick)

- Function length check
- Magic value detection
- Basic formatting

### Pre-push (Medium)

- Clang-tidy
- Cppcheck
- Custom quality checks

### CI Pipeline (Full)

- All of the above
- Clang Static Analyzer
- GCC analyzer
- CodeQL

### Release Process (Everything)

- All of the above
- Coverity Scan
- PVS-Studio
- Security audit with Flawfinder

## Dealing with False Positives

### 1. Suppress with Comments

```c
// cppcheck-suppress uninitvar
int x = get_value();  // OK: get_value initializes via pointer

// NOLINTNEXTLINE(cppcoreguidelines-avoid-magic-numbers)
const int BUFFER_SIZE = 8192;  // OK: Well-known value
```

### 2. Configure Tool

```yaml
# .clang-tidy
Checks: '-readability-magic-numbers'
CheckOptions:
  - key: cppcoreguidelines-avoid-magic-numbers.IgnoredIntegerValues
    value: '0;1;100;1000'
```

### 3. Fix the Code

Often the tool is right! Consider if the warning indicates a real issue.

## ROI Analysis

### High Value (Use Always)

- __Clang-tidy__: Fast, catches real bugs
- __Cppcheck__: Very few false positives
- __Custom checks__: Enforce project standards

### Medium Value (Use in CI)

- __Clang Analyzer__: Deeper but slower
- __GCC analyzer__: Different perspective
- __CodeQL__: Great for security

### Situational Value

- __PVS-Studio__: Excellent but commercial
- __Coverity__: Great but requires setup
- __Infer__: Good for specific bug classes

## Getting Started

1. __Now__: Run `make lint` locally
2. __Today__: Install pre-commit hooks
3. __This Week__: Add to CI pipeline
4. __This Month__: Register for Coverity/PVS-Studio

## Summary

We already have good static analysis coverage with:

- Clang-tidy (configured)
- Cppcheck (configured)
- Custom project-specific checks
- Pre-commit hooks
- CI integration

Additional tools to consider:

- Clang Static Analyzer for security
- CodeQL for GitHub integration
- Coverity/PVS-Studio for deep analysis

The current setup catches most issues. Additional tools provide diminishing returns but can find subtle bugs in critical code.
