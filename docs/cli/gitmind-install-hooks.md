---
title: git-mind install-hooks
description: Install Git hooks to automatically capture certain semantic edges.
audience: [users, developers]
domain: [cli]
tags: [cli, hooks]
status: stable
last_updated: 2025-09-15
---

<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->
<!-- © 2025 J. Kirby Ross / Neuroglyph Collective -->

# git-mind install-hooks

> _"The past can hurt. But the way I see it, you can either run from it or learn from it."_ - Rafiki

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

When you edit a file that has semantic links, those links point to a specific version (blob object ID, or OID). After your edit, the old links become outdated. The AUGMENTS system preserves the connection by creating evolution edges:

```
Before edit:  design.md ──implements──> main.c (OID: abc123)
After edit:   design.md ──implements──> main.c (OID: def456)
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
2. For each file, finds its old blob OID (before commit)
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

_"Oh yes, the past can hurt. But you can either run from it or learn from it."_

Code evolution is natural. Files grow, split, merge, and transform. The AUGMENTS system doesn't fight this change - it embraces it. Every commit becomes a checkpoint in your code's journey.

Traditional version control tracks what changed. Semantic version control tracks why it changed and what it means.

## TECHNICAL DETAILS

### Edge Format

AUGMENTS edges have a special format:

```
{
  src_oid: "abc123...",  // Old blob object ID
  tgt_oid: "def456...",  // New blob object ID  
  rel_type: "AUGMENTS",
  confidence: 1.0,
  src_path: "core/src/old.c", // For readability
  tgt_path: "core/src/new.c"  // May differ if renamed
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

_"Change is good."_ - Rafiki

But change without memory is chaos. The AUGMENTS system ensures that every transformation is recorded, every evolution is traceable, and every connection persists through change.

Install the hooks. Let your code's journey document itself.

---

_Asante sana, squash banana! Your semantic links now travel through time!_ 🐒
