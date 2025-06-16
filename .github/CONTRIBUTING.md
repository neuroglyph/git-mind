<!-- SPDX-License-Identifier: Apache-2.0 -->
# Contributing to Neuroglyph

Thank you for your interest in contributing to Neuroglyph! We're building a protocol for transforming Git into a substrate for distributed semantic memory, and we'd love your help.

## 🚀 Getting Started

### Prerequisites

- Git 2.40+
- C compiler (gcc, clang, or compatible)
- Make
- Basic understanding of Git internals is helpful but not required
- Docker Desktop (optional, for consistent testing environment)

### Development Setup

1. Fork and clone the repository:
   ```bash
   git clone https://github.com/YOUR_USERNAME/neuroglyph.git
   cd neuroglyph
   ```

2. Install Docker:
   - [Docker Desktop](https://www.docker.com/products/docker-desktop/) (recommended)
   - Ensure Docker is running

3. Install git hooks:
   ```bash
   make install-hooks
   ```

4. Build the project:
   ```bash
   cd c
   make
   ```

   Or use Docker:
   ```bash
   make docker-test
   ```

### Running Tests

All tests MUST be run in Docker to ensure consistency with CI:

```bash
# Run full test suite
cd c && make test

# Run tests in Docker (same as CI)
make docker-test

# For C development
cd c
make          # Build binary
make test     # Run tests
make clean    # Clean build artifacts
```

**Important:** The pre-push hook will automatically run `make test` before allowing pushes.

## 📝 Code Style Guidelines

### C Code

- Follow K&R or Linux kernel style
- Use meaningful variable and function names
- Always use `snprintf` instead of `sprintf`
- Check return values from system calls
- Free allocated memory appropriately
- Write tests for new functionality
- Document functions with clear comments

### Commit Messages

Follow the conventional commits format:
```
type(scope): subject

body (optional)

footer (optional)
```

Types:
- `feat`: New feature
- `fix`: Bug fix
- `docs`: Documentation changes
- `style`: Code style changes (formatting, etc.)
- `refactor`: Code refactoring
- `test`: Test additions or changes
- `chore`: Build process or auxiliary tool changes

Example:
```
feat(cli): add gitmind init command

Implements the initialization of .gitmind directory structure
and creates necessary refs for semantic links.

Closes #42
```

## 🔄 Pull Request Process

1. **Create an issue first** - Discuss the change you wish to make
2. **Fork and create a branch** - Use a descriptive branch name
3. **Make your changes** - Follow the code style guidelines
4. **Add tests** - Ensure your changes are tested
5. **Update documentation** - If you changed behavior, update docs
6. **Run checks locally**:
   ```bash
   make test  # This runs all checks in Docker
   ```
7. **Submit PR** - Reference the issue and provide clear description

### PR Template

Your PR description should include:
- What problem does this solve?
- How does it solve it?
- What are the key changes?
- Testing performed
- Screenshots (if UI changes)

## 🧪 Testing Requirements

### 🐳 Why All Tests Run in Docker

**Every test MUST run in Docker.** This is not optional. Here's why:

1. **Consistency Across Environments**
   - No "works on my machine" issues
   - Same compiler, same libraries, same behavior
   - CI uses Docker, so local tests match exactly

2. **Real Git Operations = Real Danger**
   - Our tests create actual Git repositories
   - They make real commits, branches, and merges
   - Running these on your working directory would be catastrophic:
     - Could corrupt your uncommitted work
     - Might destroy your .git directory
     - Could conflict with your current branch
     - Would definitely ruin your day

Docker provides isolated, disposable environments where tests can:
- Create and destroy Git repos safely
- Run dangerous edge cases without risk
- Execute Git operations in parallel
- Fail spectacularly without consequences

**Never run tests outside Docker. Your repository will thank you.**

### Unit Tests
- Test individual functions in isolation
- Use temporary directories for file operations
- Never touch the actual working repository

### Integration Tests
- Test CLI commands end-to-end
- Create temporary Git repositories in Docker
- Test error cases and edge conditions
- Verify Git operations work correctly

### Example Test (C)
```c
void test_link_creation() {
    // Create isolated temp directory
    char tmpdir[] = "/tmp/gitmind_test_XXXXXX";
    mkdtemp(tmpdir);
    
    // Initialize git repo IN THE TEMP DIR
    system_fmt("cd %s && git init", tmpdir);
    gm_init(tmpdir);
    
    // Test link creation
    assert(gm_link_create("doc1.md", "doc2.md", "REFS") == 0);
    
    // Cleanup
    system_fmt("rm -rf %s", tmpdir);
}
```

## 🎯 What to Work On

### Good First Issues
Look for issues labeled `good first issue` - these are ideal for newcomers.

### Priority Areas
1. **CLI Commands** - Implementing core functionality
2. **Performance** - Optimizing for large repositories
3. **Documentation** - Improving guides and examples
4. **Testing** - Increasing test coverage
5. **Cross-platform** - Ensuring Windows/macOS/Linux compatibility

## 💬 Communication

- **GitHub Issues** - For bug reports and feature requests
- **GitHub Discussions** - For questions and ideas
- **Pull Request Comments** - For code review discussions

## 🤝 Code of Conduct

Please read and follow our [Code of Conduct](CODE_OF_CONDUCT.md). We're committed to maintaining a welcoming and inclusive community.

## 📜 License

By contributing to Neuroglyph, you agree that your contributions will be licensed under the Apache License 2.0.

## 🙏 Recognition

Contributors will be recognized in:
- The project README
- Release notes
- Special thanks in documentation

Thank you for helping make Gitmind better! 🐵✨