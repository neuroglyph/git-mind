# Instructions for Claude and AI Assistants

This file contains important instructions for Claude and other AI assistants working on the Neuroglyph project.

## Project Overview

- **Project Name**: git-mind
- **CLI Name**: git-mind
- **License**: Apache 2.0 (NOT MIT!)
- **Copyright**: © 2025 J. Kirby Ross / Neuroglyph Collective
- **Repository**: https://github.com/neuroglyph/git-mind

## Critical Rules

### 1. NEVER Make Git Commits or Operations in Working Repo
- The user will explicitly ask if they want commits made
- Default to NO commits unless specifically requested
- NEVER run git operations in the working repository
- All git operations must be in Docker containers or temp directories
- This is a hard rule with no exceptions

### 2. License Headers (SPDX)

All new files MUST include SPDX headers:

#### Source Code Files (.h, .c, .sh, etc.)
```c
/* SPDX-License-Identifier: Apache-2.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */
```

#### Shell Scripts
```bash
#!/bin/bash
# SPDX-License-Identifier: Apache-2.0
```

#### Configuration Files (Dockerfile, Makefile, .yml, .toml, .json)
```yaml
# SPDX-License-Identifier: Apache-2.0
```

#### Markdown Files (optional but encouraged)
```markdown
<!-- SPDX-License-Identifier: Apache-2.0 -->
<!-- © 2025 J. Kirby Ross / Neuroglyph Collective -->
```

### 3. License Information
- This project uses **Apache License 2.0** exclusively

### 5. Development Practices

#### Development Philosophy
- **TDD (Test-Driven Development)**: Write tests first, then implementation
- **SOLID Principles**: Single responsibility, Open/closed, Liskov substitution, Interface segregation, Dependency inversion
- **KISS**: Keep It Simple, Stupid - avoid unnecessary complexity
- **YAGNI**: You Aren't Gonna Need It - don't add functionality until needed
- **Test Double-Friendly Design**: Use dependency injection and traits to enable test doubles when needed

#### Testing
- ALWAYS run, build, or test in Docker NO MATTER WHAT
- Use `cd c && make test` to run the full test suite
- Pre-push hooks enforce test passing
- Write tests BEFORE implementation (TDD)
- Each function should have corresponding tests
- **Always use real Git repos in tests** - our entire stack relies on Git
- **NEVER run Git operations in the actual working repo** - only in Docker/temp dirs
- Create temporary Git repositories for each test
- All Git operations must be isolated from the working repository
- Use dependency injection for clean architecture
- Test doubles are only for contriving edge cases, not replacing Git
- **Always test behavior, not implementation** - Tests verify what the system does, not how
- **One test file per component** - Each test file focuses on testing one component's behavior

##### Test Strategy - Three Levels
Always have these three levels of tests, all focused on behavior:
1. **Unit Tests** - Test individual components in isolation
   - Located in `src/` files next to implementation (e.g., `#[cfg(test)] mod tests`)
   - Test single structs/functions behavior
   - Fast, focused on one thing
2. **Integration Tests** - Test components working together
   - Located in `tests/` directory
   - Test command behavior, file I/O, Git operations
   - Use real Git repos, real file systems
3. **End-to-End Tests** - Test complete user workflows
   - Also in `tests/` directory
   - Test full CLI commands from user perspective
   - Verify entire features work as users expect

**Every user story in `design/features/` should have corresponding end-to-end tests**
   
- **Every acceptance criteria should have its own specific test**

- **Tests should be organized by USER STORY by FEATURE** and should verify the user story's acceptance criteria

##### Testing Principles - NEVER Test stdout/stderr
- **NEVER test stdout/stderr output** - This is brittle and couples tests to implementation
- Testing stdout is just spying by another name - it violates behavior testing
- Instead, use proper return types:
  - Commands return `CommandResult<T>` with exit code and optional value
  - Tests check the exit code and returned values directly
  - The CLI layer handles printing - tests verify behavior, not output
