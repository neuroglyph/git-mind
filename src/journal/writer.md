# Journal Writer

## Purpose
Append edges to a branch-specific journal using Git commits with CBOR-encoded messages.

## Design Rationale

### Why Commits Instead of Objects?
1. **Branch isolation**: Each branch has its own journal ref
2. **History**: Natural ordering via parent chain
3. **Signatures**: Can sign journal commits
4. **Replication**: `git push/pull` just works
5. **No orphan refs**: GitHub/GitLab accept refs/gitmind/*

### Empty Tree Strategy
```c
#define EMPTY_TREE_SHA "4b825dc642cb6eb9a060e54bf8d69288fbee4904"
```
- Git's well-known empty tree (exists in every repo)
- Zero storage overhead (tree already exists)
- Commits become ~200 bytes + message

### Reference Naming
```
refs/gitmind/edges/main
refs/gitmind/edges/feature-x
refs/gitmind/edges/hotfix/CVE-2025-1234
```

```mermaid
gitGraph
    commit id: "main"
    branch refs/gitmind/edges/main
    commit id: "edge: A→B"
    commit id: "edge: B→C"
    
    checkout main
    branch feature-x
    commit id: "add feature"
    
    branch refs/gitmind/edges/feature-x
    commit id: "edge: X→Y"
    
    checkout main
    merge feature-x
    
    checkout refs/gitmind/edges/main
    merge refs/gitmind/edges/feature-x
```

- Mirrors branch structure
- Natural merge semantics
- Survives rebases (re-parented automatically)

## Implementation Details

### Batch Committing
- Accumulate edges up to 8KB
- Create commit when buffer full
- Single edges create immediate commits
- Balances between commit spam and latency

### Parent Handling
```c
if (ref_exists) {
    parent = current_tip;
} else {
    parent = NULL;  // First commit on branch
}
```
- Gracefully handles new branches
- No special initialization needed

### Signature Reuse
- Uses `git_signature_default()` 
- Respects user.name/user.email config
- Author = Committer (it's a machine operation)

## Edge Cases

### Branch Detection
- **Detached HEAD**: Fails with GM_ERROR
- **Empty repo**: No HEAD, cannot determine branch
- **Worktrees**: Each has own HEAD, works correctly

### Concurrent Writers
- **Same process**: Not thread-safe (by design)
- **Multiple processes**: Git handles ref locking
- **Result**: Last writer wins (standard Git behavior)

### Large Edge Batches
```
User: Create 10,000 edges
Result: ~625 commits (16 edges per commit)
Time: ~2 seconds on SSD
```
- Automatic batching prevents huge commits
- GitHub render limit: 1MB commit messages

### Unicode Branch Names
- Git already restricts ref names
- We inherit Git's validation
- No additional sanitization needed

## Error Handling

1. **No repo**: GM_ERROR (caught early)
2. **No memory**: GM_NO_MEMORY (malloc failed)
3. **Git errors**: Wrapped as GM_ERROR
4. **Disk full**: Git returns error, we propagate

## Performance Characteristics

### Write Amplification
```
1 edge = 1 commit = ~300 bytes
1000 edges in batch = 1 commit = ~100KB
Amplification factor: 300x vs 0.1x
```
- Batch operations strongly preferred
- Single edges acceptable for interactive use

### Bottlenecks
1. **Commit creation**: ~200μs (SHA-1 hashing)
2. **Ref update**: ~100μs (file I/O)
3. **Not bottlenecks**: CBOR encoding, memory

## Security Considerations

### Ref Name Injection
- Branch names come from Git (already validated)
- We only append to refs/gitmind/edges/
- No user input in ref names

### Binary Commit Messages
- Set `encoding=binary` 
- Git treats as opaque bytes
- Web UIs show "Binary content"
- No XSS risk

## Future Evolution

### Compression
```c
if (cbor_size > 1024) {
    zstd_compress(cbor_data);
    prepend_magic_byte(0xFF);
}
```
- Optional for large batches
- Transparent to readers

### Parallelism
- Could write to refs/gitmind/edges/main.lock
- Merge parallel journals periodically
- Complexity not worth it yet

## Testing Strategy

### Critical Tests
1. First edge on new branch
2. Batch overflow at 8KB boundary
3. Unicode in branch names
4. Concurrent writers (integration)
5. Power loss simulation

### Performance Tests
```bash
# Measure commit creation overhead
time git-mind link-bulk < 10k-edges.txt

# Should be < 5 seconds for 10K edges
```

## Why This Design Wins

1. **Git-native**: Uses commits, not custom objects
2. **Branch-aware**: Isolated by design
3. **Push-friendly**: Standard refs, no magic
4. **Recoverable**: History preserves everything
5. **Simple**: ~200 lines of clear logic

As Linus would say: "Good taste means using the infrastructure that's already there."