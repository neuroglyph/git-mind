# The git-mind Architecture Journey: From Holy Grail to Crossroads

**Date**: June 16, 2025  
**Authors**: Claude (D4E5F6) & James  
**Status**: At Critical Decision Point

## Table of Contents
1. [Where We Started](#where-we-started)
2. [The Problems We Discovered](#the-problems-we-discovered)
3. [Solutions We Explored](#solutions-we-explored)
4. [Where We Are Now](#where-we-are-now)
5. [The Decision Point](#the-decision-point)

## Where We Started

### The Holy Grail Architecture

We began with an elegant vision: store everything as pure Git objects, no working directory pollution.

```mermaid
graph TD
    subgraph "Holy Grail Vision"
        A[refs/gitmind/graph] -->|orphan ref| B[Root Tree]
        B --> C[Source Trees]
        C --> D[Relationship Trees]
        D --> E[Edge Blobs - CBOR]
        
        F[Working Directory] -->|never touches| X[❌]
    end
    
    style A fill:#9f9,stroke:#393,stroke-width:4px
    style E fill:#f9f,stroke:#939,stroke-width:2px
    style F fill:#fee
```

**Key Design Decisions:**
1. **Orphan ref** (`refs/gitmind/graph`) - Parallel universe for links
2. **Pure Git objects** - Trees and blobs only
3. **CBOR encoding** - Compact binary format for edges
4. **Double fan-out** - For O(1) performance at scale
5. **No working directory files** - "Pure" Git approach

### Initial Implementation

```
.git/
└── refs/
    └── gitmind/
        └── graph  # Points to tree of trees
```

Each link stored as:
```
ab/cd/abcdef.../implements/01/23/ULID.cbor
```

## The Problems We Discovered

### Problem 1: File Identity Crisis

**Discovery**: "When you edit A.md, its SHA changes!"

```mermaid
graph LR
    subgraph "The SHA Problem"
        A1[A.md v1<br/>SHA: abc123] -->|link| B[B.md]
        A2[A.md v2<br/>SHA: def456] -.->|broken link| B
        
        Q{Which A.md<br/>did we link?}
    end
    
    style A2 fill:#fcc
    style Q fill:#ffd
```

**Current Bug**: We're hashing the path string "A.md" instead of file contents!
```c
// What we do (WRONG):
hash_object("README.md", 10, "blob", out_sha);  // Hashes the string!

// What we should do:
hash_object(file_contents, file_size, "blob", out_sha);  // Hash content
```

### Additional, Philosophical Dilemma  

```
<JAMES>
 Wait, problem (1) is solved by commit hooks. When a file's SHA changes, we update the link. Right? Still thinking...
</JAMES>

⏺ You're partially right, but there's a deeper issue...

  The Hook "Solution" Has Holes

  graph TD
      subgraph "What Hooks Can Do"
          H1[Detect: git mv A.md B.md] -->|✅| U1[Update path in links]
      end

      subgraph "What Hooks CAN'T Do"
          H2[Detect: edit A.md] -->|❓| U2[Update... what exactly?]
          Q{If we store content SHA,<br/>what do we update to?}
      end

      style H1 fill:#cfc
      style H2 fill:#ffd
      style Q fill:#fee

  The Fundamental Question

  If we're truly content-addressable:

  # Link created when A.md has SHA abc123
  link:
    source_sha: abc123
    target_sha: def456

  # User edits A.md, now SHA is xyz789
  # Hook fires... then what?

  Option 1: Update to new SHA
  link:
    source_sha: xyz789  # Updated
    target_sha: def456
  But then we've lost the semantic meaning - we linked to the OLD version's ideas!

  Option 2: Keep old SHA
  link:
    source_sha: abc123  # Original
    target_sha: def456
  But now the link points to content that doesn't exist in working tree!

  Option 3: Don't use content SHA at all
  link:
    source: "A.md"  # Just the path
    target: "B.md"
  This is what we're accidentally doing now (bug becomes feature?)

  The Real Issue

  sequenceDiagram
      participant User
      participant File
      participant Link
      participant Hook

      User->>File: Edit A.md (fix typo)
      Note over File: SHA: abc → def
      Hook->>Link: Should link update?
      Link->>Link: 🤷 Link meant abc's ideas

      User->>File: Edit A.md (rewrite completely)
      Note over File: SHA: def → xyz
      Hook->>Link: Should link update?
      Link->>Link: 🤷 Now it definitely shouldn't!

  Hooks can't determine INTENT - was this a minor edit that preserves meaning, or a complete rewrite that breaks the semantic
  relationship?

  So You're Right... If We Give Up on Content-Addressing

  If we just track paths (not content), then yes, hooks solve everything:
  - git mv → Update path ✅
  - Edit file → Nothing to update (path unchanged) ✅
  - Delete file → Mark link as broken ✅

  But then we're not really linking specific ideas/content, just current files.

  The Philosophical Choice

  Content-addressed (Pure but Fragile):
  "Link the idea in A.md (as of Jan 2024) to the concept in B.md"

  Path-addressed (Practical but Loose):
  "Link whatever is currently in A.md to whatever is currently in B.md"

  You're right that hooks solve the mechanical problem. The question is: what are we actually trying to link? 🤔
```

---

### Problem 2: Branch Isolation Failure

**Discovery**: "The graph doesn't follow branches!"

```mermaid
gitGraph
    commit id: "main"
    branch feature
    commit id: "add B.md"
    checkout main
    commit id: "update A.md"
    
    branch refs/gitmind/graph
    commit id: "link A→B" type: HIGHLIGHT
    
    checkout main
    commit id: "B.md doesn't exist here!" tag: "PROBLEM"
```

The orphan ref is global - same links visible on all branches!

---

### Problem 3: Push/Pull Impossibility

**Discovery**: "How do you push refs/gitmind/graph to GitHub?"

```mermaid
sequenceDiagram
    participant Dev
    participant Git
    participant GitHub
    
    Dev->>Git: git push origin main
    Git->>GitHub: ✅ Code pushed
    
    Dev->>Git: git push origin refs/gitmind/graph
    Git->>GitHub: Push orphan ref
    GitHub-->>Git: ❌ refusing to create funny ref
    Dev->>Dev: 😭
```

Most Git hosts don't accept arbitrary refs!

---

### Problem 4: Git Workflows Broken

```mermaid
graph TD
    subgraph "Broken Workflows"
        W1[Git Worktrees] -->|Different .git dirs| B1[❌ No shared graph]
        W2[Sparse Checkout] -->|Files not in tree| B2[❌ Links to nothing]
        W3[Submodules] -->|Separate repos| B3[❌ Can't link across]
        W4[Shallow Clones] -->|No full history| B4[❌ Missing graph commits]
    end
    
    style B1 fill:#fcc
    style B2 fill:#fcc
    style B3 fill:#fcc
    style B4 fill:#fcc
```

---

### Problem 5: Tombstone Complexity

**Discovery**: "Do we really need tombstones when Git tracks deletions?"

```mermaid
graph LR
    subgraph "Tombstone Overhead"
        E1[Active Edges: 1,000]
        E2[Tombstones: 9,000]
        Total[Must scan 10,000 edges!]
        
        E1 --> Total
        E2 --> Total
    end
    
    subgraph "Git Already Has This"
        G[git log --diff-filter=D]
        G -->|Shows all deletions| H[With who/when/why!]
    end
    
    style E2 fill:#fcc
    style Total fill:#fee
    style H fill:#cfc
```

---

## Solutions We Explored

---

### Attempt 1: Git Hooks

**Idea**: Use hooks to update links on file operations

```mermaid
sequenceDiagram
    participant User
    participant Git
    participant Hook
    participant git-mind
    
    User->>Git: git mv old.md new.md
    Git->>Hook: post-commit
    Hook->>git-mind: update-path old.md new.md
    git-mind-->>Hook: Links updated
    Hook-->>User: Automatic fix!
```

**Result**: ✅ Helps with renames, ❌ Doesn't solve other problems

---

### Attempt 2: Path-Based Identity

**Idea**: Links are between paths, not content

```yaml
# Instead of content SHA
link:
  source_sha: abc123...
  target_sha: def456...
  
# Just use paths
link:
  source: README.md
  target: docs/api.md
```

**Result**: ✅ Survives edits, ❌ Still has branch/push problems

---

### Attempt 3: Hybrid Approach

**Idea**: Track both path and content

```yaml
link:
  source_path: README.md
  source_sha: abc123  # When linked
  source_current_sha: def456  # Now
  staleness: 0.7  # How much it changed
```

**Result**: ✅ Best of both worlds, ❌ Complex implementation

---

## Where We Are Now

### The Crossroads

```mermaid
graph TD
    subgraph "Current State"
        HS[Holy Grail<br/>Architecture]
        P1[❌ Can't push]
        P2[❌ Not branch-aware]
        P3[❌ File identity issues]
        P4[❌ Complex implementation]
        P5[✅ No merge conflicts]
        P6[✅ Pure Git objects]
        
        HS --> P1
        HS --> P2
        HS --> P3
        HS --> P4
        HS --> P5
        HS --> P6
    end
    
    subgraph "Proposed Alternative"
        WT[Working Tree<br/>Storage]
        A1[✅ Pushable]
        A2[✅ Branch-aware]
        A3[✅ Simple paths]
        A4[✅ Easy implementation]
        A5[✅ No merge conflicts*]
        A6[❌ Files in working tree]
        
        WT --> A1
        WT --> A2
        WT --> A3
        WT --> A4
        WT --> A5
        WT --> A6
    end
    
    style P1 fill:#fcc
    style P2 fill:#fcc
    style P3 fill:#fcc
    style P4 fill:#fcc
    style P5 fill:#cfc
    style P6 fill:#cfc
    
    style A1 fill:#cfc
    style A2 fill:#cfc
    style A3 fill:#cfc
    style A4 fill:#cfc
    style A5 fill:#cfc
    style A6 fill:#fcc
```

---

### Proposed Working Tree Structure

Keep the conflict-free design but in `.gitmind/`:

```
.gitmind/
└── links/
    ├── 01HPGJ4X7MZVR8QWTYC3BKEYN0.link  # ULID ensures uniqueness
    ├── 01HPGJ4X7MZVR8QWTYC3BKEYN1.link
    └── 01HPGJ4X7MZVR8QWTYC3BKEYN2.link
```

---

## The Decision Point

### Option A: Double Down on Holy Grail

**Changes Needed:**
1. Fix SHA bug (hash content, not paths)
2. Implement branch-aware graphs (`refs/gitmind/main`, `refs/gitmind/feature`)
3. Create sync tools for push/pull
4. Add worktree support somehow
5. Keep tombstones for distributed consistency

**Pros:**
- Architecturally pure
- No working directory files
- Theoretically elegant

**Cons:**
- Massive complexity
- Fighting Git's design
- Poor user experience
- Requires special tooling

---

### Option B: Pivot to Working Tree

**Changes Needed:**
1. Move `.gitmind/` to working directory
2. Keep ULID-based files to avoid conflicts
3. Remove orphan ref entirely
4. Simplify to path-based links
5. Let Git handle history (no tombstones)

**Pros:**
- Git-native workflows
- Branch awareness for free
- Pushable to any host
- Simple implementation
- Works with all Git features

**Cons:**
- Less "pure"
- Files in working directory
- Loses some theoretical elegance

---

### Option C: Hybrid Storage

**Idea:** Important links in working tree, cache in refs/

```
.gitmind/
├── links/          # Tracked, pushed with code
└── cache/          # Local only, like .git/

refs/gitmind/cache  # Local optimization
```

**Pros:**
- Best of both worlds
- Backwards compatible

**Cons:**
- Two systems to maintain
- Complexity

---

## Critical Questions

1. **What are we optimizing for?**
   - Theoretical purity?
   - User experience?
   - Implementation simplicity?

2. **Who is our user?**
   - Git experts who understand orphan refs?
   - Regular developers who just want links?

3. **What's our core value?**
   - "No files in working directory" at all costs?
   - "It just works" with normal Git workflows?

---

## The Irony

We tried so hard to be "pure Git" that we created something incompatible with Git's actual workflows. The orphan ref approach is like building a submarine and then realizing you need to drive on roads.

---

## Recommendation

**Embrace Simplicity**: The working tree approach (`.gitmind/links/`) gives us:

1. Everything users actually need
2. Compatibility with all Git workflows
3. Simple implementation
4. No merge conflicts (with ULID names)
5. Push/pull just works

The Holy Grail architecture is beautiful, but sometimes the simplest solution is the right one.

---

## Next Steps

We need to decide:

1. **Continue with Holy Grail?** (Fix all the issues)
2. **Pivot to Working Tree?** (Simpler, more compatible)
3. **Create Hybrid?** (Complex but flexible)

The code is at a crossroads. Which path do we take?

---

*"Perfection is achieved not when there is nothing more to add, but when there is nothing left to take away." - Antoine de Saint-Exupéry*

*Perhaps it's time to take away the complexity and embrace what Git does best: tracking files.*