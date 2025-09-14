### __Fresh-start blueprint —__

### __C-only, journal-commit graph, zero legacy baggage__

You’re free to wipe the slate, so let’s carve the “ultimate design” straight into a clean C codebase.

Below is a pragmatic build plan that sticks to _portable C99 + libgit2_ (no shell-outs), keeps each component <-500 lines, and lands you with a push-friendly, branch-aware, content-addressed graph in roughly three focused weekends.

---

## __0 High-level directory scaffold__

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

- __libgit2__ – pulls in portable object plumbing & ref manipulation without system() abuse.
- __croaring__ – tiny, MIT, single-TU roaring bitset for the cache layer.

---

## __1 Edge payload spec (binary, 32–56 B)__

|__Field (tag)__|__Bytes__|__Notes__|
|---|---|---|
|0x00 src_sha|20|blob SHA-1 (SHA-256 later → 32)|
|0x01 tgt_sha|20|blob SHA-1|
|0x02 rel_type|2|enum ID (uint16) → table in repo root|
|0x03 conf|2|IEEE-754 half|
|0x04 ts|8|unix millis (uint64)|
|0x05 extra_len + bytes|var|reserved|

Encode as __CBOR major-type 4 (array)__: [src_sha, tgt_sha, rel_id, conf, ts].
Serializer fits in ≈120 C lines.

---

## __2. Layer 0 — edge journal branch__

### __2.1 Commit writer (gm_journal_append)__

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

→ __~200 μs__ per edge on NVMe when batching  hundreds.

### __2.2 Branch mapping__

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

## __3. Layer 1 — fan-out bitmap cache__

### __3.1 Directory scheme (OBJECT-like but private)__

```
refs/gitmind/cache/heads/main/<fan1>/<fan2>/<src_blob_sha>.roar
```

- `fan1 = src_sha[0..1], fan2 = src_sha[2..3]` → each dir ≤ 256 files.
- File contents = roaring bitmap of __edge-IDs__ (uint32 index into journal order).
- Separate bitmap for _incoming_ edges lives under `.../in/`.

### __3.2 Builder (gm_cache_rebuild_until)__

- Input: repo, last_cache_commit, new_journal_tip
- Walk new journal commits (`libgit2 revwalk`).
- For each edge ADD:
    `roaring_bitmap_add(out_bitmap, edge_id)`
- For REMOVE (tombstone): `roaring_bitmap_remove`.
- Flush modified bitmaps with `git_blob_create_frombuffer`.
- Write fan-out trees → commit to `refs/gitmind/cache/....`

Runs in CI & optional local daemon; never pushed if you don’t want.

---

## __4 CLI commands (MVP)__

|__Command__|__Function__|
|---|---|
|`git mind link <src> <tgt> --type <verb> [--conf 0.9]`|– resolves path → blob SHA at HEAD– writes journal commit|
|`git mind list <path>`|– Fan-out bitmap lookup (cache if present else journal scan)|
|`git mind stale`|– Detect edges whose src_sha not reachable from HEAD → print, suggest relink|
|`git mind compact`|– Build cache incrementally|

---

## __5 Editor/Hook glue (later)__

- post-commit (shell): detect `git diff --name-status HEAD~` → for each rename add `RENAMED_PATH` edge so path mapping stays discoverable.
- VS Code extension talks over JSON-RPC: git mind list --json.

---

## __6 Timeline (three hobby weekends)__

|__W-end__|__Goals__|__LOC__|
|---|---|---|
|__1__|libgit2 init, edge CBOR coder, journal append & read|~600|
|__2__|basic CLI (link, list via journal scan), ULID gen|~500|
|__3__|roaring cache builder + fast list, stale detector, docs|~800|

Total ≈ 2 k C-lines — perfectly weekendable.

---

## __7 Potential dragons & antidotes__

|__Dragon__|__Antidote__|
|---|---|
|__Commit-message binary screws older tools__|Set encoding=binary header; GitHub PR diff just shows “Binary data not displayed”, harmless.|
|__Journal spam in git log__|Ship alias: git config alias.mlog 'log --exclude=refs/gitmind/edges/**'|
|__Mass edge import large commit__|Provide --batch 1000 to split after N edges to keep commit msg <1 MB.|
|__Edge-ID overflow (>4 B edges)__|Switch edge-ID to 48-bit, roaring64 variant — unlikely for hobby.|

---

## __8 Why this remains “ultimate”__

- __Content-addressable truth__: each blob version permanent; AUGMENTS gives lineage.
- __Branch-relative semantics__: links live where code lives.
- __Push-safe__: totally standard refs.
- __Cold-start queries__: scan ≤ (#new commits) not full history.
- __No WD garbage__: code tree stays pristine.
- __Fully recoverable__: lose cache? Rebuild from journal in O(history).
  
You get math-geek elegance _and_ a repo any Git host understands.

Enjoy the blank slate—go melt silicon. 🚀