- Example: Don't test "No links found" string, test that `list()` returns empty Vec

##### Definition of Done for Features
A feature is only complete when:
1. All acceptance criteria from `design/features/F*.md` have corresponding tests
2. Tests pass at all three levels (unit, integration, end-to-end)
3. User story is demonstrable through end-to-end tests
4. Documentation is updated
5. Code follows project conventions
6. TASKLIST.md has been updated

#### Code Style
- Keep functions small and focused (SOLID)
- Avoid premature optimization (YAGNI)
- **Single Responsibility Principle** - Each module/struct has one reason to change

### 6. Key Technical Details

#### Storage Model
- Links stored in `.gitmind/links/` directory
- Format: `LINK_TYPE: source -> target  # ts:timestamp`
- Files named by SHA of content
- **Git IS the database** - no external storage
- Every operation uses Git's content-addressable storage
- Testing must use real Git operations to be valid

#### Architectur- CLI first approach (no server required)
- Optional daemon for web UI
- Distributed by design
- Content-addressable storage

### 8. Important Files to Read
When starting work, always check:
1. `TASKLIST.md` - Current implementation status
2. `README.md` - Project overview (root)

## Common Tasks

### Adding a New Source File
1. Add SPDX header (see above)
2. Follow project conventions
3. Add tests
4. Update documentation if needed

### Updating Documentation
1. Add SPDX headers to new docs
2. Keep technical docs in `/design/` or `/docs/` (user docs)
4. Update `TASKLIST.md` when completing tasks

### Working with Git
1. NEVER commit without explicit permission
2. Use conventional commits when asked to commit or suggest commit messages
4. Run tests before any push (automatic via hooks)
5. When asked for commit messages, provide them in this format:
   ```bash
   git add <files> && git commit -m "type(scope): message"
   ```
6. Suggest commits after completing each task from TASKLIST.md
7. Keep TASKLIST.md up-to-date as tasks are completed

## Development Workflow Guidelines

### Tech Design and Feature Tracking
- Whenever we make a significant tech design decision, document it in `design/decisions`
- Whenever we complete a feature, update the document in `design/features` to reflect its status

## Final Reminders
- Apache 2.0, not MIT!
- No commits without permission!
- SPDX headers on all new files!
- Test everything in Docker!

## Development Workflow Example

When implementing a new feature:
1. **Start with tests** (TDD)
2. **Design with DI** for testability
3. **Write minimal code** to make tests pass (KISS)
4. **Use real Git repos** in tests, test doubles only when needed, work from temporary directories
5. **Refactor** if needed while keeping tests green
6. **Don't add extra features** (YAGNI)
7. **Keep each module focused** on one responsibility (SRP)

## Important Development Practices

### Logging and Journal
- Leave your impressions or thoughts in `.claude/claude-mind/YYYY-MM-DD/` directory
- Always use today's actual date (double-check to avoid date errors!)
- Create per-session files to avoid merge conflicts:
  - Format: `HH:MM-topic-sessionID.md` (e.g., `14:45-web-viz-abc123.md`)
  - Use first 6 chars of conversation ID for sessionID
- Use posix timestamps (e.g., `1749941840`)
- Your "Collaborator" identity comes from  `git confg user.name` and `git config user.email`, but protected  i.e. "[J. Kirby Ross](james@flyingrobots.dev)"
- Include session metadata at the start of each file:
  ```markdown
  # Claude Development Journal
  
  ## Session: 1749941840
  **Collaborator**: [J. Kirby Ross](james@flyingrobots.dev)
  **Topics**: Web visualization, Help system  
  **Conversation**: abc123def456 (first 12 chars)
  
  ### 14:45 UTC - Initial Thoughts
  [content...]
  ```
- This is your dev journal - feel free to write freely when you get ideas or insights
- Create an `index.md` in each day's directory summarizing sessions

---
*Last updated: June 2025*