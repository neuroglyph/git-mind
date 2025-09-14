<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->
<!-- © 2025 J. Kirby Ross / Neuroglyph Collective -->

# git-mind

> _"Remember who you are."_ - Mufasa

Table of Contents

- [Name](#name)
- [Synopsis](#synopsis)
- [Description](#description)
- [The Vision](#the-vision)
- [Commands](#commands)

## NAME

`git-mind` - Semantic version control for the relationships between your files

## SYNOPSIS

```bash
git mind <command> [<args>]
```

## DESCRIPTION

In the great Circle of Development, code is written, connections form, understanding grows, and knowledge must be passed on. But traditional version control only tracks what changed, not why it matters or how it relates.

git-mind adds a semantic layer to Git, allowing you to create, query, and explore meaningful relationships between files. These connections travel through time with your code, creating a permanent record of your understanding.

_"Everything the light touches has been connected by git-mind."_

## THE VISION

You're deep in a codebase. Instead of asking:

- "Where is this implemented?" (grep)
- "What depends on this?" (hope)
- "Where are the tests?" (find)

You ask:

- "Show me all implementations of this design"
- "What breaks if I change this?"  
- "How has our understanding evolved?"

And git-mind answers, instantly, accurately, with the wisdom of everyone who came before.

## COMMANDS

### Core Commands

`link <source> <target> --type <relationship>`
: Create semantic connection between files

`list [options]`
: Query and explore connections

`traverse <file> [--depth N]`
: Follow connection paths from starting point _(coming soon)_

`unlink <source> <target>`
: Remove connection (adds tombstone) _(coming soon)_

### Management Commands

`cache-rebuild [--branch <branch>]`
: Rebuild Roaring Bitmap cache for fast queries

`install-hooks`
: Install Git hooks for automatic evolution tracking

`status`
: Show semantic graph statistics _(coming soon)_

`gc [--aggressive]`
: Compress old journal entries _(coming soon)_

### Utility Commands

`version`
: Display version information

`help [<command>]`
: Show help for commands

## HOW IT WORKS

### The Journal Layer (Truth)

Every semantic link is stored as a Git commit in refs/gitmind/edges/<branch>. These commits:

- Contain CBOR-encoded edge data
- Are branch-specific
- Push/pull with your code
- Survive forever in Git history

### The Cache Layer (Speed)

For repositories with thousands of connections, git-mind builds Roaring Bitmap indices:

- O(log N) query performance
- Sharded by SHA prefix
- Stored in refs/gitmind/cache/<branch>
- Rebuilt on demand

### The Evolution System (AUGMENTS)

When files change, their blob SHAs change. The AUGMENTS system:

- Tracks file evolution automatically
- Preserves connections through changes
- Installed via post-commit hook
- Creates chains: old version → new version

## EXAMPLES

### Starting Your Semantic Journey

```bash
# Initial setup (happens automatically on first link)
$ cd myproject
$ git mind link README.md docs/quickstart.md --type references
✓ Created connection: README.md ──references──> docs/quickstart.md

# See what you've connected
$ git mind list
README.md ──references──> docs/quickstart.md
```

### Building Understanding

```bash
# Document implementation relationships
$ git mind link src/parser.c docs/parser-design.md --type implements
$ git mind link src/lexer.c docs/parser-design.md --type implements

# Track test coverage
$ git mind link tests/test_parser.c src/parser.c --type tests
$ git mind link tests/test_lexer.c src/lexer.c --type tests

# Show everything connected to the design
$ git mind list --to docs/parser-design.md
src/parser.c ──implements──> docs/parser-design.md
src/lexer.c ──implements──> docs/parser-design.md
```

### Time Travel Through Understanding

```bash
# Current state
$ git mind list --from src/api.c
src/api.c ──depends_on──> config/api.yaml
src/api.c ──implements──> docs/api-v2.md

# Travel back 30 commits
$ git checkout HEAD~30
$ git mind list --from src/api.c  
src/api.c ──implements──> docs/api-v1.md
src/api.c ──experimental──> research/new-approach.pdf

# Return to present
$ git checkout main
```

### Automatic Evolution Tracking

```bash
# Install hooks
$ git mind install-hooks
✓ Hooks installed - file evolution will be tracked automatically

# Edit a file that has connections
$ vim src/parser.c
$ git add src/parser.c && git commit -m "Optimize parser"

# The connections followed the change!
$ git mind list --from src/parser.c
src/parser.c ──implements──> docs/parser-design.md
# (connection automatically updated to new blob SHA)
```

### Performance at Scale

```bash
# After months of development
$ git mind status
Semantic Graph Statistics:
- Total edges: 3,847
- Unique relationships: 12 types
- Most connected: src/core.c (147 connections)

# Queries getting slow?
$ git mind cache-rebuild
Rebuilding cache for branch 'main'...
✓ Processed 3,847 edges into 156KB cache
✓ Queries now 500x faster!
```

## PHILOSOPHY

### Why Journal Commits

_"It's the Circle of Life, and it moves us all"_

Traditional metadata systems fight Git's nature. They store data outside the history, create merge conflicts, and lose synchronization. git-mind embraces Git's design:

- __Commits are immutable__ - Your connections can't be lost
- __Branches isolate changes__ - Different understanding on different branches
- __History is permanent__ - Travel through time to see past understanding
- __Merging is managed__ - Git handles conflicts naturally

### Why Semantic Connections

_"There's more to being king than getting your way"_

Code without context is just syntax. Understanding comes from relationships:

- Why was this built?
- What does it implement?
- What depends on it?
- How has it evolved?

git-mind makes these relationships first-class citizens of your repository.

### Why Performance Matters

_"Life's not fair, is it?"_

A tool unused is worthless. git-mind stays fast through:

- Pure C implementation
- Roaring Bitmap indices  
- O(log N) query complexity
- Minimal dependencies

Even with millions of edges, queries complete in milliseconds.

## INTEGRATION

### Git Configuration

```bash
# Push semantic refs automatically
git config --add remote.origin.push 'refs/gitmind/*:refs/gitmind/*'

# Fetch semantic refs automatically  
git config --add remote.origin.fetch '+refs/gitmind/*:refs/gitmind/*'
```

### Editor Integration

_(Coming soon)_ Plugins for:

- VS Code - See connections in sidebar
- Vim - Query connections with :GitMindList
- Emacs - Semantic navigation with git-mind-mode

### CI/CD Integration

```yaml
# GitHub Actions example
- name: Check semantic coverage
  run: |
    git mind list --type tests --format json > test-coverage.json
    # Ensure all source files have tests
```

## THE DEEPER MAGIC

### For Individuals

*"Look inside yourself. You are more than what you have become."_

Your understanding is valuable. git-mind preserves it, letting you:

- Remember why you made decisions
- Navigate by meaning, not just files
- Build on your past insights

### For Teams

*"We are one."_

Shared understanding is your competitive advantage:

- Onboard faster with visible relationships
- Preserve knowledge through transitions
- Build collective intelligence

### For the Future

*"It is time."_

What starts as a CLI becomes:

- Infrastructure for AI agents
- Foundation for semantic IDEs
- Bridge between human and machine understanding

## ENVIRONMENT

`GIT_MIND_BRANCH`
: Override branch for operations (default: current)

`GIT_MIND_NO_CACHE`
: Disable cache usage (force journal scan)

`GIT_MIND_VERBOSE`
: Enable verbose output

## FILES

`.git/refs/gitmind/edges/<branch>`
: Journal commits with semantic edges

`.git/refs/gitmind/cache/<branch>`
: Roaring Bitmap cache trees

`.git/hooks/post-commit`
: Hook for automatic evolution tracking

## EXIT STATUS

`0`
: Success

`1`
: General error

`2`
: Usage error (invalid arguments)

`3`
: Git error (not in repository, etc.)

## SEE ALSO

Individual command documentation:

- `git-mind-link(1)` - Create connections
- `git-mind-list(1)` - Query connections
- `git-mind-cache-rebuild(1)` - Rebuild performance cache
- `git-mind-install-hooks(1)` - Setup evolution tracking

Git documentation:

- `git(1)` - The stupid content tracker
- `gitglossary(7)` - Git glossary

## HISTORY

git-mind emerged from a simple observation: we spend enormous effort understanding code, then throw that understanding away. What if we could capture it?

The journey from idea to implementation taught us:

- Simple is powerful (just Git commits)
- Performance enables adoption (Roaring Bitmaps)
- Evolution is inevitable (AUGMENTS system)
- Understanding is the real product

## AUTHORS

Created by J. Kirby Ross and the Neuroglyph Collective.

_"Hakuna Matata"_ - No worries, your semantic connections are safe.

## THE PROMISE

_"Remember who you are."_

Your code tells a story. git-mind helps you remember it, share it, and build upon it. In the great Circle of Development, no understanding is lost, no connection forgotten, no wisdom wasted.

Welcome to semantic version control. Welcome to git-mind.

---

_"Oh yes, the past can hurt. But the way I see it, you can either run from it or learn from it."_

__Choose to learn. Choose git-mind.__
