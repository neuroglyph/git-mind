# TASKLIST: Project No Tux Given 🐧⚡

**Mission**: Rebuild git-mind using the Ultimate Design - edge journal commits with branch-aware graphs

**Status**: BLANK SLATE REBUILD - Day 3 pivot to the correct architecture

---

## Phase 0: Archive & Prepare (30 min)
- [ ] Tag current implementation: `git tag archive/holy-grail-v0`
- [ ] Create new branch: `git checkout -b no-tux-given`
- [ ] Clear src/ directory (keep docs for reference)
- [ ] Update README.md with "Under reconstruction" notice
- [ ] Read these docs in order:
  - [ ] `/docs/reports/ideas.md` - The Ultimate Design
  - [ ] `/docs/reports/blank-slate-c.md` - Implementation approach
  - [ ] `/docs/architecture/no-tux-given-plan.md` - Battle plan

---

## Phase 1: Core Journal (Weekend 1)

### Setup (2 hours)
- [ ] Create minimal directory structure:
  ```
  src/
  ├── core/
  │   ├── journal.c
  │   ├── edge.c
  │   └── cbor.c
  ├── cli/
  │   └── main.c
  └── util/
      └── common.h
  ```
- [ ] Write Makefile with libgit2 linking
- [ ] Create `gm_edge_t` struct with blob SHAs + paths
- [ ] Implement CBOR encoder for edges (< 150 lines)

### Journal Writer (3 hours)
- [ ] Implement `gm_journal_init()` - creates refs/gitmind/edges/<branch>
- [ ] Implement `gm_journal_append()`:
  - [ ] Get current branch name from HEAD
  - [ ] Create empty tree object (static OID)
  - [ ] Encode edge(s) to CBOR
  - [ ] Create commit with CBOR as message
  - [ ] Update refs/gitmind/edges/<branch>
- [ ] Test: Create 10 edges, verify with `git log`

### Journal Reader (3 hours)
- [ ] Implement `gm_journal_walk()` - iterate commits
- [ ] Implement `gm_journal_read_edges()` - decode CBOR from commit
- [ ] Test: Read back the 10 edges created above

### Basic CLI (2 hours)
- [ ] `git-mind init` - creates journal ref for current branch
- [ ] `git-mind link <src> <tgt> --type <rel>` - creates edge commit
- [ ] `git-mind list` - walks journal, prints edges (slow version)
- [ ] Test on real repository with ~20 links

---

## Phase 2: Git Integration (Weekend 2)

### Path to SHA Resolution (2 hours)
- [ ] Implement `gm_resolve_path_to_blob()` using libgit2
- [ ] Handle non-existent files gracefully
- [ ] Store both SHA and path in edge for human readability

### Branch Awareness (3 hours)
- [ ] Implement `gm_get_current_branch()` 
- [ ] Update all journal operations to use branch-specific refs
- [ ] Test: Create different links on main vs feature branch
- [ ] Test: Switch branches, verify different links shown

### AUGMENTS System (3 hours)
- [ ] Design AUGMENTS edge type for edits
- [ ] Create post-commit hook that:
  - [ ] Detects changed files via `git diff`
  - [ ] Looks up old blob SHA in recent edges
  - [ ] Creates AUGMENTS edge: old_blob -> new_blob
- [ ] Update `list` to follow AUGMENTS chains

### Push/Pull Testing (2 hours)
- [ ] Test: `git push origin main refs/gitmind/edges/main`
- [ ] Verify GitHub accepts the ref
- [ ] Test clone and pull on fresh repo
- [ ] Document any hosting-specific issues

---

## Phase 3: Performance Layer (Weekend 3)

### Roaring Bitmap Integration (3 hours)
- [ ] Add croaring library (single header)
- [ ] Design bitmap storage format
- [ ] Implement bitmap serialization/deserialization

### Cache Builder (4 hours)
- [ ] Implement `gm_cache_rebuild()`:
  - [ ] Walk journal commits since last cache
  - [ ] Build src_sha -> edge_ids bitmap
  - [ ] Build tgt_sha -> edge_ids reverse index
  - [ ] Store under refs/gitmind/cache/<branch>
- [ ] Make incremental (only process new commits)

### Fast Query Path (3 hours)
- [ ] Implement `gm_query_fanout()` using cache
- [ ] Fall back to journal scan if cache missing
- [ ] Benchmark: 100K edges, measure query time
- [ ] Update CLI to use fast path

---

## Phase 4: Polish & Ship (Bonus Weekend)

### Missing Features
- [ ] `git-mind unlink` - adds tombstone edge
- [ ] `git-mind traverse` - BFS from starting point
- [ ] `git-mind gc` - compress old journal commits
- [ ] `git-mind status` - show graph statistics

### Developer Experience  
- [ ] Installation script
- [ ] Git hooks installer
- [ ] Man pages
- [ ] Comprehensive error messages

### Documentation
- [ ] Architecture diagram with mermaid
- [ ] Tutorial: "Your first semantic graph"
- [ ] FAQ: "Why journal commits?"
- [ ] Migration guide from v0

### Testing & Release
- [ ] Integration tests for all commands
- [ ] Test on macOS, Linux, Windows (WSL)
- [ ] GitHub Actions CI/CD
- [ ] Create v0.2.0 release

---

## Critical Implementation Notes

### Journal Ref Format
```
refs/gitmind/edges/main       # for main branch
refs/gitmind/edges/feature-x  # for feature-x branch
refs/gitmind/edges/heads/main # alternative (more explicit)
```

### Empty Tree OID
```c
static const char *empty_tree = "4b825dc642cb6eb9a060e54bf8d69288fbee4904";
```

### CBOR Edge Format
```
[src_sha, tgt_sha, rel_type, confidence, timestamp, src_path, tgt_path]
```

### Commit Message
- Set encoding to "binary" if using raw CBOR
- Or use ASCII format: `[src]=abc123 [tgt]=def456 [type]=implements`

### Push Configuration
```bash
git config --add remote.origin.push refs/gitmind/edges/*:refs/gitmind/edges/*
```

---

## Success Metrics

- [ ] Can create links that persist in commits
- [ ] Links are branch-specific
- [ ] Can push/pull to GitHub without errors
- [ ] Query performance < 10ms for 100K edges
- [ ] No merge conflicts for concurrent link creation
- [ ] Total implementation < 3000 lines of C

---

## Remember

1. **Read the Ultimate Design first** - Don't code until you understand
2. **Test push early** - Verify GitHub accepts refs before building more
3. **Keep it simple** - CBOR + commits, nothing fancy
4. **Branch awareness is key** - Every operation uses current branch
5. **Cache is optional** - Get journal working first

---

*"Keep code in files, truth in commits, speed in shards."*

**LET'S BUILD THIS!** 🚀