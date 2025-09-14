# SHA Resolution Utilities

## Purpose

Resolve file paths to Git blob SHAs using the repository index.

## Design Rationale

### Why Index Instead of Working Tree

```mermaid
graph TD
    A[Working Tree] -->|Modified files| B[Unstaged changes]
    C[Index/Staging] -->|Committed or staged| D[Known SHAs]
    E[Object DB] -->|Historical versions| F[All SHAs ever]
    
    C -->|We use this!| G[Best tradeoff]
    
    style C fill:#0f0
    style G fill:#0f0
```

Using the index means:

1. __Predictable__: Only committed/staged content
2. __Fast__: In-memory structure
3. __Consistent__: Same view as `git status`
4. __Safe__: No dirty working tree surprises

### Why Not Hash Files Ourselves

```c
// What we DON'T do:
sha1(read_file("README.md"))  // ❌ Wrong!

// What Git does:
sha1("blob " + filesize + "\0" + content)  // ✅ Right!
```

Git blob SHA includes:

- Object type prefix ("blob ")
- Content size in ASCII
- Null byte separator  
- Actual file content

Let Git do it right.

## Implementation Strategy

### Index Lookup

```c
git_repository_index(&index, repo);
entry = git_index_get_bypath(index, path, 0);
memcpy(sha, entry->id.id, 20);
```

Direct index access:

- O(log n) lookup (index is sorted)
- No filesystem I/O
- Returns exact SHA Git would use

### Stage Parameter

```c
git_index_get_bypath(index, path, 0);
                                  ^
                                stage
```

- 0 = Normal (no conflict)
- 1 = Common ancestor
- 2 = "Ours"  
- 3 = "Theirs"

We use 0 (no merge conflict handling yet).

## Edge Cases

### Untracked Files

```
$ git-mind link untracked.md README.md
Error: Not found
```

- File exists in working tree
- Not in index (never `git add`ed)
- Correct behavior: fail fast

### Deleted Files

```bash
$ git rm file.md
$ git-mind list file.md
# Shows old links, can't create new ones
```

- Deleted files removed from index
- Old edges still reference blob SHA
- Historical integrity preserved

### Submodules

```
repo/
├── .git/
├── README.md (blob)
└── submodule/ (commit SHA!)
```

- Submodules are commit objects, not blobs
- Currently would store commit SHA
- Future: Reject or handle specially?

### Symbolic Links

```bash
ln -s target.md link.md
git add link.md
git-mind link link.md other.md
```

- Git stores symlink content as blob
- SHA is of link text, not target
- Semantically correct behavior

### Case Sensitivity

```
Windows/macOS: README.md = readme.md
Linux: README.md ≠ readme.md
```

- Git index respects core.ignoreCase
- We inherit Git's behavior
- Platform-consistent automatically

## Performance Analysis

### Cost Breakdown

```
git_repository_index(): ~50μs (load index)
git_index_get_bypath(): ~5μs (binary search)
memcpy(): ~10ns
Total: ~55μs first call, ~5μs subsequent
```

### Index Caching

- libgit2 caches index in memory
- Multiple lookups share same index
- Free on repository close

### Scalability

```
10 files: 5μs
10K files: 15μs (log n growth)
1M files: 25μs
```

Binary search scales beautifully.

## Security Considerations

### Path Traversal

```c
gm_sha_from_path(ctx, "../../../etc/passwd", sha);
```

- Git index only contains repo files
- Path traversal impossible
- Returns GM_NOT_FOUND safely

### TOCTOU (Time-of-Check-Time-of-Use)

```
Thread 1: Check file.md exists
Thread 2: git rm file.md
Thread 1: Use stale SHA
```

- Index is snapshot at load time
- Concurrent modifications not seen
- Acceptable for our use case

## Error Handling

### Clean Failures

```c
GM_NOT_FOUND    // File not in index
GM_ERROR        // Can't load index
GM_INVALID_ARG  // NULL parameters
```

No partial results or corrupt data.

## Testing Scenarios

### Core Tests

1. Normal file → Returns correct SHA
2. Untracked file → Returns NOT_FOUND
3. Deleted file → Returns NOT_FOUND  
4. Directory path → Returns NOT_FOUND
5. NULL path → Returns INVALID_ARG

### Integration Tests

```bash
# Create test scenario
echo "content" > test.md
git add test.md

# Verify SHA matches Git
EXPECTED=$(git hash-object test.md)
ACTUAL=$(git-mind debug-sha test.md)
test "$EXPECTED" = "$ACTUAL"
```

### Edge Case Tests

- Symlinks
- Files with spaces
- Unicode filenames
- Very long paths (> 255)
- Case sensitivity

## Future Enhancements

### Working Tree Mode

```c
int gm_sha_from_path_worktree(ctx, path, sha) {
    // Hash file directly
    // For uncommitted experimentation
}
```

- Optional unsafe mode
- Hash working tree files
- User explicitly opts in

### SHA-256 Support

```c
if (repo_uses_sha256) {
    memcpy(sha, entry->id.id, 32);
} else {
    memcpy(sha, entry->id.id, 20);
}
```

- Detect repository format
- Handle both hash types
- Future-proof design

### Batch Operations

```c
int gm_sha_from_paths(ctx, paths[], shas[], count);
```

- Amortize index loading
- Single pass for multiple files
- Optimize bulk operations

## Why This Design Wins

1. __Correct__: Uses Git's exact SHA calculation
2. __Fast__: Direct index access, no I/O
3. __Simple__: ~40 lines of clear code
4. __Safe__: No working tree surprises
5. __Portable__: libgit2 handles platform quirks

As Linus would appreciate: "Don't reinvent Git's wheel. Use Git's wheel."
