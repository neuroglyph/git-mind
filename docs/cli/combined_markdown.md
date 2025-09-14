# README.md
<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->
<!-- © 2025 J. Kirby Ross / Neuroglyph Collective -->

# git-mind Command Reference

> *"The great kings of the past look down on us from those stars... So whenever you feel alone, just remember that those kings will always be there to guide you, and so will I."* - Mufasa

Table of Contents
- [Overview](#overview)
- [Core Commands](#core-commands)
- [Performance Commands](#performance-commands)
- [Evolution Commands](#evolution-commands)
- [Coming Soon](#coming-soon)

## Overview

Welcome to the git-mind command reference. These guides will teach you not just how to use each command, but why they exist and what they enable.

## Core Commands

### [git-mind](gitmind.md)
The main command and your entry into semantic version control. Start here to understand the vision.

### [git-mind link](gitmind-link.md)  
Create meaningful connections between files. This is the heart of git-mind - where understanding becomes permanent.

### [git-mind list](gitmind-list.md)
Query and explore your semantic web. Your window into the relationships that bind your code.

## Performance Commands

### [git-mind cache-rebuild](gitmind-cache-rebuild.md)
Harness the power of Roaring Bitmaps for lightning-fast queries. When your semantic web grows large, this command keeps it fast.

## Evolution Commands

### [git-mind install-hooks](gitmind-install-hooks.md)
Install automatic evolution tracking. Let git-mind follow your files as they change and grow.

## Coming Soon

### git-mind traverse
Follow connection paths through your codebase. Like `git log` but for relationships.

### git-mind unlink  
Remove connections gracefully with tombstone edges.

### git-mind status
See statistics about your semantic graph.

### git-mind gc
Compress old journal entries for long-lived repositories.

## The Philosophy

*"Look beyond what you see."* - Rafiki

These aren't just commands. They're tools for encoding understanding. Every link you create, every query you run, adds to the collective wisdom of your codebase.

Traditional documentation tells you what the code does. Semantic version control tells you what it means.

## Getting Started

1. Start with [git-mind link](gitmind-link.md) - create your first connection
2. Use [git-mind list](gitmind-list.md) - see what you've built
3. Install [evolution tracking](gitmind-install-hooks.md) - let connections follow changes
4. When things grow, [rebuild the cache](gitmind-cache-rebuild.md) - keep it fast

## Remember

*"The past can hurt. But the way I see it, you can either run from it or learn from it."*

Every command in git-mind helps you learn from your code's past and build a better future.

---

*Welcome to the Pride Lands of semantic version control.*


# gitmind-cache-rebuild.md
<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->
<!-- © 2025 J. Kirby Ross / Neuroglyph Collective -->

# git-mind cache-rebuild

> *"Look at the stars. The great kings of the past look down on us from those stars... So whenever you feel alone, just remember that those kings will always be there to guide you, and so will I."* - Mufasa

Table of Contents
- [Name](#name)
- [Synopsis](#synopsis)
- [Description](#description)
- [How It Works](#how-it-works)
- [Options](#options)

## NAME

`git-mind cache-rebuild` - Rebuild the Roaring Bitmap cache for lightning-fast queries

## SYNOPSIS

```bash
git mind cache-rebuild [--branch <branch>] [--force]
```

## DESCRIPTION

In the great Circle of Life, journal commits are born, live, and accumulate. Without the cache, every query must walk through the entire history - like crossing the Pride Lands on foot. The cache-rebuild command creates a Roaring Bitmap index that lets you soar above the savanna, seeing all connections at once.

This command rebuilds the performance cache for the current (or specified) branch, creating compressed bitmap indices that reduce query time from O(N) to O(log N). It's the difference between Mufasa climbing Pride Rock and Simba being lifted by Rafiki.

## HOW IT WORKS

### The Journal: Source of Truth
Your semantic links live as CBOR-encoded Git commits in `refs/gitmind/edges/<branch>`. This is the permanent record - like the stars themselves.

### The Cache: Speed of Light
The cache creates two Roaring Bitmap indices:
- **Forward index**: source SHA → edge IDs (who depends on this?)
- **Reverse index**: target SHA → edge IDs (what does this depend on?)

These are stored as Git tree objects under `refs/gitmind/cache/<branch>`, sharded into 256 buckets using the first byte of each SHA.

### The Magic: Roaring Bitmaps
Roaring Bitmaps compress sequences of integers with supernatural efficiency. A million edges might compress to just kilobytes. It's the same technology that powers modern databases and search engines.

## OPTIONS

`--branch <branch>`
: Rebuild cache for specified branch (default: current branch)

`--force`
: Force full rebuild even if cache appears current

`--verbose`
: Show detailed progress during rebuild

## EXAMPLES

### Basic rebuild for current branch
```bash
$ git mind cache-rebuild
Rebuilding cache for branch 'main'...
Processed 1,534 edges from 342 commits
Cache size: 156KB (compressed from 2.1MB)
Cache rebuilt successfully!
```

### Rebuild after heavy development
```bash
$ git mind link src/parser.c docs/parsing.md --type implements
$ git mind link src/lexer.c src/parser.c --type feeds
$ git mind cache-rebuild
Rebuilding cache for branch 'feature/parser'...
Processed 2 new edges from 2 commits
Cache updated incrementally!
```

### Force rebuild when things seem wrong
```bash
$ git mind cache-rebuild --force --verbose
Force rebuilding cache for branch 'main'...
Walking journal from refs/gitmind/edges/main
[00] Processing edge: design.md -> src/main.c (implements)
[01] Processing edge: README.md -> docs/api.md (references)
...
Building forward index bitmaps...
Building reverse index bitmaps...
Creating Git tree objects...
Cache rebuilt: refs/gitmind/cache/main -> 3a7f9b2
```

## PERFORMANCE

Without cache (journal scan):
- 1,000 edges: ~50ms
- 10,000 edges: ~500ms  
- 100,000 edges: ~5000ms

With cache (bitmap query):
- 1,000 edges: ~2ms
- 10,000 edges: ~5ms
- 100,000 edges: ~10ms

*"It's the Circle of Life, and it moves us all!"* The cache makes git-mind scale to repositories with millions of semantic connections.

## WHEN TO REBUILD

The cache automatically detects when it's stale by comparing the journal tip with the cached tip. However, you should manually rebuild when:

1. **After bulk operations** - Added many links at once
2. **After merging branches** - Cache is branch-specific
3. **Performance degrades** - Queries feel slow
4. **Something seems wrong** - Use `--force` to ensure consistency

## TECHNICAL DETAILS

### Storage Format
```
refs/gitmind/cache/main
└── tree
    ├── meta.json           # Cache metadata
    ├── 00/                 # SHA prefix shard
    │   ├── 00a1b2c3.forward
    │   └── 00a1b2c3.reverse
    ├── 01/
    │   └── 01d4e5f6.forward
    ...
    └── ff/
        └── ffa9b8c7.reverse
```

### Bitmap Serialization
Each bitmap file contains:
1. 16-byte header with magic number and version
2. Compressed Roaring Bitmap data
3. CRC32 checksum

### Incremental Updates
The cache tracks the last processed journal commit. On rebuild, it only processes new commits, making updates extremely fast.

## DIAGNOSTICS

The cache-rebuild command returns 0 on success, non-zero on failure.

Common issues:
- **"Permission denied"**: Ensure you have write access to `.git/`
- **"No journal found"**: Create some links first with `git mind link`
- **"Invalid journal format"**: Corrupted journal - seek help

## SEE ALSO

- `git-mind-link(1)` - Create semantic links
- `git-mind-list(1)` - Query semantic links  
- `git-mind(1)` - Main command overview

## PHILOSOPHY

*"Remember who you are."*

The cache is not the truth - it's a performance optimization. The journal commits are the permanent record. If the cache is ever lost or corrupted, it can always be rebuilt from the journal.

This design ensures that git-mind scales from personal note-taking to massive codebases with millions of connections, all while maintaining Git's promise: your data is yours, forever.

---

*In the great Circle of Life, every query begins with a search, and every search begins with hope. The cache ensures that hope is never disappointed.*


# gitmind-install-hooks.md
<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->
<!-- © 2025 J. Kirby Ross / Neuroglyph Collective -->

# git-mind install-hooks

> *"The past can hurt. But the way I see it, you can either run from it or learn from it."* - Rafiki

Table of Contents
- [Name](#name)
- [Synopsis](#synopsis)
- [Description](#description)
- [The Circle of Change](#the-circle-of-change)
- [Options](#options)

## NAME

`git-mind install-hooks` - Install Git hooks that automatically track file evolution

## SYNOPSIS

```bash
git mind install-hooks [--force] [--no-backup]
```

## DESCRIPTION

Change happens. Files evolve. The old gives way to the new. But what if every change could be remembered? What if your code's evolution was as traceable as Simba's journey from cub to king?

The install-hooks command sets up a post-commit hook that automatically creates AUGMENTS edges whenever you modify files that have existing semantic links. It's like having Rafiki watching over your shoulder, documenting every moment of growth.

## THE CIRCLE OF CHANGE

When you edit a file that has semantic links, those links point to a specific version (blob SHA). After your edit, the old links become outdated. The AUGMENTS system preserves the connection by creating evolution edges:

```
Before edit:  design.md ──implements──> main.c (SHA: abc123)
After edit:   design.md ──implements──> main.c (SHA: def456)
              main.c:abc123 ──AUGMENTS──> main.c:def456
```

Now when you query connections to main.c, git-mind knows to follow the AUGMENTS chain to the current version.

## OPTIONS

`--force`
: Overwrite existing hooks without asking

`--no-backup`
: Don't create backup of existing hooks

`--verbose`
: Show detailed installation progress

## EXAMPLES

### First time installation
```bash
$ git mind install-hooks
Installing git-mind hooks...
✓ Created .git/hooks/post-commit
✓ Hook installed successfully!

Your commits will now track file evolution automatically.
```

### With existing hooks
```bash
$ git mind install-hooks
Found existing post-commit hook
Backing up to .git/hooks/post-commit.backup.1736637890
✓ Existing hook backed up
✓ New hook installed with existing hook preservation

Both your existing hook and git-mind will run after commits.
```

### Force installation
```bash
$ git mind install-hooks --force --no-backup
Installing git-mind hooks...
✓ Overwrote existing .git/hooks/post-commit
✓ Hook installed successfully!
```

## HOW IT WORKS

### The Hook Script
The installed hook is a simple shell script that calls the git-mind-hook binary:

```bash
#!/bin/sh
# git-mind post-commit hook
# Automatically tracks file evolution

# Run existing hook if present
if [ -f .git/hooks/post-commit.backup.* ]; then
    .git/hooks/post-commit.backup.* || true
fi

# Run git-mind augments tracker
.git/hooks/git-mind-hook post-commit
```

### The Magic Moment
After every commit, the hook:
1. Gets the list of modified files
2. For each file, finds its old blob SHA (before commit)
3. Searches recent edges (last 200) for links to that blob
4. Creates AUGMENTS edges from old blob to new blob
5. Updates the semantic graph silently in the background

### Performance
The hook is blazing fast:
- Typical commit: <10ms overhead
- Large commit (50+ files): <50ms overhead
- Massive refactor (500+ files): <200ms overhead

It runs asynchronously and never blocks your workflow.

## WHEN AUGMENTS HAPPEN

AUGMENTS edges are created when:
- You modify a file that has incoming semantic links
- You rename a file (old path AUGMENTS to new path)
- You split a file (original AUGMENTS to both pieces)

AUGMENTS edges are NOT created for:
- New files (no history to track)
- Deleted files (use tombstone edges instead)
- Merge commits (too complex for v1)

## QUERYING EVOLUTION

Once installed, the `git mind list` command automatically follows AUGMENTS chains:

```bash
# See current connections (follows AUGMENTS)
$ git mind list

# See full evolution history
$ git mind list --show-augments

# See connections at specific point in time
$ git mind list --at HEAD~10
```

## UNINSTALLING

To remove the hooks:

```bash
# Restore original hook if backed up
$ mv .git/hooks/post-commit.backup.* .git/hooks/post-commit

# Or simply remove
$ rm .git/hooks/post-commit
```

Your semantic links remain intact - only automatic tracking stops.

## PHILOSOPHY

*"Oh yes, the past can hurt. But you can either run from it or learn from it."*

Code evolution is natural. Files grow, split, merge, and transform. The AUGMENTS system doesn't fight this change - it embraces it. Every commit becomes a checkpoint in your code's journey.

Traditional version control tracks what changed. Semantic version control tracks why it changed and what it means.

## TECHNICAL DETAILS

### Edge Format
AUGMENTS edges have a special format:
```
{
  src_sha: "abc123...",  // Old blob SHA
  tgt_sha: "def456...",  // New blob SHA  
  rel_type: "AUGMENTS",
  confidence: 1.0,
  src_path: "src/old.c", // For readability
  tgt_path: "src/new.c"  // May differ if renamed
}
```

### Search Window
The hook searches the last 200 edges for performance. This covers several days of typical development. Older links won't auto-update but can be manually linked.

### Conflict Resolution
AUGMENTS edges never conflict because they:
- Use ULID-based edge IDs (time-ordered, unique)
- Are append-only (no modifications)
- Are branch-specific (isolated evolution)

## SEE ALSO

- `git-mind-link(1)` - Create semantic links manually
- `git-mind-list(1)` - Query links with AUGMENTS support
- `git-mind(1)` - Main command overview
- `githooks(5)` - Git hooks documentation

## THE LESSON

*"Change is good."* - Rafiki

But change without memory is chaos. The AUGMENTS system ensures that every transformation is recorded, every evolution is traceable, and every connection persists through change.

Install the hooks. Let your code's journey document itself.

---

*Asante sana, squash banana! Your semantic links now travel through time!* 🐒


# gitmind-link.md
<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->
<!-- © 2025 J. Kirby Ross / Neuroglyph Collective -->

# git-mind link

> *"Everything the light touches is our kingdom... But a king's time as ruler rises and falls like the sun."* - Mufasa

Table of Contents
- [Name](#name)
- [Synopsis](#synopsis)
- [Description](#description)
- [The Heart of Git-Mind](#the-heart-of-git-mind)
- [Options](#options)

## NAME

`git-mind link` - Create semantic connections between files

## SYNOPSIS

```bash
git mind link <source> <target> --type <relationship> [options]
```

## DESCRIPTION

In the vast savanna of your codebase, no file stands alone. Each is connected to others through invisible threads of meaning - a test file validates its implementation, documentation explains its design, configuration guides its behavior.

The `git mind link` command makes these connections visible and permanent. It creates semantic edges that travel through time with your code, surviving refactors, renames, and the endless circle of development.

## THE HEART OF GIT-MIND

This is the command that started it all. When you create a link, you're not just adding metadata - you're encoding understanding into the fabric of your repository. These links become part of your Git history, as permanent as the code itself.

```bash
# The moment of connection
$ git mind link design.md src/parser.c --type implements
Created edge: design.md ──implements──> src/parser.c
```

From this moment forward, anyone who checks out your code can discover this relationship. More importantly, they can travel back in time and see what you understood at any point in history.

## OPTIONS

`--type <relationship>`
: **Required.** The type of semantic relationship. Common types:
  - `implements` - Source implements design/spec in target
  - `references` - Source references or mentions target
  - `depends_on` - Source requires target to function
  - `tests` - Source tests the functionality in target
  - `documents` - Source documents how target works
  - `inspires` - Source inspired the creation of target
  - `refactors` - Source is refactored version of target
  - *...or any relationship that captures your understanding*

`--confidence <float>`
: Confidence level from 0.0 to 1.0 (default: 1.0)
  - `1.0` - Certain (default)
  - `0.8` - Highly confident
  - `0.5` - Somewhat confident
  - `0.2` - Speculative

`--note <message>`
: Add a note explaining the connection (stored in commit message)

`--bidirectional`
: Create edges in both directions

`--quiet`
: Suppress success message

## EXAMPLES

### Basic usage - The daily workflow
```bash
# Implementation tracks design
$ git mind link src/auth.c docs/auth-design.md --type implements

# Test validates implementation  
$ git mind link tests/test_auth.c src/auth.c --type tests

# Config guides behavior
$ git mind link src/auth.c config/auth.yaml --type depends_on
```

### With confidence - Encoding uncertainty
```bash
# Human creating a confident link (confidence defaults to 1.0)
$ git mind link src/parser.c rfc/grammar.txt --type implements

# AI suggesting a relationship with confidence score
$ export GIT_MIND_SOURCE=claude
$ export GIT_MIND_AUTHOR=claude@anthropic
$ git mind link src/parser.c rfc/grammar.txt --type implements --confidence 0.85
Created link: src/parser.c ──implements──> rfc/grammar.txt [claude: claude@anthropic, conf: 0.85]

# High-confidence AI relationship
$ git mind link src/algorithm.c papers/dijkstra62.pdf --type references --confidence 0.95
```

### Bidirectional links - Symmetric relationships
```bash
# These files are peers that reference each other
$ git mind link src/client.c src/server.c --type references --bidirectional
Created edge: src/client.c ──references──> src/server.c
Created edge: src/server.c ──references──> src/client.c
```

### Human-AI Collaboration - Attribution System
```bash
# Human creates edge (default behavior)
$ git mind link README.md src/main.c --type documents
Created link: README.md ──documents──> src/main.c

# AI creates edge with attribution
$ export GIT_MIND_SOURCE=claude
$ export GIT_MIND_AUTHOR=claude@anthropic  
$ export GIT_MIND_SESSION=analysis_2025
$ git mind link src/auth.c config/oauth.json --type depends_on --confidence 0.82
Created link: src/auth.c ──depends_on──> config/oauth.json [claude: claude@anthropic, conf: 0.82]

# System-generated edge (from hooks)
$ export GIT_MIND_SOURCE=system
$ git mind link old_file.c new_file.c --type augments
```

## HOW IT WORKS

### Git-Native Storage
Each link is stored as a Git commit in `refs/gitmind/edges/<branch>`. The commit contains:
- CBOR-encoded edge data in the commit message
- Empty tree (git's null tree: 4b825dc...)
- Author/committer from your Git config
- Timestamp of creation

### Content Addressing
Links connect specific versions of files using their blob SHAs:
```
source_sha: 8a9f3b2c...  # SHA of design.md's content
target_sha: 7c4d5e6f...  # SHA of parser.c's content
```

This means links point to exact versions. When files change, the AUGMENTS system (see `git-mind-install-hooks(1)`) tracks their evolution.

### Branch Isolation
Links are branch-specific. Create different connections on different branches:
```bash
$ git checkout -b feature/auth
$ git mind link src/auth.c new-auth-api.md --type implements

$ git checkout main  
$ git mind list  # Won't show the feature branch link
```

## RELATIONSHIP TYPES

While you can use any string as a relationship type, these conventions help maintain consistency:

### Structural Relations
- `implements` - Code implements a design/specification
- `depends_on` - Requires target to function correctly
- `extends` - Builds upon target's functionality
- `replaces` - Newer version replacing target

### Documentation Relations
- `documents` - Explains how target works
- `references` - Mentions or links to target
- `examples` - Shows usage of target
- `tutorials` - Teaches concepts in target

### Development Relations
- `tests` - Validates target's behavior
- `benchmarks` - Measures target's performance
- `fixes` - Resolves issues in target
- `refactors` - Restructured version of target

### Inspiration Relations
- `inspires` - Target inspired this implementation
- `adapts` - Modified version of target's approach
- `conflicts` - Incompatible with target
- `discusses` - Analyzes or critiques target

## BEST PRACTICES

### Be Specific
```bash
# Too vague
$ git mind link src/parser.c docs/ --type references  # Which doc?

# Better
$ git mind link src/parser.c docs/parser-design.md --type implements
```

### Use Conventional Types
Stick to common relationship types when possible. This makes queries more powerful:
```bash
# Find all tests easily
$ git mind list --type tests
```

### Link at the Right Granularity
- Link files, not directories (unless the directory is the unit)
- Link to specific documents, not entire folders
- Link the most specific relevant files

### Document Non-Obvious Links
```bash
# Why does the parser depend on the logger config?
$ git mind link src/parser.c config/logger.yaml --type depends_on \
    --note "Parser uses logger for syntax error reporting"
```

## PERMANENCE & EVOLUTION

*"Remember who you are."*

Every link you create becomes part of your repository's permanent history. Future developers can:
- See what you connected and why
- Travel back to see historical connections
- Understand the evolution of architectural decisions

This is why git-mind exists: to encode not just what your code is, but what it means and how it relates to everything else.

## ERROR HANDLING

Common errors and solutions:

**"File not found"**
- Ensure both source and target exist in the repository
- Use paths relative to repository root
- Check for typos in filenames

**"Not in a git repository"**
- Run from within a Git repository
- Initialize with `git init` if needed

**"Invalid confidence value"**
- Use a float between 0.0 and 1.0
- Default is 1.0 if not specified

## SEE ALSO

- `git-mind-list(1)` - Query semantic links
- `git-mind-traverse(1)` - Explore connection graphs
- `git-mind-install-hooks(1)` - Track file evolution
- `git-mind(1)` - Main command overview

## THE PHILOSOPHY

*"There's more to being a king than getting your way."*

Creating links is an act of documentation, but more than that - it's an act of teaching. Every link you create teaches future readers (including future you) about the relationships that matter.

The power isn't in the tool. It's in the understanding you encode with it.

---

*"You are more than what you have become. Remember."* - Your semantic links remember for you.


# gitmind-list.md
<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->
<!-- © 2025 J. Kirby Ross / Neuroglyph Collective -->

# git-mind list

> *"Look beyond what you see."* - Rafiki

Table of Contents
- [Name](#name)
- [Synopsis](#synopsis)
- [Description](#description)
- [The Power of Seeing](#the-power-of-seeing)

## NAME

`git-mind list` - Query and explore semantic connections

## SYNOPSIS

```bash
git mind list [options]
```

## DESCRIPTION

In the Pride Lands of your codebase, every file has connections - some obvious, some hidden, some forgotten. The `git mind list` command reveals these relationships, showing you the semantic web that binds your code together.

This is your window into understanding. Use it to discover dependencies, trace implementations, find tests, and see how knowledge flows through your repository.

## THE POWER OF SEEING

Without git-mind:
```bash
# Where is this implemented?
$ grep -r "auth flow" docs/
# What tests cover this?
$ find . -name "*test*" | xargs grep -l "parser"
# What depends on this config?
$ # ...good luck
```

With git-mind:
```bash
# All connections at a glance
$ git mind list --from src/parser.c

# Specific relationships
$ git mind list --type tests --to src/auth.c

# Everything in the web
$ git mind list
```

## OPTIONS

### Filtering Options

`--from <path>`
: Show only edges originating from this file

`--to <path>`
: Show only edges pointing to this file

`--type <relationship>`
: Filter by relationship type (implements, tests, etc.)

`--min-confidence <min>`
: Show only edges with confidence >= threshold (0.0-1.0)

### Display Options

`--format <format>`
: Output format (default: human)
  - `human` - Readable format with arrows
  - `json` - Machine-parseable JSON
  - `tsv` - Tab-separated values
  - `dot` - Graphviz DOT format

`--show-sha`
: Display blob SHAs (default: hide)

`--show-augments`
: Include AUGMENTS evolution edges

`--show-attribution`
: Display attribution information for all edges

### Time Travel Options

`--at <commit>`
: Show connections as they existed at specific commit

`--branch <branch>`
: Query different branch (default: current)

### Performance Options

`--no-cache`
: Force journal scan (bypass cache)

`--limit <n>`
: Limit results (default: unlimited)

## EXAMPLES

### Basic Queries

Show all connections:
```bash
$ git mind list
README.md ──references──> docs/quickstart.md
src/main.c ──implements──> docs/design.md
tests/test_parser.c ──tests──> src/parser.c
src/parser.c ──depends_on──> config/grammar.yaml
... (42 more edges)
```

### Focused Queries

What does this file connect to?
```bash
$ git mind list --from src/auth.c
src/auth.c ──implements──> docs/auth-design.md
src/auth.c ──depends_on──> config/auth.yaml
src/auth.c ──references──> lib/crypto.c
```

What connects to this file?
```bash
$ git mind list --to src/parser.c
tests/test_parser.c ──tests──> src/parser.c
tests/bench_parser.c ──benchmarks──> src/parser.c
docs/parser-guide.md ──documents──> src/parser.c
src/compiler.c ──depends_on──> src/parser.c
```

### Filtered Queries

Find all test files:
```bash
$ git mind list --type tests
tests/test_auth.c ──tests──> src/auth.c
tests/test_parser.c ──tests──> src/parser.c
tests/integration/test_flow.c ──tests──> src/flow.c
```

Find uncertain connections:
```bash
$ git mind list --min-confidence 0.8
src/auth.c ──depends_on──> config/oauth.json [claude: claude@anthropic, conf: 0.85]
src/parser.c ──implements──> specs/grammar.md [gpt: gpt4@openai, conf: 0.92]
```

### Attribution Filtering

Show only human-created edges:
```bash
$ git mind list --source human
README.md ──documents──> src/main.c
src/auth.c ──implements──> docs/auth-design.md
tests/test_auth.c ──tests──> src/auth.c
```

Show only AI-suggested edges:
```bash
$ git mind list --source ai
src/parser.c ──depends_on──> lib/lexer.h [claude: claude@anthropic, conf: 0.78]
src/crypto.c ──references──> papers/aes.pdf [gpt: gpt4@openai, conf: 0.91]
```

Show attribution for all edges:
```bash
$ git mind list --show-attribution
README.md ──documents──> src/main.c [human: user@example.com]
src/auth.c ──depends_on──> config/oauth.json [claude: claude@anthropic, conf: 0.85]
src/test.c ──tests──> src/main.c [human: developer@team.com]
```

High-confidence AI insights only:
```bash
$ git mind list --source ai --min-confidence 0.9
src/crypto.c ──references──> papers/aes.pdf [gpt: gpt4@openai, conf: 0.91]
src/algorithm.c ──implements──> papers/dijkstra.pdf [claude: claude@anthropic, conf: 0.95]
```

### Time Travel Queries

See historical connections:
```bash
# What did we understand 50 commits ago?
$ git mind list --at HEAD~50

# What connections existed at release?
$ git mind list --at v1.0.0

# Show connections on feature branch
$ git mind list --branch feature/new-auth
```

### Evolution Tracking

See how files evolved:
```bash
$ git mind list --show-augments
src/auth.c:8a9f3b ──AUGMENTS──> src/auth.c:7c4d5e
src/auth.c:7c4d5e ──AUGMENTS──> src/auth.c:current
docs/api.md:3f2a1b ──AUGMENTS──> docs/api.md:current
```

### Machine-Readable Output

JSON for tooling:
```bash
$ git mind list --format json
{
  "edges": [
    {
      "source": "README.md",
      "target": "docs/quickstart.md",
      "type": "references",
      "confidence": 1.0,
      "timestamp": 1736637890000
    }
  ]
}
```

DOT for visualization:
```bash
$ git mind list --format dot > graph.dot
$ dot -Tpng graph.dot -o graph.png
```

## UNDERSTANDING THE OUTPUT

### Human Format (default)
```
<source> ──<type>──> <target> [metadata]
```

Metadata may include:
- `(confidence: 0.8)` - Less than certain connections
- `(SHA: abc123)` - When --show-sha is used
- `(AUGMENTED)` - When file has evolved

### Following AUGMENTS

By default, list follows AUGMENTS chains to show current connections:

```bash
# Original link: design.md -> old-parser.c
# After refactor: old-parser.c AUGMENTS new-parser.c
# List shows: design.md -> new-parser.c
```

Use `--no-follow-augments` to see historical links.

## PERFORMANCE

The list command uses the Roaring Bitmap cache for O(log N) queries:

- **With cache**: <10ms for 100K edges
- **Without cache**: ~5 seconds for 100K edges
- **Cache miss**: Automatically falls back to journal scan
- **Stale cache**: Detects and warns (run `git mind cache-rebuild`)

## ADVANCED PATTERNS

### Dependency Analysis
```bash
# What does my module depend on?
$ git mind list --from "src/mymodule/**" --type depends_on

# What depends on this config?
$ git mind list --to config/database.yaml --type depends_on
```

### Test Coverage Discovery
```bash
# What files lack tests?
$ comm -23 \
    <(git ls-files src/ | sort) \
    <(git mind list --type tests --format tsv | cut -f2 | sort)
```

### Architecture Visualization
```bash
# Generate architecture diagram
$ git mind list --format dot --type implements > arch.dot
$ dot -Tsvg arch.dot -o architecture.svg
```

### Change Impact Analysis
```bash
# What might break if I change this?
$ git mind list --to src/core-api.h --type depends_on
```

## PHILOSOPHY

*"The question is: Who are you?"* - Rafiki

Your code is more than files and functions. It's a living system of relationships, dependencies, and meanings. The `list` command doesn't just show you connections - it reveals the hidden structure of your understanding.

Every query is a question about your code's nature. Every result is an answer that teaches.

## DIAGNOSTICS

Returns 0 on success, non-zero on failure.

Common warnings:
- `Cache stale, using journal scan` - Run `git mind cache-rebuild`
- `No edges found` - No links match your query
- `Following AUGMENTS chain` - File has evolved (use --no-follow-augments for original)

## SEE ALSO

- `git-mind-link(1)` - Create semantic connections
- `git-mind-traverse(1)` - Explore connection graphs
- `git-mind-cache-rebuild(1)` - Optimize query performance
- `git-mind(1)` - Main command overview

## THE LESSON

*"Oh, the past can hurt. But the way I see it, you can either run from it or learn from it."*

Every list query teaches you something about your codebase. The more you look, the more you see. The more you see, the more you understand.

---

*"It is time."* - Time to see what connects your kingdom.


# gitmind.md
<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->
<!-- © 2025 J. Kirby Ross / Neuroglyph Collective -->

# git-mind

> *"Remember who you are."* - Mufasa

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

*"Everything the light touches has been connected by git-mind."*

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
: Follow connection paths from starting point *(coming soon)*

`unlink <source> <target>`
: Remove connection (adds tombstone) *(coming soon)*

### Management Commands

`cache-rebuild [--branch <branch>]`
: Rebuild Roaring Bitmap cache for fast queries

`install-hooks`
: Install Git hooks for automatic evolution tracking

`status`
: Show semantic graph statistics *(coming soon)*

`gc [--aggressive]`
: Compress old journal entries *(coming soon)*

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

### Why Journal Commits?

*"It's the Circle of Life, and it moves us all"*

Traditional metadata systems fight Git's nature. They store data outside the history, create merge conflicts, and lose synchronization. git-mind embraces Git's design:

- **Commits are immutable** - Your connections can't be lost
- **Branches isolate changes** - Different understanding on different branches
- **History is permanent** - Travel through time to see past understanding
- **Merging is managed** - Git handles conflicts naturally

### Why Semantic Connections?

*"There's more to being king than getting your way"*

Code without context is just syntax. Understanding comes from relationships:
- Why was this built?
- What does it implement?
- What depends on it?
- How has it evolved?

git-mind makes these relationships first-class citizens of your repository.

### Why Performance Matters?

*"Life's not fair, is it?"*

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

*(Coming soon)* Plugins for:
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
*"Look inside yourself. You are more than what you have become."*

Your understanding is valuable. git-mind preserves it, letting you:
- Remember why you made decisions
- Navigate by meaning, not just files
- Build on your past insights

### For Teams
*"We are one."*

Shared understanding is your competitive advantage:
- Onboard faster with visible relationships
- Preserve knowledge through transitions
- Build collective intelligence

### For the Future
*"It is time."*

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

*"Hakuna Matata"* - No worries, your semantic connections are safe.

## THE PROMISE

*"Remember who you are."*

Your code tells a story. git-mind helps you remember it, share it, and build upon it. In the great Circle of Development, no understanding is lost, no connection forgotten, no wisdom wasted.

Welcome to semantic version control. Welcome to git-mind.

---

*"Oh yes, the past can hurt. But the way I see it, you can either run from it or learn from it."*

**Choose to learn. Choose git-mind.**


