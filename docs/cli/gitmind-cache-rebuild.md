<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->
<!-- © 2025 J. Kirby Ross / Neuroglyph Collective -->

# git-mind cache-rebuild

> *"Look at the stars. The great kings of the past look down on us from those stars... So whenever you feel alone, just remember that those kings will always be there to guide you, and so will I."* - Mufasa

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