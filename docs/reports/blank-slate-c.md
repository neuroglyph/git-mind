### **Fresh-start blueprint —** 

### **C-only, journal-commit graph, zero legacy baggage**

You’re free to wipe the slate, so let’s carve the “ultimate design” straight into a clean C codebase.

Below is a pragmatic build plan that sticks to _portable C99 + libgit2_ (no shell-outs), keeps each component <-500 lines, and lands you with a push-friendly, branch-aware, content-addressed graph in roughly three focused weekends.

---

## **0 High-level directory scaffold**

```
c/
├── include/
│   └── gm.h                 # public API
├── src/
│   ├── journal.c            # write/read edge commits
│   ├── edge.c               # CBOR encode/decode
│   ├── cache.c              # bitmap fan-out builder
│   ├── bitmap.c             # roaring wrapper (croaring)
│   ├── cli.c                # main executable
│   └── util/                # sha1/sha256, ulid, strbuf, etc.
├── tests/
└── Makefile
```

- **libgit2** – pulls in portable object plumbing & ref manipulation without system() abuse.
- **croaring** – tiny, MIT, single-TU roaring bitset for the cache layer.

---

## **1 Edge payload spec (binary, 32–56 B)**

|**Field (tag)**|**Bytes**|**Notes**|
|---|---|---|
|0x00 src_sha|20|blob SHA-1 (SHA-256 later → 32)|
|0x01 tgt_sha|20|blob SHA-1|
|0x02 rel_type|2|enum ID (uint16) → table in repo root|
|0x03 conf|2|IEEE-754 half|
|0x04 ts|8|unix millis (uint64)|
|0x05 extra_len + bytes|var|reserved|

Encode as **CBOR major-type 4 (array)**: [src_sha, tgt_sha, rel_id, conf, ts].
Serializer fits in ≈120 C lines.

---

## **2. Layer 0 — edge journal branch**

### **2.1 Commit writer (gm_journal_append)**

```c
int gm_journal_append(git_repository *repo,
                      const gm_edge   *edges,
                      size_t           n_edges);
/* Steps:
   1. create empty tree (cached OID)
   2. build CBOR buffer for n_edges
   3. find journal ref: refs/gitmind/edges/<branchName>
   4. git_commit_create() with:
        tree   = emptyTree,
        parents= {old_head},
        message= (char*)cbor_buf, message_encoding="binary"
*/
```

→ **~200 μs** per edge on NVMe when batching  hundreds.

### **2.2 Branch mapping**

```
refs/gitmind/edges/heads/main      (mirrors refs/heads/main)
refs/gitmind/edges/heads/feature-x
```

Determine the suffix at runtime:

```
git_reference *head = git_repository_head(repo);
const char *name = git_reference_shorthand(head);   // "main"
char refname[64];
snprintf(refname, sizeof refname,
         "refs/gitmind/edges/heads/%s", name);
```

---

## **3. Layer 1 — fan-out bitmap cache**

### **3.1 Directory scheme (OBJECT-like but private)**

```
refs/gitmind/cache/heads/main/<fan1>/<fan2>/<src_blob_sha>.roar
```

- `fan1 = src_sha[0..1], fan2 = src_sha[2..3]` → each dir ≤ 256 files.
- File contents = roaring bitmap of **edge-IDs** (uint32 index into journal order).
- Separate bitmap for _incoming_ edges lives under `.../in/`.


### **3.2 Builder (gm_cache_rebuild_until)**

- Input: repo, last_cache_commit, new_journal_tip
- Walk new journal commits (`libgit2 revwalk`).
- For each edge ADD:
    `roaring_bitmap_add(out_bitmap, edge_id)`
- For REMOVE (tombstone): `roaring_bitmap_remove`.
- Flush modified bitmaps with `git_blob_create_frombuffer`.
- Write fan-out trees → commit to `refs/gitmind/cache/....`

Runs in CI & optional local daemon; never pushed if you don’t want.

---

## **4 CLI commands (MVP)**

|**Command**|**Function**|
|---|---|
|`git mind link <src> <tgt> --type <verb> [--conf 0.9]`|– resolves path → blob SHA at HEAD– writes journal commit|
|`git mind list <path>`|– Fan-out bitmap lookup (cache if present else journal scan)|
|`git mind stale`|– Detect edges whose src_sha not reachable from HEAD → print, suggest relink|
|`git mind compact`|– Build cache incrementally|

---

## **5 Editor/Hook glue (later)**

- post-commit (shell): detect `git diff --name-status HEAD~` → for each rename add `RENAMED_PATH` edge so path mapping stays discoverable.
- VS Code extension talks over JSON-RPC: git mind list --json.

---

## **6 Timeline (three hobby weekends)**

|**W-end**|**Goals**|**LOC**|
|---|---|---|
|**1**|libgit2 init, edge CBOR coder, journal append & read|~600|
|**2**|basic CLI (link, list via journal scan), ULID gen|~500|
|**3**|roaring cache builder + fast list, stale detector, docs|~800|

Total ≈ 2 k C-lines — perfectly weekendable.

---

## **7 Potential dragons & antidotes**

|**Dragon**|**Antidote**|
|---|---|
|**Commit-message binary screws older tools**|Set encoding=binary header; GitHub PR diff just shows “Binary data not displayed”, harmless.|
|**Journal spam in git log**|Ship alias: git config alias.mlog 'log --exclude=refs/gitmind/edges/**'|
|**Mass edge import large commit**|Provide --batch 1000 to split after N edges to keep commit msg <1 MB.|
|**Edge-ID overflow (>4 B edges)**|Switch edge-ID to 48-bit, roaring64 variant — unlikely for hobby.|

---

## **8 Why this remains “ultimate”**

- **Content-addressable truth**: each blob version permanent; AUGMENTS gives lineage.
- **Branch-relative semantics**: links live where code lives.
- **Push-safe**: totally standard refs.
- **Cold-start queries**: scan ≤ (#new commits) not full history.
- **No WD garbage**: code tree stays pristine.
- **Fully recoverable**: lose cache? Rebuild from journal in O(history).
  
You get math-geek elegance _and_ a repo any Git host understands.

Enjoy the blank slate—go melt silicon. 🚀
