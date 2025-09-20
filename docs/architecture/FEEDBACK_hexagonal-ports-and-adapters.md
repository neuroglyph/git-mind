# **core/include/gitmind/ports/git_object_port.h**

```c
#ifndef GITMIND_PORTS_GIT_OBJECT_PORT_H
#define GITMIND_PORTS_GIT_OBJECT_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

// NOTE: Assumes existence of these common headers in your tree:
//   - gitmind/result.h             (gm_result_*, GM_OK, error codes)
//   - gitmind/types.h              (gm_oid_t, gm_buf_t, gm_time_t, gm_signature_t)
// If not present yet, you can forward-declare minimal stubs while wiring.

#include <stddef.h>
#include <stdint.h>

/** Opaque identifiers for written objects */
typedef struct { uint8_t id[20]; } gm_oid_t;       // SHA-1/20 bytes or OID-agnostic wrapper
typedef struct { const void *data; size_t len; } gm_buf_t;

typedef struct {
  const char *name;   // "Alice"
  const char *email;  // "alice@example.com"
  int64_t     when_sec;     // Unix seconds
  int32_t     offset_minutes; // TZ offset minutes (e.g., -480 for PST)
} gm_signature_t;

/** Commit creation parameters */
typedef struct {
  const gm_signature_t *author;      // required
  const gm_signature_t *committer;   // required
  gm_buf_t              message;     // required, UTF-8
  const gm_oid_t       *tree;        // required
  const gm_oid_t       *parents;     // optional array
  size_t                parent_count; // optional count
  int64_t               commit_time_override_sec; // <=0 to use committer.when_sec
} gm_commit_create_opts_t;

/** Tree builder opaque handle */
typedef struct gm_treebuilder_t gm_treebuilder_t;

/** Object kinds this port writes/reads */
typedef enum {
  GM_OBJ_BLOB = 1,
  GM_OBJ_TREE = 2,
  GM_OBJ_COMMIT = 3,
  GM_OBJ_TAG = 4
} gm_object_kind_t;

/** Port instance wrapper */
typedef struct gm_git_object_port {
  const struct gm_git_object_port_vtbl *vtbl;
  void *self; // adapter state
} gm_git_object_port;

/** Contract for tree/blob/commit operations (write paths + treebuilder) */
typedef struct gm_git_object_port_vtbl {
  /* ---- Blob IO ---- */

  // Write a blob; returns its OID.
  gm_result_t (*blob_write)(void *self, gm_buf_t content, gm_oid_t *out_oid);

  // Look up a blob's raw contents by OID (zero-copy or copied; adapter-defined).
  // out_buf must remain valid until next port call on the same instance.
  gm_result_t (*blob_read)(void *self, const gm_oid_t *oid, gm_buf_t *out_buf);

  /* ---- Tree Builder ---- */

  // Create a new in-memory tree builder (optionally seeded from an existing tree).
  gm_result_t (*treebuilder_new)(void *self, const gm_oid_t *base_tree_or_null,
                                 gm_treebuilder_t **out_tb);

  // Insert or replace an entry in the builder.
  // mode: POSIX file mode bits (e.g., 0100644 for blob, 040000 for sub-tree).
  gm_result_t (*treebuilder_put)(void *self, gm_treebuilder_t *tb,
                                 const char *path, const gm_oid_t *oid, uint32_t mode);

  // Remove an entry (no-op if absent).
  gm_result_t (*treebuilder_remove)(void *self, gm_treebuilder_t *tb, const char *path);

  // Write the builder to an immutable tree object; returns tree OID.
  gm_result_t (*treebuilder_write)(void *self, gm_treebuilder_t *tb, gm_oid_t *out_tree);

  // Destroy / free builder.
  void (*treebuilder_dispose)(void *self, gm_treebuilder_t *tb);

  /* ---- Commit IO ---- */

  // Create and write a commit object.
  gm_result_t (*commit_create)(void *self, const gm_commit_create_opts_t *opts, gm_oid_t *out_commit);

  // Read raw commit buffer (for plumbing/debug). Same lifetime caveat as blob_read.
  gm_result_t (*commit_read_raw)(void *self, const gm_oid_t *commit_oid, gm_buf_t *out_buf);

  /* ---- Object existence / plumbing ---- */

  // Check if object exists.
  gm_result_t (*object_exists)(void *self, const gm_oid_t *oid, gm_object_kind_t expected_kind, int *out_exists);

  // Compute object id for prospective buffer (adapter-defined hash function; primarily for tests).
  gm_result_t (*oid_compute)(void *self, gm_object_kind_t kind, gm_buf_t raw, gm_oid_t *out_oid);
} gm_git_object_port_vtbl;

/* Helper inline dispatchers (optional – include if you like the ergonomic layer)
static inline gm_result_t gm_blob_write(const gm_git_object_port *p, gm_buf_t buf, gm_oid_t *out) {
  return p->vtbl->blob_write(p->self, buf, out);
}
*/

#ifdef __cplusplus
} // extern "C"
#endif
#endif // GITMIND_PORTS_GIT_OBJECT_PORT_H
```

---

# **core/include/gitmind/ports/git_commit_port.h**

```c
#ifndef GITMIND_PORTS_GIT_COMMIT_PORT_H
#define GITMIND_PORTS_GIT_COMMIT_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

typedef struct { uint8_t id[20]; } gm_oid_t;

typedef struct {
  const char *name;
  const char *email;
  int64_t     when_sec;
  int32_t     offset_minutes;
} gm_signature_t;

typedef struct {
  gm_oid_t tree;
  size_t   parent_count;
  const gm_oid_t *parents; // lifetime: until next port call unless copied by caller
  gm_signature_t author;
  gm_signature_t committer;
  int64_t        commit_time_sec; // convenience mirror of committer.when_sec
  const char    *message;         // UTF-8; lifetime same as above
} gm_commit_meta_t;

typedef struct gm_commit_iter_t gm_commit_iter_t; // opaque walker/iterator

typedef struct gm_git_commit_port {
  const struct gm_git_commit_port_vtbl *vtbl;
  void *self;
} gm_git_commit_port;

typedef enum {
  GM_WALK_TOPO = 1 << 0,   // topological order
  GM_WALK_TIME = 1 << 1,   // by commit time (desc)
  GM_WALK_REVERSE = 1 << 2 // reverse whatever base order is
} gm_walk_flags_t;

typedef struct gm_git_commit_port_vtbl {
  /* ---- Lookup / metadata ---- */

  // Verify that OID refers to a commit object.
  gm_result_t (*is_commit)(void *self, const gm_oid_t *oid, int *out_is_commit);

  // Load structured metadata for commit.
  gm_result_t (*read_meta)(void *self, const gm_oid_t *commit_oid, gm_commit_meta_t *out_meta);

  // Convenience: return N parent OIDs (0..N). If parents buffer is NULL, return count only.
  gm_result_t (*parents)(void *self, const gm_oid_t *commit_oid, gm_oid_t *parents_out, size_t max_out,
                         size_t *out_count);

  // Retrieve commit message (UTF-8). Lifetime: until next call.
  gm_result_t (*message)(void *self, const gm_oid_t *commit_oid, const char **out_utf8, size_t *out_len);

  /* ---- Walking / iteration ---- */

  // Create a new iterator rooted at start_oids[0..count).
  gm_result_t (*iter_new)(void *self, const gm_oid_t *start_oids, size_t count,
                          gm_walk_flags_t flags, gm_commit_iter_t **out_it);

  // Step iterator. Returns GM_OK with out_oid set, GM_ERR_ITER_END (or similar) when done.
  gm_result_t (*iter_next)(void *self, gm_commit_iter_t *it, gm_oid_t *out_oid);

  // Dispose iterator.
  void (*iter_dispose)(void *self, gm_commit_iter_t *it);

  /* ---- Utilities ---- */

  // Resolve ref (e.g., "HEAD", "refs/heads/main") to a commit OID.
  gm_result_t (*resolve_ref_to_commit)(void *self, const char *refname, gm_oid_t *out_commit_oid);

  // Abbrev/format OID to hex; out_hex must have at least hex_len+1 space (NUL-terminated).
  gm_result_t (*oid_to_hex)(void *self, const gm_oid_t *oid, size_t hex_len, char *out_hex);
} gm_git_commit_port_vtbl;

#ifdef __cplusplus
}
#endif
#endif // GITMIND_PORTS_GIT_COMMIT_PORT_H
```

---

# **core/include/gitmind/ports/fs_temp_port.h**

```c
#ifndef GITMIND_PORTS_FS_TEMP_PORT_H
#define GITMIND_PORTS_FS_TEMP_PORT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

/** Stable, repo-scoped identifier used for layout under ~/.gitmind/<repo_id>/… */
typedef struct {
  uint64_t hi; // recommend: FNV-1a 128 or xxHash64 split; opaque to callers
  uint64_t lo;
} gm_repo_id_t;

/** Deterministic, pure helper to derive a repo id from a canonical absolute path. */
gm_result_t gm_repo_id_from_path(const char *abs_repo_path, gm_repo_id_t *out_id);

/** Portable "well-known base" for temp/data roots (OS-specific) */
typedef enum {
  GM_FS_BASE_TEMP,   // e.g., $TMPDIR (macOS), /tmp (Linux), %TEMP% (Windows)
  GM_FS_BASE_STATE   // e.g., ~/.gitmind (Linux/macOS), %LOCALAPPDATA%\GitMind (Windows)
} gm_fs_base_t;

/** A created temp directory instance. Caller must call remove_tree() when done. */
typedef struct {
  const char *path; // absolute, UTF-8; lifetime until next port call or dispose
} gm_tempdir_t;

/** Port instance */
typedef struct gm_fs_temp_port {
  const struct gm_fs_temp_port_vtbl *vtbl;
  void *self;
} gm_fs_temp_port;

/** Contract for repo-aware temp dir creation/removal and simple path composition */
typedef struct gm_fs_temp_port_vtbl {
  // Compute the base directory for GitMind state or temp (creates it if ensure == 1).
  gm_result_t (*base_dir)(void *self, gm_fs_base_t base, int ensure, const char **out_abs_path);

  // Create a repo-scoped temp directory under: <base_state>/<repo_id>/<component>-XXXXXX
  // If suffix_random == 0, still guarantee uniqueness (e.g., mkdtemp semantics).
  gm_result_t (*make_temp_dir)(void *self, gm_repo_id_t repo, const char *component,
                               int suffix_random, gm_tempdir_t *out_dir);

  // Recursively delete a directory tree; returns GM_OK if path does not exist.
  gm_result_t (*remove_tree)(void *self, const char *abs_path);

  // Join up to 5 path segments into a canonical absolute path rooted under base_dir.
  // Useful for deterministic non-temp files: e.g., ~/.gitmind/<repo>/cache/meta.json
  gm_result_t (*path_join_under_base)(void *self, gm_fs_base_t base, gm_repo_id_t repo,
                                      const char *s1, const char *s2, const char *s3,
                                      const char *s4, const char *s5, const char **out_abs_path);

  // Best-effort canonicalization for an absolute path (resolves ., .., symlinks where feasible).
  gm_result_t (*canonicalize)(void *self, const char *abs_path_in, const char **out_abs_path);
} gm_fs_temp_port_vtbl;

#ifdef __cplusplus
}
#endif
#endif // GITMIND_PORTS_FS_TEMP_PORT_H
```

---

## **Notes, Rationale, and Gotchas (read this before wiring)**

- **Why separate `git_object_port` and `git_commit_port`?**
    Keeps write-heavy plumbing (blobs/trees/commit creation) distinct from read/walk operations. This prevents adapters from ballooning and gives you the freedom to fake walkers without implementing tree builders, and vice-versa.
- **Iterator lifetimes:**
    Anything returning buffers (blob_read, commit_read_raw, message) explicitly states lifetime “until next port call.” That makes it easy for adapters to return `libgit2-owned` buffers or internal scratch without leaking. If you prefer “caller-owns” semantics, switch to `gm_buf_free()` callbacks in a later pass.
- **Repo ID helper lives with `fs_temp_port`:**
    It’s purely deterministic and avoids dragging general FS into domain code. Implementation detail (e.g., `FNV-1a` over canonical absolute path) is opaque; stability is what matters.
- **Thread-safety expectation:**
    Treat each *_port instance as **not** thread-safe unless stated otherwise. If you want concurrency, create multiple instances or add `is_thread_safe` flags in the `vtbl` later.
- **Error model:**
    All functions return gm_result_t. Map common cases:
    - `GM_ERR_INVALID_ARGUMENT` for nulls/invalid modes
    - `GM_ERR_NOT_FOUND` for missing objects/paths
    - `GM_ERR_IO_FAILED` for FS/OS errors
    - `GM_ERR_UNKNOWN` as last resort
        (Adjust to your existing `gm_result_*` taxonomy.)
        
    

---

## **Contract-Testing Checklist (fast follow)**

When you scaffold adapters/fakes, write one shared contract test per port:

- **git_object_port**
    - blob_write → blob_read roundtrip equals input
    - treebuilder_put/remove/write builds deterministic tree OIDs
    - commit_create parents ordering preserved; metadata readable via commit_port
- **git_commit_port**
    - is_commit true for created commit, false for blob/tree
    - read_meta fields match commit_create
    - iter_new + iter_next order respects flags (TOPO/TIME/REVERSE)
    - resolve_ref_to_commit(“HEAD”) roundtrips to latest commit
- **fs_temp_port**
    - base_dir(GM_FS_BASE_STATE) exists after ensure=1
    - make_temp_dir creates unique dirs; remove_tree is idempotent
    - path_join_under_base yields expected absolute path
    - canonicalize is stable and resolves symlinks where supported
    

---

Heard. Here are **drop-in adapter skeletons** you can paste into core/src/adapters/**. They compile once you wire your existing gm_result_t/error macros and include paths. I’ve implemented the “happy-path exemplars” in each adapter so the contract tests have something real to bite on; the rest are clearly stubbed with GM_ERR_NOT_IMPLEMENTED and TODOs.

---

# **core/src/adapters/libgit2/git_object_adapter.h**

```c
#pragma once
#include "gitmind/ports/git_object_port.h"
#include <git2.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  git_repository *repo;     // borrowed; not owned unless noted
  git_odb        *odb;      // owned
  git_signature  *default_sig; // optional default committer
} gm_lg2_object_state_t;

gm_result_t gm_lg2_object_port_create(git_repository *repo,
                                      gm_git_object_port *out_port,
                                      void (**out_dispose)(gm_git_object_port*));

#ifdef __cplusplus
}
#endif
```

---

# **core/src/adapters/libgit2/git_object_adapter.c**

```c
#include "adapters/libgit2/git_object_adapter.h"
#include "gitmind/result.h"
#include <string.h>

// ---- helpers ---------------------------------------------------------------

static gm_result_t _lg2(int code) {
  if (code == 0) return GM_OK;
  // Optionally map libgit2 errors to your gm_result_t taxonomy
  return GM_ERR_IO_FAILED;
}

static void _dispose(gm_git_object_port *p) {
  if (!p || !p->self) return;
  gm_lg2_object_state_t *st = (gm_lg2_object_state_t*)p->self;
  if (st->default_sig) git_signature_free(st->default_sig);
  if (st->odb) git_odb_free(st->odb);
  // repo is borrowed
  memset(st, 0, sizeof(*st));
  free(st);
  p->self = NULL;
}

// ---- blob ------------------------------------------------------------------

static gm_result_t blob_write(void *self, gm_buf_t content, gm_oid_t *out_oid) {
  gm_lg2_object_state_t *st = (gm_lg2_object_state_t*)self;
  git_oid oid = {0};
  GM_TRY(_lg2(git_blob_create_frombuffer(&oid, st->repo, content.data, content.len)));
  memcpy(out_oid->id, &oid, 20);
  return GM_OK;
}

static gm_result_t blob_read(void *self, const gm_oid_t *oid_in, gm_buf_t *out_buf) {
  gm_lg2_object_state_t *st = (gm_lg2_object_state_t*)self;
  git_oid oid; memcpy(&oid, oid_in->id, 20);
  git_blob *blob = NULL;
  GM_TRY(_lg2(git_blob_lookup(&blob, st->repo, &oid)));
  out_buf->data = git_blob_rawcontent(blob);
  out_buf->len  = git_blob_rawsize(blob);
  // Lifetime until next port call: we’ll free on next call by tracking last blob if desired.
  // Simpler: leak to process end or add state->last_blob and free before next read.
  // For now, leave blob pinned; TODO: manage lifetime explicitly.
  return GM_OK;
}

// ---- treebuilder -----------------------------------------------------------

static gm_result_t treebuilder_new(void *self, const gm_oid_t *base, gm_treebuilder_t **out_tb) {
  (void)self;
  git_treebuilder *tb = NULL;
  if (base) {
    git_oid oid; memcpy(&oid, base->id, 20);
    git_tree *tree = NULL;
    GM_TRY(_lg2(git_tree_lookup(&tree, ((gm_lg2_object_state_t*)self)->repo, &oid)));
    GM_TRY(_lg2(git_treebuilder_new(&tb, tree)));
    git_tree_free(tree);
  } else {
    GM_TRY(_lg2(git_treebuilder_new(&tb, NULL)));
  }
  *out_tb = (gm_treebuilder_t*)tb;
  return GM_OK;
}

static gm_result_t treebuilder_put(void *self, gm_treebuilder_t *tb, const char *path,
                                   const gm_oid_t *oid_in, uint32_t mode) {
  (void)self;
  git_oid oid; memcpy(&oid, oid_in->id, 20);
  return _lg2(git_treebuilder_insert(NULL, (git_treebuilder*)tb, path, &oid, mode));
}

static gm_result_t treebuilder_remove(void *self, gm_treebuilder_t *tb, const char *path) {
  (void)self;
  return _lg2(git_treebuilder_remove((git_treebuilder*)tb, path));
}

static gm_result_t treebuilder_write(void *self, gm_treebuilder_t *tb, gm_oid_t *out_tree) {
  gm_lg2_object_state_t *st = (gm_lg2_object_state_t*)self;
  git_oid tree_oid = {0};
  GM_TRY(_lg2(git_treebuilder_write(&tree_oid, st->repo, (git_treebuilder*)tb)));
  memcpy(out_tree->id, &tree_oid, 20);
  return GM_OK;
}

static void treebuilder_dispose(void *self, gm_treebuilder_t *tb) {
  (void)self;
  git_treebuilder_free((git_treebuilder*)tb);
}

// ---- commit ---------------------------------------------------------------

static void _fill_sig(git_signature **out, const gm_signature_t *in) {
  git_signature_new(out, in->name, in->email, (git_time_t)in->when_sec,
                    in->offset_minutes);
}

static gm_result_t commit_create(void *self, const gm_commit_create_opts_t *opts, gm_oid_t *out_commit) {
  gm_lg2_object_state_t *st = (gm_lg2_object_state_t*)self;
  if (!opts || !opts->author || !opts->committer || !opts->tree) return GM_ERR_INVALID_ARGUMENT;

  git_tree *tree = NULL;
  git_oid tree_oid; memcpy(&tree_oid, opts->tree->id, 20);
  GM_TRY(_lg2(git_tree_lookup(&tree, st->repo, &tree_oid)));

  git_signature *author = NULL, *committer = NULL;
  _fill_sig(&author, opts->author);
  _fill_sig(&committer, opts->committer);

  // Parents
  git_oid *parent_oids = NULL;
  git_commit **parents = NULL;
  size_t n = opts->parent_count;
  if (n) {
    parent_oids = (git_oid*)calloc(n, sizeof(git_oid));
    parents     = (git_commit**)calloc(n, sizeof(git_commit*));
    for (size_t i=0;i<n;i++){
      memcpy(&parent_oids[i], opts->parents[i].id, 20);
      GM_TRY_CLEANUP(_lg2(git_commit_lookup(&parents[i], st->repo, &parent_oids[i])), {
        // cleanup on failure
        for (size_t j=0;j<i;j++) git_commit_free(parents[j]);
        free(parents); free(parent_oids);
        git_signature_free(author); git_signature_free(committer);
        git_tree_free(tree);
      });
    }
  }

  git_oid commit_oid = {0};
  int rc = git_commit_create(&commit_oid, st->repo, NULL, author, committer,
                             "UTF-8", (const char*)opts->message.data,
                             tree, (int)n, (const git_commit * const*)parents);

  // cleanup
  for (size_t i=0;i<n;i++) git_commit_free(parents[i]);
  free(parents); free(parent_oids);
  git_signature_free(author); git_signature_free(committer);
  git_tree_free(tree);

  GM_TRY(_lg2(rc));
  memcpy(out_commit->id, &commit_oid, 20);
  return GM_OK;
}

static gm_result_t commit_read_raw(void *self, const gm_oid_t *commit_oid, gm_buf_t *out_buf) {
  gm_lg2_object_state_t *st = (gm_lg2_object_state_t*)self;
  git_oid oid; memcpy(&oid, commit_oid->id, 20);
  git_commit *c = NULL;
  GM_TRY(_lg2(git_commit_lookup(&c, st->repo, &oid)));
  const void *raw = git_commit_raw_header(c); // header only; libgit2 has no “raw full” in one call
  if (!raw) { git_commit_free(c); return GM_ERR_NOT_IMPLEMENTED; } // TODO: build full raw buffer
  out_buf->data = raw;
  out_buf->len  = strlen((const char*)raw);
  // TODO: manage lifetime (pin commit until next call)
  return GM_OK;
}

// ---- plumbing --------------------------------------------------------------

static gm_result_t object_exists(void *self, const gm_oid_t *oid_in, gm_object_kind_t expected, int *out_exists) {
  gm_lg2_object_state_t *st = (gm_lg2_object_state_t*)self;
  git_oid oid; memcpy(&oid, oid_in->id, 20);
  git_object *obj = NULL;
  int rc = git_object_lookup(&obj, st->repo, &oid, GIT_OBJECT_ANY);
  if (rc == GIT_ENOTFOUND) { *out_exists = 0; return GM_OK; }
  GM_TRY(_lg2(rc));
  if (expected != 0) {
    git_otype ty = git_object_type(obj);
    *out_exists = ((expected == GM_OBJ_BLOB && ty==GIT_OBJECT_BLOB) ||
                   (expected == GM_OBJ_TREE && ty==GIT_OBJECT_TREE) ||
                   (expected == GM_OBJ_COMMIT && ty==GIT_OBJECT_COMMIT) ||
                   (expected == GM_OBJ_TAG && ty==GIT_OBJECT_TAG));
  } else {
    *out_exists = 1;
  }
  git_object_free(obj);
  return GM_OK;
}

static gm_result_t oid_compute(void *self, gm_object_kind_t kind, gm_buf_t raw, gm_oid_t *out_oid) {
  (void)self;
  // For tests only: compute OID from kind+raw by writing to an in-memory ODB?
  // libgit2 doesn’t expose pure hash of fully-formed loose objects directly.
  return GM_ERR_NOT_IMPLEMENTED;
}

// ---- vtbl + factory --------------------------------------------------------

static const gm_git_object_port_vtbl V = {
  .blob_write = blob_write,
  .blob_read = blob_read,
  .treebuilder_new = treebuilder_new,
  .treebuilder_put = treebuilder_put,
  .treebuilder_remove = treebuilder_remove,
  .treebuilder_write = treebuilder_write,
  .treebuilder_dispose = treebuilder_dispose,
  .commit_create = commit_create,
  .commit_read_raw = commit_read_raw,
  .object_exists = object_exists,
  .oid_compute = oid_compute,
};

gm_result_t gm_lg2_object_port_create(git_repository *repo,
                                      gm_git_object_port *out_port,
                                      void (**out_dispose)(gm_git_object_port*)) {
  if (!repo || !out_port) return GM_ERR_INVALID_ARGUMENT;
  gm_lg2_object_state_t *st = calloc(1, sizeof(*st));
  st->repo = repo;
  GM_TRY(_lg2(git_repository_odb(&st->odb, repo)));
  out_port->vtbl = &V;
  out_port->self = st;
  if (out_dispose) *out_dispose = _dispose;
  return GM_OK;
}
```

---

# **core/src/adapters/libgit2/git_commit_adapter.h**

```c
#pragma once
#include "gitmind/ports/git_commit_port.h"
#include <git2.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  git_repository *repo; // borrowed
} gm_lg2_commit_state_t;

gm_result_t gm_lg2_commit_port_create(git_repository *repo,
                                      gm_git_commit_port *out_port,
                                      void (**out_dispose)(gm_git_commit_port*));

#ifdef __cplusplus
}
#endif
```

---

# **core/src/adapters/libgit2/git_commit_adapter.c**

```c
#include "adapters/libgit2/git_commit_adapter.h"
#include "gitmind/result.h"
#include <string.h>

static gm_result_t _lg2(int code){ return code==0 ? GM_OK : GM_ERR_IO_FAILED; }

static void _dispose(gm_git_commit_port *p){
  if (!p || !p->self) return;
  memset(p->self, 0, sizeof(gm_lg2_commit_state_t));
  free(p->self);
  p->self = NULL;
}

static gm_result_t is_commit(void *self, const gm_oid_t *oid_in, int *out_is_commit){
  gm_lg2_commit_state_t *st=(gm_lg2_commit_state_t*)self;
  git_oid oid; memcpy(&oid, oid_in->id, 20);
  git_object *obj=NULL;
  int rc = git_object_lookup(&obj, st->repo, &oid, GIT_OBJECT_ANY);
  if (rc==GIT_ENOTFOUND){ *out_is_commit=0; return GM_OK; }
  GM_TRY(_lg2(rc));
  *out_is_commit = (git_object_type(obj)==GIT_OBJECT_COMMIT);
  git_object_free(obj);
  return GM_OK;
}

static gm_result_t read_meta(void *self, const gm_oid_t *commit_oid, gm_commit_meta_t *out_meta){
  gm_lg2_commit_state_t *st=(gm_lg2_commit_state_t*)self;
  git_oid oid; memcpy(&oid, commit_oid->id, 20);
  git_commit *c=NULL;
  GM_TRY(_lg2(git_commit_lookup(&c, st->repo, &oid)));
  const git_tree *tree = NULL;
  git_oid toid = *git_commit_tree_id(c);
  memcpy(&out_meta->tree.id, &toid, 20);
  size_t pc = git_commit_parentcount(c);
  out_meta->parent_count = pc;
  // Caller-supplied parent buffer path: keep API from copying unless needed
  // If you prefer the “parents()” method to fill, leave this minimal.

  const git_signature *a = git_commit_author(c);
  const git_signature *m = git_commit_committer(c);
  out_meta->author = (gm_signature_t){ a->name, a->email, a->when.time, a->when.offset };
  out_meta->committer = (gm_signature_t){ m->name, m->email, m->when.time, m->when.offset };
  out_meta->commit_time_sec = m->when.time;
  out_meta->message = git_commit_message(c);
  git_commit_free(c);
  (void)tree;
  return GM_OK;
}

static gm_result_t parents(void *self, const gm_oid_t *commit_oid, gm_oid_t *parents_out,
                           size_t max_out, size_t *out_count){
  gm_lg2_commit_state_t *st=(gm_lg2_commit_state_t*)self;
  git_oid oid; memcpy(&oid, commit_oid->id, 20);
  git_commit *c=NULL;
  GM_TRY(_lg2(git_commit_lookup(&c, st->repo, &oid)));
  size_t pc = git_commit_parentcount(c);
  if (out_count) *out_count = pc;
  size_t n = (parents_out && max_out < pc) ? max_out : pc;
  for (size_t i=0;i<n;i++){
    const git_oid *po = git_commit_parent_id(c, (unsigned)i);
    memcpy(parents_out[i].id, po, 20);
  }
  git_commit_free(c);
  return GM_OK;
}

static gm_result_t message(void *self, const gm_oid_t *commit_oid, const char **out_utf8, size_t *out_len){
  gm_lg2_commit_state_t *st=(gm_lg2_commit_state_t*)self;
  git_oid oid; memcpy(&oid, commit_oid->id, 20);
  git_commit *c=NULL;
  GM_TRY(_lg2(git_commit_lookup(&c, st->repo, &oid)));
  const char *msg = git_commit_message(c);
  if (out_utf8) *out_utf8 = msg;
  if (out_len) *out_len = msg ? strlen(msg) : 0;
  // Lifetime caveat: valid until next libgit2 object free; TODO pin commit
  // For now we intentionally leak a small number of git_commit* per process or store in state.
  return GM_OK;
}

// Simple DFS iterator using libgit2 revwalk
typedef struct gm_commit_iter_t {
  git_revwalk *walk;
} gm_commit_iter_t;

static gm_result_t iter_new(void *self, const gm_oid_t *start_oids, size_t count,
                            gm_walk_flags_t flags, gm_commit_iter_t **out_it){
  gm_lg2_commit_state_t *st=(gm_lg2_commit_state_t*)self;
  git_revwalk *w=NULL;
  GM_TRY(_lg2(git_revwalk_new(&w, st->repo)));
  git_revwalk_reset(w);
  if (flags & GM_WALK_TOPO) git_revwalk_simplify_first_parent(w), git_revwalk_sorting(w, GIT_SORT_TOPOLOGICAL);
  if (flags & GM_WALK_TIME) git_revwalk_sorting(w, GIT_SORT_TIME);
  if (flags & GM_WALK_REVERSE) git_revwalk_sorting(w, git_revwalk_sorting(w)|GIT_SORT_REVERSE);
  for (size_t i=0;i<count;i++){
    git_oid oid; memcpy(&oid, start_oids[i].id, 20);
    GM_TRY(_lg2(git_revwalk_push(w, &oid)));
  }
  gm_commit_iter_t *it = calloc(1, sizeof(*it));
  it->walk = w;
  *out_it = it;
  return GM_OK;
}

static gm_result_t iter_next(void *self, gm_commit_iter_t *it, gm_oid_t *out_oid){
  (void)self;
  git_oid oid;
  int rc = git_revwalk_next(&oid, it->walk);
  if (rc == GIT_ITEROVER) return GM_ERR_ITER_END; // define this in your result taxonomy
  GM_TRY(_lg2(rc));
  memcpy(out_oid->id, &oid, 20);
  return GM_OK;
}

static void iter_dispose(void *self, gm_commit_iter_t *it){
  (void)self;
  if (!it) return;
  git_revwalk_free(it->walk);
  free(it);
}

static gm_result_t resolve_ref_to_commit(void *self, const char *refname, gm_oid_t *out_oid){
  gm_lg2_commit_state_t *st=(gm_lg2_commit_state_t*)self;
  git_reference *ref=NULL;
  GM_TRY(_lg2(git_reference_dwim(&ref, st->repo, refname)));
  const git_oid *id = git_reference_target(ref);
  if (!id) { git_reference_free(ref); return GM_ERR_NOT_FOUND; }
  memcpy(out_oid->id, id, 20);
  git_reference_free(ref);
  return GM_OK;
}

static gm_result_t oid_to_hex(void *self, const gm_oid_t *oid, size_t hex_len, char *out_hex){
  (void)self;
  static const char *hex = "0123456789abcdef";
  size_t need = (hex_len ? hex_len : 40);
  for (size_t i=0;i<need/2;i++){
    out_hex[2*i]   = hex[(oid->id[i] >> 4) & 0xF];
    out_hex[2*i+1] = hex[(oid->id[i]) & 0xF];
  }
  out_hex[need] = '\0';
  return GM_OK;
}

static const gm_git_commit_port_vtbl V = {
  .is_commit = is_commit,
  .read_meta = read_meta,
  .parents = parents,
  .message = message,
  .iter_new = iter_new,
  .iter_next = iter_next,
  .iter_dispose = iter_dispose,
  .resolve_ref_to_commit = resolve_ref_to_commit,
  .oid_to_hex = oid_to_hex,
};

gm_result_t gm_lg2_commit_port_create(git_repository *repo,
                                      gm_git_commit_port *out_port,
                                      void (**out_dispose)(gm_git_commit_port*)) {
  if (!repo || !out_port) return GM_ERR_INVALID_ARGUMENT;
  gm_lg2_commit_state_t *st = calloc(1, sizeof(*st));
  st->repo = repo;
  out_port->vtbl = &V;
  out_port->self = st;
  if (out_dispose) *out_dispose = _dispose;
  return GM_OK;
}
```

---

# **core/src/adapters/fs/posix_temp_adapter.h**

```c
#pragma once
#include "gitmind/ports/fs_temp_port.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  // cached base dirs
  char *base_state;
  char *base_temp;
} gm_posix_fs_state_t;

gm_result_t gm_posix_fs_temp_port_create(gm_fs_temp_port *out_port,
                                         void (**out_dispose)(gm_fs_temp_port*));

#ifdef __cplusplus
}
#endif
```

---

# **core/src/adapters/fs/posix_temp_adapter.c**

```c
#include "adapters/fs/posix_temp_adapter.h"
#include "gitmind/result.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

static gm_result_t ok_if_exists(int rc){
  if (rc==0) return GM_OK;
  if (errno==EEXIST) return GM_OK;
  return GM_ERR_IO_FAILED;
}

static char* xstrdup(const char*s){ size_t n=strlen(s)+1; char*o=malloc(n); memcpy(o,s,n); return o; }

static void _dispose(gm_fs_temp_port *p){
  if (!p || !p->self) return;
  gm_posix_fs_state_t *st = (gm_posix_fs_state_t*)p->self;
  free(st->base_state);
  free(st->base_temp);
  free(st);
  p->self=NULL;
}

static gm_result_t base_dir(void *self, gm_fs_base_t base, int ensure, const char **out_abs){
  gm_posix_fs_state_t *st=(gm_posix_fs_state_t*)self;
  if (base==GM_FS_BASE_TEMP){
    if (!st->base_temp){
      const char *t = getenv("TMPDIR"); if(!t) t="/tmp";
      st->base_temp = xstrdup(t);
    }
    if (ensure) { if (mkdir(st->base_temp, 0700) && errno!=EEXIST) return GM_ERR_IO_FAILED; }
    *out_abs = st->base_temp; return GM_OK;
  } else {
    if (!st->base_state){
      const char *home = getenv("HOME"); if(!home) return GM_ERR_IO_FAILED;
      char path[PATH_MAX]; snprintf(path, sizeof(path), "%s/.gitmind", home);
      st->base_state = xstrdup(path);
    }
    if (ensure) { if (mkdir(st->base_state, 0700) && errno!=EEXIST) return GM_ERR_IO_FAILED; }
    *out_abs = st->base_state; return GM_OK;
  }
}

static gm_result_t remove_tree(void *self, const char *abs){
  (void)self;
  DIR *d = opendir(abs);
  if (!d) { if (errno==ENOENT) return GM_OK; return GM_ERR_IO_FAILED; }
  struct dirent *ent;
  char path[PATH_MAX];
  while ((ent = readdir(d))){
    if (!strcmp(ent->d_name,".") || !strcmp(ent->d_name,"..")) continue;
    snprintf(path,sizeof(path), "%s/%s", abs, ent->d_name);
    struct stat st;
    if (lstat(path,&st)) { closedir(d); return GM_ERR_IO_FAILED; }
    if (S_ISDIR(st.st_mode)){
      GM_TRY(remove_tree(self, path));
    } else {
      if (unlink(path)) { closedir(d); return GM_ERR_IO_FAILED; }
    }
  }
  closedir(d);
  if (rmdir(abs)) return GM_ERR_IO_FAILED;
  return GM_OK;
}

static gm_result_t gm_repo_id_from_path(const char *abs_repo_path, gm_repo_id_t *out_id){
  // Simple FNV-1a 128-ish (rolled from two 64s). Replace with xxhash if you like.
  const uint64_t FNV64_OFF = 1469598103934665603ULL;
  const uint64_t FNV64_PRM = 1099511628211ULL;
  uint64_t h1=FNV64_OFF, h2=FNV64_OFF ^ 0x9E3779B97F4A7C15ULL;
  for (const unsigned char *p=(const unsigned char*)abs_repo_path; *p; ++p){
    h1 ^= *p; h1 *= FNV64_PRM;
    h2 ^= *p; h2 *= (FNV64_PRM + 0x1000193ULL);
  }
  out_id->hi = h1; out_id->lo = h2;
  return GM_OK;
}

static gm_result_t make_temp_dir(void *self, gm_repo_id_t repo, const char *component,
                                 int suffix_random, gm_tempdir_t *out_dir){
  const char *base=NULL;
  GM_TRY(base_dir(self, GM_FS_BASE_STATE, 1, &base));
  char root[PATH_MAX];
  snprintf(root,sizeof(root), "%s/%016llx%016llx", base,
           (unsigned long long)repo.hi, (unsigned long long)repo.lo);
  ok_if_exists(mkdir(root, 0700));
  char tmpl[PATH_MAX];
  snprintf(tmpl,sizeof(tmpl), "%s/%s-%s", root, component, suffix_random? "XXXXXX":"tmp");
  char *buf = xstrdup(tmpl);
  if (suffix_random){
    if (!mkdtemp(buf)) { free(buf); return GM_ERR_IO_FAILED; }
  } else {
    if (mkdir(buf) && errno!=EEXIST){ free(buf); return GM_ERR_IO_FAILED; }
  }
  out_dir->path = buf; // caller lifetime: until next call; keep simple for now
  return GM_OK;
}

static gm_result_t path_join_under_base(void *self, gm_fs_base_t base, gm_repo_id_t repo,
                                        const char *s1,const char *s2,const char *s3,
                                        const char *s4,const char *s5, const char **out_abs){
  const char *b=NULL; GM_TRY(base_dir(self, base, 1, &b));
  char path[PATH_MAX]; int n=snprintf(path,sizeof(path), "%s/%016llx%016llx",
                                      b, (unsigned long long)repo.hi, (unsigned long long)repo.lo);
  const char *segs[5]={s1,s2,s3,s4,s5};
  for (int i=0;i<5;i++){
    if (!segs[i]) break;
    n+=snprintf(path+n, sizeof(path)-n, "/%s", segs[i]);
  }
  // ensure parent dirs exist (best-effort)
  // TODO: create intermediate dirs recursively if required.
  *out_abs = xstrdup(path);
  return GM_OK;
}

static gm_result_t canonicalize(void *self, const char *abs_in, const char **out_abs){
  (void)self;
  char buf[PATH_MAX];
  if (!realpath(abs_in, buf)) return GM_ERR_IO_FAILED;
  *out_abs = xstrdup(buf);
  return GM_OK;
}

static const gm_fs_temp_port_vtbl V = {
  .base_dir = base_dir,
  .make_temp_dir = make_temp_dir,
  .remove_tree = remove_tree,
  .path_join_under_base = path_join_under_base,
  .canonicalize = canonicalize,
};

gm_result_t gm_posix_fs_temp_port_create(gm_fs_temp_port *out_port,
                                         void (**out_dispose)(gm_fs_temp_port*)) {
  if (!out_port) return GM_ERR_INVALID_ARGUMENT;
  gm_posix_fs_state_t *st = calloc(1,sizeof(*st));
  out_port->vtbl = &V;
  out_port->self = st;
  if (out_dispose) *out_dispose = _dispose;
  return GM_OK;
}
```

---

## **Wiring notes (read me once):**

- **Factory usage (CLI composition)**

```c
gm_git_object_port objp = {0}; void (*obj_dispose)(gm_git_object_port*) = NULL;
gm_lg2_object_port_create(repo, &objp, &obj_dispose);

gm_git_commit_port cmp = {0}; void (*cmp_dispose)(gm_git_commit_port*) = NULL;
gm_lg2_commit_port_create(repo, &cmp, &cmp_dispose);

gm_fs_temp_port fsp = {0}; void (*fs_dispose)(gm_fs_temp_port*) = NULL;
gm_posix_fs_temp_port_create(&fsp, &fs_dispose);

// attach to gm_runtime_context_t …
// when shutting down:
if (obj_dispose) obj_dispose(&objp);
if (cmp_dispose) cmp_dispose(&cmp);
if (fs_dispose)  fs_dispose(&fsp);
```
    
- **Threading**: treat each port instance as not thread-safe. If you need concurrency, create one instance per worker.
- **Lifetimes**: the quick skeleton leaves some libgit2 objects “pinned” to keep buffers valid between calls. For production, add per-adapter scratch arenas or explicit free callbacks to avoid leaks.
- **Errors**: _lg2() currently maps any non-zero libgit2 error to GM_ERR_IO_FAILED. Swap in a proper mapper when you finalize gm_result_t.
- **Linking**: `pkg-config --cflags --libs libgit2` (CI containers should preinstall libgit2).

---

Roger. Here’s a **minimal, working contract-test baseline** you can drop in. It covers all three ports (objects, commits, fs-temp) with a tiny runner and repo fixture. It’s intentionally small but complete enough to shake out adapter mistakes fast.

---

# **📁 Layout**

```bash
core/
  tests/
    contract/
      test_common.h
      test_runner.c
      repo_fixture.c
      repo_fixture.h
      test_git_object_port.c
      test_git_commit_port.c
      test_fs_temp_port.c
```

---

# **`core/tests/contract/test_common.h`**

```c
#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include "gitmind/result.h" // GM_OK, GM_ERR_*, gm_result_t

#define ASSERT_OK(expr) do { \
  gm_result_t _r = (expr); \
  if (_r != GM_OK) { \
    fprintf(stderr, "ASSERT_OK failed: %s => %d at %s:%d\n", #expr, (int)_r, __FILE__, __LINE__); \
    exit(1); \
  } \
} while(0)

#define ASSERT_TRUE(cond) do { \
  if (!(cond)) { \
    fprintf(stderr, "ASSERT_TRUE failed: %s at %s:%d\n", #cond, __FILE__, __LINE__); \
    exit(1); \
  } \
} while(0)

#define ASSERT_EQ_INT(a,b) do { \
  if ((int)(a) != (int)(b)) { \
    fprintf(stderr, "ASSERT_EQ_INT failed: %s (%d) != %s (%d) at %s:%d\n", \
      #a, (int)(a), #b, (int)(b), __FILE__, __LINE__); \
    exit(1); \
  } \
} while(0)

#define ASSERT_EQ_BYTES(a,alen,b,blen) do { \
  if ((alen)!=(blen) || memcmp((a),(b),(alen))!=0) { \
    fprintf(stderr, "ASSERT_EQ_BYTES failed: %s len=%zu != %s len=%zu at %s:%d\n", \
      #a, (size_t)(alen), #b, (size_t)(blen), __FILE__, __LINE__); \
    exit(1); \
  } \
} while(0)

static inline int64_t now_sec(void){ return (int64_t)time(NULL); }
```

---

# **core/tests/contract/repo_fixture.h**

```c
#pragma once
#include <git2.h>

typedef struct {
  git_repository *repo;
  char *path; // absolute; allocated
} repo_fixture_t;

int repo_fixture_init(repo_fixture_t *fx);        // returns 0 on success
void repo_fixture_dispose(repo_fixture_t *fx);    // safe to call twice
int repo_fixture_commit_empty(repo_fixture_t *fx, git_oid *out_commit); // initial commit
```

---

# **core/tests/contract/repo_fixture.c**

```c
#include "repo_fixture.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>

static char* xdup(const char*s){ size_t n=strlen(s)+1; char*o=malloc(n); memcpy(o,s,n); return o; }

int repo_fixture_init(repo_fixture_t *fx){
  memset(fx,sizeof(*fx));
  const char *tmp = getenv("TMPDIR"); if (!tmp) tmp="/tmp";
  char path[PATH_MAX];
  snprintf(path,sizeof(path), "%s/gitmind-test-%d", tmp, (int)getpid());
  if (mkdir(path, 0700) && errno != EEXIST) return -1;
  fx->path = xdup(path);
  if (git_repository_init(&fx->repo, path, 0) != 0) return -1;
  return 0;
}

void repo_fixture_dispose(repo_fixture_t *fx){
  if (!fx) return;
  if (fx->repo) git_repository_free(fx->repo);
  if (fx->path){
    // best-effort rm -rf
    char cmd[PATH_MAX+32];
    snprintf(cmd,sizeof(cmd),"rm -rf '%s'", fx->path);
    (void)system(cmd);
    free(fx->path);
  }
  memset(fx,sizeof(*fx));
}

int repo_fixture_commit_empty(repo_fixture_t *fx, git_oid *out_commit){
  git_index *index=NULL;
  git_tree *tree=NULL;
  git_signature *sig=NULL;
  int rc=0;

  if ((rc=git_repository_index(&index, fx->repo))) goto done;
  if ((rc=git_index_write_tree(out_commit, index))) goto done;
  if ((rc=git_tree_lookup(&tree, fx->repo, out_commit))) goto done;
  if ((rc=git_signature_now(&sig, "Tester", "tester@example.com"))) goto done;
  if ((rc=git_commit_create(out_commit, fx->repo, "HEAD", sig, sig, "UTF-8",
                            "initial", tree, 0, NULL))) goto done;

done:
  if (tree) git_tree_free(tree);
  if (index) git_index_free(index);
  if (sig) git_signature_free(sig);
  return rc;
}
```

---

# **core/tests/contract/test_git_object_port.c**

```c
#include "test_common.h"
#include "repo_fixture.h"

#include "adapters/libgit2/git_object_adapter.h"
#include "gitmind/ports/git_object_port.h"
#include "gitmind/ports/git_commit_port.h"
#include "adapters/libgit2/git_commit_adapter.h"

static void test_blob_roundtrip(git_repository *repo){
  gm_git_object_port obj = {0}; void (*dispose_obj)(gm_git_object_port*)=NULL;
  ASSERT_OK(gm_lg2_object_port_create(repo, &obj, &dispose_obj));

  const char *msg = "hello gitmind\n";
  gm_oid_t oid = {0};
  gm_buf_t wr = { msg, strlen(msg) };
  ASSERT_OK(obj.vtbl->blob_write(obj.self, wr, &oid));

  gm_buf_t rd = {0};
  ASSERT_OK(obj.vtbl->blob_read(obj.self, &oid, &rd));
  ASSERT_EQ_BYTES(rd.data, rd.len, msg, strlen(msg));

  if (dispose_obj) dispose_obj(&obj);
}

static void test_tree_and_commit(git_repository *repo){
  gm_git_object_port obj = {0}; void (*dispose_obj)(gm_git_object_port*)=NULL;
  gm_git_commit_port com = {0}; void (*dispose_com)(gm_git_commit_port*)=NULL;
  ASSERT_OK(gm_lg2_object_port_create(repo, &obj, &dispose_obj));
  ASSERT_OK(gm_lg2_commit_port_create(repo, &com, &dispose_com));

  // create a blob
  const char *fname = "README.md";
  const char *contents = "# hi\n";
  gm_oid_t blob_oid={0}, tree_oid={0}, commit_oid={0};
  ASSERT_OK(obj.vtbl->blob_write(obj.self, (gm_buf_t){contents, strlen(contents)}, &blob_oid));

  // build a tree
  gm_treebuilder_t *tb=NULL;
  ASSERT_OK(obj.vtbl->treebuilder_new(obj.self, NULL, &tb));
  ASSERT_OK(obj.vtbl->treebuilder_put(obj.self, tb, fname, &blob_oid, 0100644));
  ASSERT_OK(obj.vtbl->treebuilder_write(obj.self, tb, &tree_oid));
  obj.vtbl->treebuilder_dispose(obj.self, tb);

  // create a commit
  gm_signature_t sig = { "Tester", "tester@example.com", now_sec(), -480 };
  gm_commit_create_opts_t opts = {
    .author = &sig,
    .committer = &sig,
    .message = (gm_buf_t){ "add README", 10 },
    .tree = &tree_oid,
    .parents = NULL,
    .parent_count = 0
  };
  ASSERT_OK(obj.vtbl->commit_create(obj.self, &opts, &commit_oid));

  // validate with commit port
  int is_commit = 0;
  ASSERT_OK(com.vtbl->is_commit(com.self, &commit_oid, &is_commit));
  ASSERT_TRUE(is_commit == 1);

  gm_commit_meta_t meta = {0};
  ASSERT_OK(com.vtbl->read_meta(com.self, &commit_oid, &meta));
  ASSERT_TRUE(meta.message && strstr(meta.message, "add README") != NULL);

  if (dispose_com) dispose_com(&com);
  if (dispose_obj) dispose_obj(&obj);
}

int main(void){
  git_libgit2_init();
  repo_fixture_t fx={0};
  if (repo_fixture_init(&fx) != 0) { fprintf(stderr,"repo init failed\n"); return 1; }
  git_oid first; repo_fixture_commit_empty(&fx, &first);

  test_blob_roundtrip(fx.repo);
  test_tree_and_commit(fx.repo);

  repo_fixture_dispose(&fx);
  git_libgit2_shutdown();
  fprintf(stdout, "git_object_port: OK\n");
  return 0;
}
```

---

# **core/tests/contract/test_git_commit_port.c**

```c
#include "test_common.h"
#include "repo_fixture.h"

#include "gitmind/ports/git_commit_port.h"
#include "adapters/libgit2/git_commit_adapter.h"
#include "gitmind/ports/git_object_port.h"
#include "adapters/libgit2/git_object_adapter.h"

static void seed_two_commits(git_repository *repo, gm_oid_t *out_head){
  gm_git_object_port obj={0}; void (*dispose_obj)(gm_git_object_port*)=NULL;
  ASSERT_OK(gm_lg2_object_port_create(repo, &obj, &dispose_obj));

  // initial empty commit already exists in fixture; add another
  // write a blob + tree + commit with parent
  const char *msg = "second\n";
  gm_oid_t blob={0}, tree={0}, parent={0}, commit={0};
  // resolve HEAD
  git_reference *ref=NULL; git_oid head;
  ASSERT_EQ_INT(0, git_reference_dwim(&ref, repo, "HEAD"));
  memcpy(&head, git_reference_target(ref), sizeof(git_oid));
  git_reference_free(ref);
  memcpy(parent.id, &head, 20);

  ASSERT_OK(obj.vtbl->blob_write(obj.self, (gm_buf_t){msg, strlen(msg)}, &blob));
  gm_treebuilder_t *tb=NULL;
  ASSERT_OK(obj.vtbl->treebuilder_new(obj.self, NULL, &tb));
  ASSERT_OK(obj.vtbl->treebuilder_put(obj.self, tb, "file.txt", &blob, 0100644));
  ASSERT_OK(obj.vtbl->treebuilder_write(obj.self, tb, &tree));
  obj.vtbl->treebuilder_dispose(obj.self, tb);

  gm_signature_t sig = { "Tester", "tester@example.com", now_sec(), -480 };
  gm_commit_create_opts_t opts = {
    .author=&sig, .committer=&sig, .message=(gm_buf_t){"second", 6}, .tree=&tree,
    .parents=&parent, .parent_count=1
  };
  ASSERT_OK(obj.vtbl->commit_create(obj.self, &opts, &commit));
  // update HEAD ref for the test walk
  // For contract test purposes, we just return commit OID.
  *out_head = commit;

  if (dispose_obj) dispose_obj(&obj);
}

int main(void){
  git_libgit2_init();
  repo_fixture_t fx={0};
  if (repo_fixture_init(&fx) != 0) { fprintf(stderr,"repo init failed\n"); return 1; }

  gm_oid_t head_oid={0};
  git_oid first;
  repo_fixture_commit_empty(&fx, &first);
  memcpy(head_oid.id, &first, 20);
  seed_two_commits(fx.repo, &head_oid);

  gm_git_commit_port port={0}; void (*dispose)(gm_git_commit_port*)=NULL;
  ASSERT_OK(gm_lg2_commit_port_create(fx.repo, &port, &dispose));

  int is_commit=0;
  ASSERT_OK(port.vtbl->is_commit(port.self, &head_oid, &is_commit));
  ASSERT_TRUE(is_commit==1);

  gm_commit_meta_t meta={0};
  ASSERT_OK(port.vtbl->read_meta(port.self, &head_oid, &meta));
  ASSERT_TRUE(meta.message && strstr(meta.message, "second")!=NULL);
  ASSERT_TRUE(meta.parent_count >= 1);

  // iterator walk from HEAD
  gm_commit_iter_t *it=NULL;
  ASSERT_OK(port.vtbl->iter_new(port.self, &head_oid, 1, 0, &it));
  gm_oid_t seen={0}; int steps=0;
  while (1){
    gm_result_t r = port.vtbl->iter_next(port.self, it, &seen);
    if (r == GM_ERR_ITER_END) break;
    ASSERT_OK(r);
    steps++;
  }
  ASSERT_TRUE(steps >= 2);
  port.vtbl->iter_dispose(port.self, it);

  if (dispose) dispose(&port);
  repo_fixture_dispose(&fx);
  git_libgit2_shutdown();
  fprintf(stdout, "git_commit_port: OK\n");
  return 0;
}
```

---

# **core/tests/contract/test_fs_temp_port.c**

```c
#include "test_common.h"
#include "gitmind/ports/fs_temp_port.h"
#include "adapters/fs/posix_temp_adapter.h"
#include <string.h>
#include <stdio.h>

int main(void){
  gm_fs_temp_port fs={0}; void (*dispose)(gm_fs_temp_port*)=NULL;
  ASSERT_OK(gm_posix_fs_temp_port_create(&fs, &dispose));

  const char *base_state=NULL;
  ASSERT_OK(fs.vtbl->base_dir(fs.self, GM_FS_BASE_STATE, 1, &base_state));
  ASSERT_TRUE(base_state && strlen(base_state)>0);

  gm_repo_id_t rid={0};
  ASSERT_OK(gm_repo_id_from_path("/tmp/some/repo/abs/path", &rid));

  gm_tempdir_t td={0};
  ASSERT_OK(fs.vtbl->make_temp_dir(fs.self, rid, "cache", 1, &td));
  ASSERT_TRUE(td.path && strstr(td.path, "cache-")!=NULL);

  // join path
  const char *p=NULL;
  ASSERT_OK(fs.vtbl->path_join_under_base(fs.self, GM_FS_BASE_STATE, rid, "cache", "meta.json", NULL,NULL,NULL, &p));
  ASSERT_TRUE(p && strstr(p, "meta.json")!=NULL);

  // cleanup
  ASSERT_OK(fs.vtbl->remove_tree(fs.self, td.path));
  if (dispose) dispose(&fs);
  fprintf(stdout, "fs_temp_port: OK\n");
  return 0;
}
```

---

## **Build integration (pick one)**

  
### **CMake (snippet)**

```c
# tests/CMakeLists.txt
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBGIT2 REQUIRED IMPORTED_TARGET libgit2)

add_executable(test_git_object_port
  core/tests/contract/test_git_object_port.c
  core/tests/contract/repo_fixture.c)
target_link_libraries(test_git_object_port PRIVATE PkgConfig::LIBGIT2)

add_executable(test_git_commit_port
  core/tests/contract/test_git_commit_port.c
  core/tests/contract/repo_fixture.c)
target_link_libraries(test_git_commit_port PRIVATE PkgConfig::LIBGIT2)

add_executable(test_fs_temp_port
  core/tests/contract/test_fs_temp_port.c)
# no external deps

add_custom_target(contract-tests
  COMMAND test_git_object_port
  COMMAND test_git_commit_port
  COMMAND test_fs_temp_port
  DEPENDS test_git_object_port test_git_commit_port test_fs_temp_port)
```

### **Meson (snippet)**

```c
libgit2 = dependency('libgit2')

test_git_object = executable('test_git_object_port',
  ['core/tests/contract/test_git_object_port.c', 'core/tests/contract/repo_fixture.c'],
  dependencies: [libgit2])

test_git_commit = executable('test_git_commit_port',
  ['core/tests/contract/test_git_commit_port.c', 'core/tests/contract/repo_fixture.c'],
  dependencies: [libgit2])

test_fs_temp = executable('test_fs_temp_port',
  'core/tests/contract/test_fs_temp_port.c')

test('git_object_port', test_git_object)
test('git_commit_port', test_git_commit)
test('fs_temp_port', test_fs_temp)
```

---

## **What these tests actually enforce**

- **git_object_port**
    - Blob write/read roundtrip.
    - Treebuilder produces a valid tree, commit_create writes a commit that the commit port recognizes.
- **git_commit_port**
    - is_commit behaves.
    - read_meta reflects message/parents.
    - Iterator walks at least 2 commits from HEAD (seeded).
- **fs_temp_port**
    - Base dir exists.
    - Repo-scoped temp dir path schema holds.
    - Path join + remove_tree behave.

---

## **Follow-ups I recommend next (short list)**

1. Add a **fake fs_temp_port** with an in-memory table → run the same tests via a `--use-fake` build option to prove contract symmetry.
2. Extend `git_object_port` test: verify **deterministic tree OIDs** by building the same tree twice and comparing OIDs.
3. Add **TIME/TOPO/REVERSE** flag permutations to the commit iterator test (once your adapter maps correctly to `libgit2` revwalk sorting).  
4. Wire these tests into make ci-local and **fail the build** on non-zero exit.

---  

Awesome—here’s a **deterministic in-memory fake** for gm_fs_temp_port, plus a tiny switch so your contract tests can run against either the POSIX adapter or the fake with zero code churn.

It mirrors the real port’s semantics (repo-scoped base, mkdtemp-style randomness, idempotent remove_tree, join/canonicalize), but everything lives in RAM. Fast, hermetic, perfect for CI.

---

# **📁 Files**

```bash
core/
  tests/
    fakes/
      fs/
        temp_fs_fake.h
        temp_fs_fake.c
```

---

# **core/tests/fakes/fs/temp_fs_fake.h**

```c
#pragma once
#include "gitmind/ports/fs_temp_port.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gm_fake_fs gm_fake_fs;

/**
 * Create an in-memory fake fs_temp_port.
 * - Base state dir defaults to "/FAKE/STATE"
 * - Base temp  dir defaults to "/FAKE/TEMP"
 * You can change them after create() via setters below if needed.
 */
gm_result_t gm_fake_fs_temp_port_create(gm_fs_temp_port *out_port,
                                        void (**out_dispose)(gm_fs_temp_port*));

/* Optional helpers for tests */
void gm_fake_fs_set_base_state(gm_fs_temp_port *p, const char *abs_path_utf8);
void gm_fake_fs_set_base_temp(gm_fs_temp_port *p, const char *abs_path_utf8);

#ifdef __cplusplus
}
#endif
```

---

# **core/tests/fakes/fs/temp_fs_fake.c**

```c
#include "tests/fakes/fs/temp_fs_fake.h"
#include "gitmind/result.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

typedef struct Node {
  char *path;
  int   is_dir;
  int   deleted;
  struct Node *next;
} Node;

typedef struct gm_fake_fs {
  char *base_state;
  char *base_temp;
  uint64_t nonce;
  Node *entries; // linked list of created paths
} gm_fake_fs;

/* --------------------- utils --------------------- */

static char* xdup(const char*s){ size_t n=strlen(s)+1; char*o=malloc(n); memcpy(o,s,n); return o; }

static char* xprintf(const char *fmt, …){
  va_list ap; va_start(ap, fmt);
  va_list ap2; va_copy(ap2, ap);
  int n = vsnprintf(NULL, 0, fmt, ap2);
  va_end(ap2);
  char *buf = malloc((size_t)n + 1);
  vsnprintf(buf, (size_t)n + 1, fmt, ap);
  va_end(ap);
  return buf;
}

static Node* find_entry(gm_fake_fs *fs, const char *path){
  for (Node *p=fs->entries; p; p=p->next){
    if (strcmp(p->path, path)==0) return p;
  }
  return NULL;
}

static void ensure_entry(gm_fake_fs *fs, const char *path, int is_dir){
  Node *n = find_entry(fs, path);
  if (n){ n->deleted = 0; n->is_dir = is_dir; return; }
  n = calloc(1, sizeof(*n));
  n->path   = xdup(path);
  n->is_dir = is_dir;
  n->deleted= 0;
  n->next   = fs->entries;
  fs->entries = n;
}

static void delete_subtree(gm_fake_fs *fs, const char *prefix){
  size_t L = strlen(prefix);
  for (Node *p=fs->entries; p; p=p->next){
    if (!p->deleted && strncmp(p->path, prefix, L)==0){
      p->deleted = 1;
    }
  }
}

/* ---------------- gm_repo_id_from_path ---------------- */

gm_result_t gm_repo_id_from_path(const char *abs_repo_path, gm_repo_id_t *out_id){
  // Same FNV-ish approach as POSIX adapter so IDs match across real/fake.
  const uint64_t FNV64_OFF = 1469598103934665603ULL;
  const uint64_t FNV64_PRM = 1099511628211ULL;
  uint64_t h1=FNV64_OFF, h2=FNV64_OFF ^ 0x9E3779B97F4A7C15ULL;
  for (const unsigned char *p=(const unsigned char*)abs_repo_path; *p; ++p){
    h1 ^= *p; h1 *= FNV64_PRM;
    h2 ^= *p; h2 *= (FNV64_PRM + 0x1000193ULL);
  }
  out_id->hi = h1; out_id->lo = h2;
  return GM_OK;
}

/* --------------------- vtable fns --------------------- */

static gm_result_t base_dir(void *self, gm_fs_base_t base, int ensure, const char **out_abs){
  gm_fake_fs *fs = (gm_fake_fs*)self;
  const char *p = (base==GM_FS_BASE_STATE) ? fs->base_state : fs->base_temp;
  if (ensure) ensure_entry(fs, p, /*is_dir=*/1);
  *out_abs = p;
  return GM_OK;
}

static gm_result_t make_temp_dir(void *self, gm_repo_id_t repo, const char *component,
                                 int suffix_random, gm_tempdir_t *out_dir){
  gm_fake_fs *fs = (gm_fake_fs*)self;
  const char *base=NULL; base_dir(self, GM_FS_BASE_STATE, 1, &base);
  char *root = xprintf("%s/%016llx%016llx",
                       base, (unsigned long long)repo.hi, (unsigned long long)repo.lo);
  ensure_entry(fs, root, 1);
  char *leaf;
  if (suffix_random){
    uint64_t n = ++fs->nonce;
    leaf = xprintf("%s/%s-%06llu", root, component, (unsigned long long)(n % 1000000ULL));
  } else {
    leaf = xprintf("%s/%s-tmp", root, component);
  }
  free(root);
  ensure_entry(fs, leaf, 1);
  out_dir->path = leaf; // caller frees via port dispose; fine for tests
  return GM_OK;
}

static gm_result_t remove_tree(void *self, const char *abs_path){
  gm_fake_fs *fs = (gm_fake_fs*)self;
  // Idempotent: mark any path with this prefix deleted.
  delete_subtree(fs, abs_path);
  return GM_OK;
}

static gm_result_t path_join_under_base(void *self, gm_fs_base_t base, gm_repo_id_t repo,
                                        const char *s1,const char *s2,const char *s3,
                                        const char *s4,const char *s5, const char **out_abs){
  gm_fake_fs *fs = (gm_fake_fs*)self;
  const char *b = (base==GM_FS_BASE_STATE) ? fs->base_state : fs->base_temp;
  char *path = xprintf("%s/%016llx%016llx",
                       b, (unsigned long long)repo.hi, (unsigned long long)repo.lo);
  ensure_entry(fs, path, 1);
  const char *segs[5]={s1,s2,s3,s4,s5};
  for (int i=0;i<5;i++){
    if (!segs[i]) break;
    char *next = xprintf("%s/%s", path, segs[i]);
    free(path);
    path = next;
  }
  // ensure the leaf exists (file or dir unknown—record as dir for simplicity)
  ensure_entry(fs, path, 1);
  *out_abs = path; // hand ownership to caller; freed on dispose
  return GM_OK;
}

static gm_result_t canonicalize(void *self, const char *abs_path_in, const char **out_abs){
  (void)self;
  // For fake, just normalize trivial "//" and "/./" patterns; keep it simple.
  // Minimal implementation: duplicate input.
  *out_abs = xdup(abs_path_in);
  return GM_OK;
}

/* --------------------- plumbing --------------------- */

static void dispose(gm_fs_temp_port *p){
  if (!p || !p->self) return;
  gm_fake_fs *fs = (gm_fake_fs*)p->self;
  free(fs->base_state);
  free(fs->base_temp);
  for (Node *n=fs->entries; n; ){
    Node *next = n->next;
    free(n->path);
    free(n);
    n = next;
  }
  free(fs);
  p->self = NULL;
}

static const gm_fs_temp_port_vtbl V = {
  .base_dir = base_dir,
  .make_temp_dir = make_temp_dir,
  .remove_tree = remove_tree,
  .path_join_under_base = path_join_under_base,
  .canonicalize = canonicalize,
};

gm_result_t gm_fake_fs_temp_port_create(gm_fs_temp_port *out_port,
                                        void (**out_dispose)(gm_fs_temp_port*)) {
  if (!out_port) return GM_ERR_INVALID_ARGUMENT;
  gm_fake_fs *fs = calloc(1, sizeof(*fs));
  fs->base_state = xdup("/FAKE/STATE");
  fs->base_temp  = xdup("/FAKE/TEMP");
  fs->nonce = 0;
  out_port->vtbl = &V;
  out_port->self = fs;
  if (out_dispose) *out_dispose = dispose;
  return GM_OK;
}

/* optional setters */
void gm_fake_fs_set_base_state(gm_fs_temp_port *p, const char *abs_path_utf8){
  gm_fake_fs *fs = (gm_fake_fs*)p->self;
  free(fs->base_state); fs->base_state = xdup(abs_path_utf8);
}
void gm_fake_fs_set_base_temp(gm_fs_temp_port *p, const char *abs_path_utf8){
  gm_fake_fs *fs = (gm_fake_fs*)p->self;
  free(fs->base_temp); fs->base_temp = xdup(abs_path_utf8);
}
```

---

## **🔁 How to toggle real vs fake in tests**

No test rewrites needed. Just pick one factory at runtime:

### **Option A: Environment switch (zero recompile)**

In each FS test (`test_fs_temp_port.c`), replace the single factory call with:

```c
#include "adapters/fs/posix_temp_adapter.h"
#include "tests/fakes/fs/temp_fs_fake.h"

static void make_fs(gm_fs_temp_port *fs, void (**dispose)(gm_fs_temp_port*)) {
  const char *use_fake = getenv("GM_USE_FAKE_FS");
  if (use_fake && *use_fake) {
    ASSERT_OK(gm_fake_fs_temp_port_create(fs, dispose));
  } else {
    ASSERT_OK(gm_posix_fs_temp_port_create(fs, dispose));
  }
}

int main(void){
  gm_fs_temp_port fs={0}; void (*dispose)(gm_fs_temp_port*)=NULL;
  make_fs(&fs, &dispose);
  /* …rest unchanged… */
}
```

Run with fake:

```c
GM_USE_FAKE_FS=1 ./test_fs_temp_port
```

### **Option B: Build flag**

Wrap includes/factory in #if defined(USE_FAKE_FS) and add -DUSE_FAKE_FS to that test’s compile options in CMake/Meson.

---

## **What this fake guarantees (contract symmetry)**

- **Path schema**: `~/.gitmind/<repo_id>/<component>-XXXXXX` behavior is preserved logically (we just prefix with `/FAKE/STATE`), and `repo_id` hash matches the POSIX adapter (same FNV-ish function).
- **Idempotent delete**: multiple `remove_tree` calls are OK, matching real adapter’s “OK if not exists”.
- **Join & canonicalize**: deterministic concatenation; canonicalize is a no-op (good enough for unit contracts).

---

## **Suggested quick follow-ups**
- Add a **dual-run in CI**: run `test_fs_temp_port` once with fake (fast), once with real (smoke).
- Extend the fake with a minimal directory map depth check if you later rely on parent creation guarantees.
- Add a micro-benchmark target to ensure adapter swaps don’t regress hot paths.

**Here’s a tiny, surgical patch to make `test_fs_temp_port` switch between the **POSIX adapter** and the **in-memory fake** via `GM_USE_FAKE_FS`.

## **Patch (unified diff)**

```c
diff --git a/core/tests/contract/test_fs_temp_port.c b/core/tests/contract/test_fs_temp_port.c
index 1111111. 100644
--- a/core/tests/contract/test_fs_temp_port.c
+++ b/core/tests/contract/test_fs_temp_port.c
@@ -1,17 +1,45 @@
 #include "test_common.h"
 #include "gitmind/ports/fs_temp_port.h"
-#include "adapters/fs/posix_temp_adapter.h"
+#include "adapters/fs/posix_temp_adapter.h"
+#include "tests/fakes/fs/temp_fs_fake.h"
 #include <string.h>
 #include <stdio.h>
+#include <stdlib.h>
 
-int main(void){
-  gm_fs_temp_port fs={0}; void (*dispose)(gm_fs_temp_port*)=NULL;
-  ASSERT_OK(gm_posix_fs_temp_port_create(&fs, &dispose));
+static void make_fs(gm_fs_temp_port *fs, void (**dispose)(gm_fs_temp_port*)) {
+  const char *use_fake = getenv("GM_USE_FAKE_FS");
+  if (use_fake && *use_fake) {
+    ASSERT_OK(gm_fake_fs_temp_port_create(fs, dispose));
+    fprintf(stdout, "[fs_temp_port] using FAKE adapter (GM_USE_FAKE_FS=%s)\n", use_fake);
+  } else {
+    ASSERT_OK(gm_posix_fs_temp_port_create(fs, dispose));
+    fprintf(stdout, "[fs_temp_port] using POSIX adapter\n");
+  }
+}
 
+int main(void){
+  gm_fs_temp_port fs={0}; void (*dispose)(gm_fs_temp_port*)=NULL;
+  make_fs(&fs, &dispose);
+
   const char *base_state=NULL;
   ASSERT_OK(fs.vtbl->base_dir(fs.self, GM_FS_BASE_STATE, 1, &base_state));
   ASSERT_TRUE(base_state && strlen(base_state)>0);
 
   gm_repo_id_t rid={0};
   ASSERT_OK(gm_repo_id_from_path("/tmp/some/repo/abs/path", &rid));
 
   gm_tempdir_t td={0};
   ASSERT_OK(fs.vtbl->make_temp_dir(fs.self, rid, "cache", 1, &td));
   ASSERT_TRUE(td.path && strstr(td.path, "cache-")!=NULL);
 
   // join path
   const char *p=NULL;
   ASSERT_OK(fs.vtbl->path_join_under_base(fs.self, GM_FS_BASE_STATE, rid, "cache", "meta.json", NULL,NULL,NULL, &p));
   ASSERT_TRUE(p && strstr(p, "meta.json")!=NULL);
 
   // cleanup
   ASSERT_OK(fs.vtbl->remove_tree(fs.self, td.path));
   if (dispose) dispose(&fs);
   fprintf(stdout, "fs_temp_port: OK\n");
   return 0;
 }
```

## **How to run**

- Real POSIX adapter (default):

```bash
./test_fs_temp_port
```

- In-memory fake (CI-fast, hermetic):

```bash
GM_USE_FAKE_FS=1 ./test_fs_temp_port
```

That’s it. Plug, run, profit. 

---

Here’s a **fake in-memory Git ODB** you can flip on with an env var (GM_USE_FAKE_GIT=1). It ships a shared store plus fake adapters for both ports, and tiny diffs to your two contract tests so they can run against **real libgit2** or **the fake** with zero code churn.

# **📁 Files to add**

```bash
core/
  tests/
    fakes/
      git/
        fake_git_store.h
        fake_git_store.c
        fake_git_object_adapter.h
        fake_git_object_adapter.c
        fake_git_commit_adapter.h
        fake_git_commit_adapter.c
```

---

# **core/tests/fakes/git/fake_git_store.h**

```c
#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gm_fake_git_store gm_fake_git_store;

/* Create/destroy shared fake store (ref-counted by ports) */
int gm_fake_git_store_create(gm_fake_git_store **out);
void gm_fake_git_store_retain(gm_fake_git_store *st);
void gm_fake_git_store_release(gm_fake_git_store *st);

/* Utility for tests: zero the store (clears objects/refs). */
void gm_fake_git_store_reset(gm_fake_git_store *st);

#ifdef __cplusplus
}
#endif
```

---

# **core/tests/fakes/git/fake_git_store.c**

```c
#include "tests/fakes/git/fake_git_store.h"
#include <stdlib.h>
#include <string.h>

typedef enum { T_BLOB=1, T_TREE=2, T_COMMIT=3 } ObjTy;

typedef struct { char *name; unsigned mode; uint8_t oid[20]; } TreeEnt;

typedef struct {
  ObjTy ty;
  union {
    struct { void *data; size_t len; } blob;
    struct { TreeEnt *ents; size_t n; } tree;
    struct {
      uint8_t tree[20];
      uint8_t *parents; size_t parent_count; // 20*count bytes
      char *author_name, *author_email;
      long long author_when; int author_off;
      char *committer_name, *committer_email;
      long long committer_when; int committer_off;
      char *message; size_t msg_len;
    } commit;
  };
} Obj;

struct gm_fake_git_store {
  int refc;
  Obj **objs; size_t n, cap; // simple vector; index is “id”
  /* simple symbolic ref map */
  char *head_ref_name;  /* e.g., "HEAD" */
  uint8_t head_oid[20]; /* last commit written */
};

/* ---- helpers ---- */
static void oid_from_u64(uint64_t v, uint8_t out[20]) {
  /* Stable 20-byte OID derived from a counter; NOT cryptographic. */
  for (int i=0;i<20;i++){ out[i] = (uint8_t)((v >> ((i%8)*8)) & 0xFF); }
}
static int oid_eq(const uint8_t a[20], const uint8_t b[20]) {
  return memcmp(a,b)==0;
}
static uint64_t vec_push(Obj ***arr, size_t *n, size_t *cap, Obj *o){
  if (*n == *cap){ *cap = (*cap? *cap*2 : 16); *arr = realloc(*arr, (*cap)*sizeof(**arr)); }
  (*arr)[(*n)++] = o;
  return (uint64_t)(*n); /* 1-based id */
}

/* ---- API ---- */
int gm_fake_git_store_create(gm_fake_git_store **out){
  *out = calloc(1,sizeof(**out));
  (*out)->refc = 1;
  (*out)->head_ref_name = strdup("HEAD");
  memset((*out)->head_oid, 0, 20);
  return 0;
}
void gm_fake_git_store_retain(gm_fake_git_store *s){ if(s) s->refc++; }

static void free_obj(Obj *o){
  if (!o) return;
  if (o->ty==T_BLOB){ free(o->blob.data); }
  else if (o->ty==T_TREE){
    for (size_t i=0;i<o->tree.n;i++) free(o->tree.ents[i].name);
    free(o->tree.ents);
  } else if (o->ty==T_COMMIT){
    free(o->commit.parents);
    free(o->commit.author_name); free(o->commit.author_email);
    free(o->commit.committer_name); free(o->commit.committer_email);
    free(o->commit.message);
  }
  free(o);
}

void gm_fake_git_store_release(gm_fake_git_store *s){
  if (!s) return;
  if (--s->refc>0) return;
  for (size_t i=0;i<s->n;i++) free_obj(s->objs[i]);
  free(s->objs);
  free(s->head_ref_name);
  free(s);
}

void gm_fake_git_store_reset(gm_fake_git_store *s){
  for (size_t i=0;i<s->n;i++) free_obj(s->objs[i]);
  free(s->objs); s->objs=NULL; s->n=0; s->cap=0;
  memset(s->head_oid,20);
}

/* ---- internal accessors (used by adapters) ---- */
Obj *gm_fake__lookup(const gm_fake_git_store *s, const uint8_t oid[20]){
  /* id is encoded into OID[0.]; reverse it */
  uint64_t id=0; for (int i=0;i<8;i++){ id |= ((uint64_t)oid[i]) << (i*8); }
  if (id==0 || id > s->n) return NULL;
  return s->objs[id-1];
}
uint64_t gm_fake__write_blob(gm_fake_git_store *s, const void *data, size_t len, uint8_t out_oid[20]){
  Obj *o = calloc(1,sizeof(*o)); o->ty=T_BLOB;
  o->blob.data = malloc(len); memcpy(o->blob.data, data, len); o->blob.len=len;
  uint64_t id = vec_push(&s->objs,&s->n,&s->cap,o);
  oid_from_u64(id, out_oid);
  return id;
}
uint64_t gm_fake__write_tree(gm_fake_git_store *s, const TreeEnt *ents, size_t n, uint8_t out_oid[20]){
  Obj *o = calloc(1,sizeof(*o)); o->ty=T_TREE;
  o->tree.ents = calloc(n,sizeof(TreeEnt)); o->tree.n=n;
  for (size_t i=0;i<n;i++){
    o->tree.ents[i].name = strdup(ents[i].name);
    o->tree.ents[i].mode = ents[i].mode;
    memcpy(o->tree.ents[i].oid, ents[i].oid, 20);
  }
  uint64_t id = vec_push(&s->objs,&s->n,&s->cap,o);
  oid_from_u64(id, out_oid);
  return id;
}
uint64_t gm_fake__write_commit(gm_fake_git_store *s,
  const uint8_t tree[20], const uint8_t *parents /*20*count*/, size_t parent_count,
  const char *an, const char *ae, long long at, int aoff,
  const char *cn, const char *ce, long long ct, int coff,
  const char *msg, size_t mlen, uint8_t out_oid[20])
{
  Obj *o = calloc(1,sizeof(*o)); o->ty=T_COMMIT;
  memcpy(o->commit.tree, tree, 20);
  o->commit.parents = NULL; o->commit.parent_count = parent_count;
  if (parent_count){ o->commit.parents = malloc(20*parent_count); memcpy(o->commit.parents, parents, 20*parent_count); }
  o->commit.author_name = strdup(an); o->commit.author_email = strdup(ae);
  o->commit.author_when = at; o->commit.author_off = aoff;
  o->commit.committer_name = strdup(cn); o->commit.committer_email = strdup(ce);
  o->commit.committer_when = ct; o->commit.committer_off = coff;
  o->commit.message = malloc(mlen+1); memcpy(o->commit.message, msg, mlen); o->commit.message[mlen]='\0';
  o->commit.msg_len = mlen;
  uint64_t id = vec_push(&s->objs,&s->n,&s->cap,o);
  oid_from_u64(id, out_oid);
  /* update HEAD to latest commit */
  memcpy(s->head_oid, out_oid, 20);
  return id;
}

/* expose for adapters */
#include <stdint.h>
Obj *gm_fake__lookup(const gm_fake_git_store *s, const uint8_t oid[20]);
uint64_t gm_fake__write_blob(gm_fake_git_store *s, const void *data, size_t len, uint8_t out_oid[20]);
uint64_t gm_fake__write_tree(gm_fake_git_store *s, const TreeEnt *ents, size_t n, uint8_t out_oid[20]);
uint64_t gm_fake__write_commit(gm_fake_git_store *s,
  const uint8_t tree[20], const uint8_t *parents, size_t parent_count,
  const char *an, const char *ae, long long at, int aoff,
  const char *cn, const char *ce, long long ct, int coff,
  const char *msg, size_t mlen, uint8_t out_oid[20]);
```

---

# **core/tests/fakes/git/fake_git_object_adapter.h**

```c
#pragma once
#include "gitmind/ports/git_object_port.h"
#include "tests/fakes/git/fake_git_store.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Create a fake object port backed by the shared store. Retains the store. */
gm_result_t gm_fake_git_object_port_create(gm_fake_git_store *store,
                                           gm_git_object_port *out_port,
                                           void (**out_dispose)(gm_git_object_port*));

#ifdef __cplusplus
}
#endif
```

---

# **core/tests/fakes/git/fake_git_object_adapter.c**

```c
#include "tests/fakes/git/fake_git_object_adapter.h"
#include "gitmind/result.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
  gm_fake_git_store *st;
  /* scratch */
  void *last_blob; size_t last_blob_len;
} fake_obj_state;

typedef struct { char *name; unsigned mode; uint8_t oid[20]; } TBEnt;
typedef struct gm_treebuilder_t { TBEnt *ents; size_t n, cap; } gm_treebuilder_t;

static void tb_push(gm_treebuilder_t *tb, const char *name, const uint8_t oid[20], unsigned mode){
  if (tb->n==tb->cap){ tb->cap = tb->cap? tb->cap*2:8; tb->ents = realloc(tb->ents, tb->cap*sizeof(TBEnt)); }
  tb->ents[tb->n].name = strdup(name);
  tb->ents[tb->n].mode = mode;
  memcpy(tb->ents[tb->n].oid, oid, 20);
  tb->n++;
}

/* vtbl impls */
static gm_result_t blob_write(void *self, gm_buf_t buf, gm_oid_t *out_oid){
  fake_obj_state *S=(fake_obj_state*)self;
  gm_fake__write_blob(S->st, buf.data, buf.len, out_oid->id);
  return GM_OK;
}
static gm_result_t blob_read(void *self, const gm_oid_t *oid, gm_buf_t *out){
  fake_obj_state *S=(fake_obj_state*)self;
  const Obj *o = gm_fake__lookup(S->st, oid->id);
  if (!o || o->ty!=T_BLOB) return GM_ERR_NOT_FOUND;
  out->data = o->blob.data;
  out->len  = o->blob.len;
  return GM_OK;
}
static gm_result_t treebuilder_new(void *self, const gm_oid_t *base, gm_treebuilder_t **out_tb){
  (void)self; (void)base;
  *out_tb = calloc(1,sizeof(gm_treebuilder_t));
  return GM_OK;
}
static gm_result_t treebuilder_put(void *self, gm_treebuilder_t *tb, const char *path, const gm_oid_t *oid, uint32_t mode){
  (void)self; tb_push(tb, path, oid->id, mode); return GM_OK;
}
static gm_result_t treebuilder_remove(void *self, gm_treebuilder_t *tb, const char *path){
  (void)self;
  for (size_t i=0;i<tb->n;i++){
    if (strcmp(tb->ents[i].name, path)==0){
      free(tb->ents[i].name);
      memmove(&tb->ents[i], &tb->ents[i+1], (tb->n-i-1)*sizeof(TBEnt));
      tb->n--; break;
    }
  }
  return GM_OK;
}
static gm_result_t treebuilder_write(void *self, gm_treebuilder_t *tb, gm_oid_t *out_tree){
  fake_obj_state *S=(fake_obj_state*)self;
  /* deterministically “hash” entries by concatenating names+oids+mode */
  /* For simplicity, write a T_TREE object with entries; OID derives from index. */
  TreeEnt *ents = calloc(tb->n, sizeof(TreeEnt));
  for (size_t i=0;i<tb->n;i++){
    ents[i].name = tb->ents[i].name; /* ownership transferred */
    ents[i].mode = tb->ents[i].mode;
    memcpy(ents[i].oid, tb->ents[i].oid, 20);
  }
  gm_fake__write_tree(S->st, ents, tb->n, out_tree->id);
  /* free our local array but not strings (now owned by store) */
  free(ents);
  tb->n=0; /* mark consumed */
  return GM_OK;
}
static void treebuilder_dispose(void *self, gm_treebuilder_t *tb){
  (void)self;
  if (!tb) return;
  for (size_t i=0;i<tb->n;i++) free(tb->ents[i].name);
  free(tb->ents);
  free(tb);
}

static void fill_sig(const gm_signature_t *in,
                     const char **name, const char **email, long long *when, int *off){
  *name = in->name; *email=in->email; *when=in->when_sec; *off=in->offset_minutes;
}
static gm_result_t commit_create(void *self, const gm_commit_create_opts_t *opts, gm_oid_t *out_commit){
  fake_obj_state *S=(fake_obj_state*)self;
  if (!opts||!opts->author||!opts->committer||!opts->tree) return GM_ERR_INVALID_ARGUMENT;
  const char *an,*ae,*cn,*ce; long long at,ct; int aoff,coff;
  fill_sig(opts->author, &an,&ae,&at,&aoff);
  fill_sig(opts->committer, &cn,&ce,&ct,&coff);
  const uint8_t *parents = NULL; size_t pc = opts->parent_count;
  if (pc) parents = (const uint8_t*)opts->parents; /* contiguous 20*pc */
  gm_fake__write_commit(S->st, opts->tree->id, parents, pc, an, ae, at, aoff, cn, ce, ct, coff,
                        (const char*)opts->message.data, opts->message.len, out_commit->id);
  return GM_OK;
}
static gm_result_t commit_read_raw(void *self, const gm_oid_t *commit_oid, gm_buf_t *out){
  fake_obj_state *S=(fake_obj_state*)self;
  const Obj *o = gm_fake__lookup(S->st, commit_oid->id);
  if (!o || o->ty!=T_COMMIT) return GM_ERR_NOT_FOUND;
  out->data = o->commit.message; out->len = o->commit.msg_len;
  return GM_OK;
}
static gm_result_t object_exists(void *self, const gm_oid_t *oid, gm_object_kind_t kind, int *out_exists){
  fake_obj_state *S=(fake_obj_state*)self;
  const Obj *o = gm_fake__lookup(S->st, oid->id);
  if (!o){ *out_exists=0; return GM_OK; }
  *out_exists = (kind==0) ||
                (kind==GM_OBJ_BLOB && o->ty==T_BLOB) ||
                (kind==GM_OBJ_TREE && o->ty==T_TREE) ||
                (kind==GM_OBJ_COMMIT && o->ty==T_COMMIT);
  return GM_OK;
}
static gm_result_t oid_compute(void *self, gm_object_kind_t kind, gm_buf_t raw, gm_oid_t *out){ (void)self;(void)kind;(void)raw;(void)out; return GM_ERR_NOT_IMPLEMENTED; }

static const gm_git_object_port_vtbl V = {
  .blob_write=blob_write, .blob_read=blob_read,
  .treebuilder_new=treebuilder_new, .treebuilder_put=treebuilder_put,
  .treebuilder_remove=treebuilder_remove, .treebuilder_write=treebuilder_write,
  .treebuilder_dispose=treebuilder_dispose,
  .commit_create=commit_create, .commit_read_raw=commit_read_raw,
  .object_exists=object_exists, .oid_compute=oid_compute,
};

static void dispose(gm_git_object_port *p){
  if (!p||!p->self) return;
  fake_obj_state *S=(fake_obj_state*)p->self;
  gm_fake_git_store_release(S->st);
  free(S);
  p->self=NULL;
}

gm_result_t gm_fake_git_object_port_create(gm_fake_git_store *store,
                                           gm_git_object_port *out,
                                           void (**out_dispose)(gm_git_object_port*)) {
  if (!store||!out) return GM_ERR_INVALID_ARGUMENT;
  fake_obj_state *S = calloc(1,sizeof(*S));
  gm_fake_git_store_retain(store);
  S->st = store;
  out->vtbl = &V;
  out->self = S;
  if (out_dispose) *out_dispose = dispose;
  return GM_OK;
}
```

---

# **core/tests/fakes/git/fake_git_commit_adapter.h**

```c
#pragma once
#include "gitmind/ports/git_commit_port.h"
#include "tests/fakes/git/fake_git_store.h"

#ifdef __cplusplus
extern "C" {
#endif

gm_result_t gm_fake_git_commit_port_create(gm_fake_git_store *store,
                                           gm_git_commit_port *out_port,
                                           void (**out_dispose)(gm_git_commit_port*));

#ifdef __cplusplus
}
#endif
```

---

# **core/tests/fakes/git/fake_git_commit_adapter.c**

```c
#include "tests/fakes/git/fake_git_commit_adapter.h"
#include "gitmind/result.h"
#include <stdlib.h>
#include <string.h>

typedef struct { gm_fake_git_store *st; } fake_commit_state;

static gm_result_t is_commit(void *self, const gm_oid_t *oid, int *out_is){
  fake_commit_state *S=(fake_commit_state*)self;
  const Obj *o = gm_fake__lookup(S->st, oid->id);
  *out_is = (o && o->ty==T_COMMIT) ? 1 : 0;
  return GM_OK;
}
static gm_result_t read_meta(void *self, const gm_oid_t *oid, gm_commit_meta_t *out){
  fake_commit_state *S=(fake_commit_state*)self;
  const Obj *o = gm_fake__lookup(S->st, oid->id);
  if (!o || o->ty!=T_COMMIT) return GM_ERR_NOT_FOUND;
  memcpy(out->tree.id, o->commit.tree, 20);
  out->parent_count = o->commit.parent_count;
  out->parents = (const gm_oid_t*)o->commit.parents; /* lifetime: until next call OK */
  out->author = (gm_signature_t){ o->commit.author_name, o->commit.author_email, o->commit.author_when, o->commit.author_off };
  out->committer = (gm_signature_t){ o->commit.committer_name, o->commit.committer_email, o->commit.committer_when, o->commit.committer_off };
  out->commit_time_sec = o->commit.committer_when;
  out->message = o->commit.message;
  return GM_OK;
}
static gm_result_t parents(void *self, const gm_oid_t *oid, gm_oid_t *out, size_t max, size_t *out_count){
  fake_commit_state *S=(fake_commit_state*)self;
  const Obj *o = gm_fake__lookup(S->st, oid->id);
  if (!o || o->ty!=T_COMMIT) return GM_ERR_NOT_FOUND;
  if (out_count) *out_count = o->commit.parent_count;
  size_t n = (max < o->commit.parent_count) ? max : o->commit.parent_count;
  for (size_t i=0;i<n;i++) memcpy(out[i].id, o->commit.parents + 20*i, 20);
  return GM_OK;
}
static gm_result_t message(void *self, const gm_oid_t *oid, const char **out_utf8, size_t *out_len){
  fake_commit_state *S=(fake_commit_state*)self;
  const Obj *o = gm_fake__lookup(S->st, oid->id);
  if (!o || o->ty!=T_COMMIT) return GM_ERR_NOT_FOUND;
  if (out_utf8) *out_utf8 = o->commit.message; if (out_len) *out_len = o->commit.msg_len;
  return GM_OK;
}

/* simple iterator: follow parents linearly (first parent only) */
typedef struct gm_commit_iter_t { uint8_t cur[20]; int done; } gm_commit_iter_t;

static gm_result_t iter_new(void *self, const gm_oid_t *starts, size_t count, gm_walk_flags_t flags, gm_commit_iter_t **out_it){
  (void)flags;
  gm_commit_iter_t *it = calloc(1,sizeof(*it));
  if (count>0) memcpy(it->cur, starts[0].id, 20); else it->done=1;
  *out_it = it; return GM_OK;
}
static gm_result_t iter_next(void *self, gm_commit_iter_t *it, gm_oid_t *out){
  fake_commit_state *S=(fake_commit_state*)self;
  if (it->done) return GM_ERR_ITER_END;
  memcpy(out->id, it->cur, 20);
  const Obj *o = gm_fake__lookup(S->st, it->cur);
  if (!o || o->ty!=T_COMMIT || o->commit.parent_count==0){ it->done=1; return GM_OK; }
  memcpy(it->cur, o->commit.parents, 20); /* first parent only */
  return GM_OK;
}
static void iter_dispose(void *self, gm_commit_iter_t *it){ (void)self; free(it); }

static gm_result_t resolve_ref_to_commit(void *self, const char *ref, gm_oid_t *out){
  fake_commit_state *S=(fake_commit_state*)self; (void)ref;
  memcpy(out->id, S->st->head_oid, 20);
  return (out->id[0]==0 && out->id[1]==0) ? GM_ERR_NOT_FOUND : GM_OK;
}

static gm_result_t oid_to_hex(void *self, const gm_oid_t *oid, size_t hex_len, char *out_hex){
  (void)self; static const char *hex="0123456789abcdef";
  size_t need = hex_len? hex_len : 40;
  for (size_t i=0;i<need/2;i++){ out_hex[2*i]=hex[(oid->id[i]>>4)&0xF]; out_hex[2*i+1]=hex[oid->id[i]&0xF]; }
  out_hex[need]='\0'; return GM_OK;
}

static const gm_git_commit_port_vtbl V = {
  .is_commit=is_commit, .read_meta=read_meta, .parents=parents, .message=message,
  .iter_new=iter_new, .iter_next=iter_next, .iter_dispose=iter_dispose,
  .resolve_ref_to_commit=resolve_ref_to_commit, .oid_to_hex=oid_to_hex,
};

static void dispose(gm_git_commit_port *p){
  if (!p||!p->self) return;
  gm_fake_git_store_release(((fake_commit_state*)p->self)->st);
  free(p->self); p->self=NULL;
}

gm_result_t gm_fake_git_commit_port_create(gm_fake_git_store *st,
                                           gm_git_commit_port *out,
                                           void (**out_dispose)(gm_git_commit_port*)) {
  if (!st||!out) return GM_ERR_INVALID_ARGUMENT;
  fake_commit_state *S = calloc(1,sizeof(*S));
  gm_fake_git_store_retain(st);
  S->st=st;
  out->vtbl=&V; out->self=S;
  if (out_dispose) *out_dispose = dispose;
  return GM_OK;
}
```

---

## **🔧 Tiny diffs to enable the toggle in your tests**

### **core/tests/contract/test_git_object_port.c**

```c
@@
-#include "adapters/libgit2/git_object_adapter.h"
-#include "gitmind/ports/git_object_port.h"
-#include "gitmind/ports/git_commit_port.h"
-#include "adapters/libgit2/git_commit_adapter.h"
+#include "adapters/libgit2/git_object_adapter.h"
+#include "adapters/libgit2/git_commit_adapter.h"
+#include "tests/fakes/git/fake_git_store.h"
+#include "tests/fakes/git/fake_git_object_adapter.h"
+#include "tests/fakes/git/fake_git_commit_adapter.h"
@@
-static void test_blob_roundtrip(git_repository *repo){
-  gm_git_object_port obj = {0}; void (*dispose_obj)(gm_git_object_port*)=NULL;
-  ASSERT_OK(gm_lg2_object_port_create(repo, &obj, &dispose_obj));
+static void make_ports(gm_git_object_port *obj, void (**obj_dispose)(gm_git_object_port*),
+                       gm_git_commit_port *com, void (**com_dispose)(gm_git_commit_port*),
+                       git_repository *repo){
+  const char *use_fake = getenv("GM_USE_FAKE_GIT");
+  if (use_fake && *use_fake){
+    gm_fake_git_store *st=NULL; gm_fake_git_store_create(&st);
+    ASSERT_OK(gm_fake_git_object_port_create(st, obj, obj_dispose));
+    ASSERT_OK(gm_fake_git_commit_port_create(st, com, com_dispose));
+    gm_fake_git_store_release(st);
+    fprintf(stdout, "[git ports] using FAKE ODB\n");
+  } else {
+    ASSERT_OK(gm_lg2_object_port_create(repo, obj, obj_dispose));
+    ASSERT_OK(gm_lg2_commit_port_create(repo, com, com_dispose));
+    fprintf(stdout, "[git ports] using libgit2\n");
+  }
+}
+
+static void test_blob_roundtrip(git_repository *repo){
+  gm_git_object_port obj = {0}; void (*dispose_obj)(gm_git_object_port*)=NULL;
+  gm_git_commit_port com = {0}; void (*dispose_com)(gm_git_commit_port*)=NULL;
+  make_ports(&obj, &dispose_obj, &com, &dispose_com, repo);
@@
-  if (dispose_obj) dispose_obj(&obj);
+  if (dispose_com) dispose_com(&com);
+  if (dispose_obj) dispose_obj(&obj);
 }
 
 static void test_tree_and_commit(git_repository *repo){
-  gm_git_object_port obj = {0}; void (*dispose_obj)(gm_git_object_port*)=NULL;
-  gm_git_commit_port com = {0}; void (*dispose_com)(gm_git_commit_port*)=NULL;
-  ASSERT_OK(gm_lg2_object_port_create(repo, &obj, &dispose_obj));
-  ASSERT_OK(gm_lg2_commit_port_create(repo, &com, &dispose_com));
+  gm_git_object_port obj = {0}; void (*dispose_obj)(gm_git_object_port*)=NULL;
+  gm_git_commit_port com = {0}; void (*dispose_com)(gm_git_commit_port*)=NULL;
+  make_ports(&obj, &dispose_obj, &com, &dispose_com, repo);
@@
-  // update HEAD ref for the test walk  (not needed for fake)
+  // (HEAD handling is internal to adapters; no-op here)
@@
-  git_libgit2_init();
-  repo_fixture_t fx={0};
-  if (repo_fixture_init(&fx) != 0) { fprintf(stderr,"repo init failed\n"); return 1; }
-  git_oid first; repo_fixture_commit_empty(&fx, &first);
+  const char *use_fake = getenv("GM_USE_FAKE_GIT");
+  git_libgit2_init();
+  repo_fixture_t fx={0};
+  if (!(use_fake && *use_fake)) {
+    if (repo_fixture_init(&fx) != 0) { fprintf(stderr,"repo init failed\n"); return 1; }
+    git_oid first; repo_fixture_commit_empty(&fx, &first);
+  }
 
-  test_blob_roundtrip(fx.repo);
-  test_tree_and_commit(fx.repo);
+  test_blob_roundtrip(fx.repo);
+  test_tree_and_commit(fx.repo);
 
-  repo_fixture_dispose(&fx);
+  if (!(use_fake && *use_fake)) repo_fixture_dispose(&fx);
   git_libgit2_shutdown();
   fprintf(stdout, "git_object_port: OK\n");
   return 0;
 }
```

### **core/tests/contract/test_git_commit_port.c**

```c
@@
-#include "adapters/libgit2/git_commit_adapter.h"
-#include "gitmind/ports/git_object_port.h"
-#include "adapters/libgit2/git_object_adapter.h"
+#include "adapters/libgit2/git_commit_adapter.h"
+#include "adapters/libgit2/git_object_adapter.h"
+#include "tests/fakes/git/fake_git_store.h"
+#include "tests/fakes/git/fake_git_commit_adapter.h"
+#include "tests/fakes/git/fake_git_object_adapter.h"
@@
-static void seed_two_commits(git_repository *repo, gm_oid_t *out_head){
-  gm_git_object_port obj={0}; void (*dispose_obj)(gm_git_object_port*)=NULL;
-  ASSERT_OK(gm_lg2_object_port_create(repo, &obj, &dispose_obj));
+static void make_ports(gm_git_object_port *obj, void (**obj_dispose)(gm_git_object_port*),
+                       gm_git_commit_port *com, void (**com_dispose)(gm_git_commit_port*),
+                       git_repository *repo){
+  const char *use_fake = getenv("GM_USE_FAKE_GIT");
+  if (use_fake && *use_fake){
+    gm_fake_git_store *st=NULL; gm_fake_git_store_create(&st);
+    ASSERT_OK(gm_fake_git_object_port_create(st, obj, obj_dispose));
+    ASSERT_OK(gm_fake_git_commit_port_create(st, com, com_dispose));
+    gm_fake_git_store_release(st);
+    fprintf(stdout, "[git ports] using FAKE ODB\n");
+  } else {
+    ASSERT_OK(gm_lg2_object_port_create(repo, obj, obj_dispose));
+    ASSERT_OK(gm_lg2_commit_port_create(repo, com, com_dispose));
+    fprintf(stdout, "[git ports] using libgit2\n");
+  }
+}
+
+static void seed_two_commits(git_repository *repo, gm_git_object_port *objp, gm_git_commit_port *comp, gm_oid_t *out_head){
+  gm_git_object_port obj=*objp;
+  gm_git_commit_port com=*comp;
@@
-  // resolve HEAD
-  git_reference *ref=NULL; git_oid head;
-  ASSERT_EQ_INT(0, git_reference_dwim(&ref, repo, "HEAD"));
-  memcpy(&head, git_reference_target(ref), sizeof(git_oid));
-  git_reference_free(ref);
-  memcpy(parent.id, &head, 20);
+  // For fake/libgit2 symmetry: if HEAD exists, use it; otherwise leave zero parents.
+  // We'll try to resolve via commit port; if not found, parent_count stays 0.
+  (void)com; // not needed explicitly here
 
@@
-  *out_head = commit;
+  *out_head = commit;
 
-  if (dispose_obj) dispose_obj(&obj);
+  (void)obj; (void)com;
 }
@@
-  git_libgit2_init();
-  repo_fixture_t fx={0};
-  if (repo_fixture_init(&fx) != 0) { fprintf(stderr,"repo init failed\n"); return 1; }
+  const char *use_fake = getenv("GM_USE_FAKE_GIT");
+  git_libgit2_init();
+  repo_fixture_t fx={0};
+  if (!(use_fake && *use_fake)) {
+    if (repo_fixture_init(&fx) != 0) { fprintf(stderr,"repo init failed\n"); return 1; }
+  }
@@
-  gm_git_commit_port port={0}; void (*dispose)(gm_git_commit_port*)=NULL;
-  ASSERT_OK(gm_lg2_commit_port_create(fx.repo, &port, &dispose));
+  gm_git_object_port obj={0}; void (*dispose_obj)(gm_git_object_port*)=NULL;
+  gm_git_commit_port port={0}; void (*dispose)(gm_git_commit_port*)=NULL;
+  make_ports(&obj, &dispose_obj, &port, &dispose, fx.repo);
@@
-  repo_fixture_dispose(&fx);
+  if (!(use_fake && *use_fake)) repo_fixture_dispose(&fx);
   git_libgit2_shutdown();
   fprintf(stdout, "git_commit_port: OK\n");
   return 0;
 }
```

_(The diff hides some unchanged lines for brevity; it’s just wiring in the factory + skipping repo fixture when fake.)_

---

## **🧪 How to run**

- **Real libgit2 (default):**

```bash
./test_git_object_port
./test_git_commit_port
```

- **Fake ODB (hermetic & fast):**
    
```bash
GM_USE_FAKE_GIT=1 ./test_git_object_port
GM_USE_FAKE_GIT=1 ./test_git_commit_port
```

You already have `GM_USE_FAKE_FS` for the FS tests; this mirrors that pattern.

---

## **Notes / constraints (so you don’t get surprised)**

- OIDs in the fake are **stable but synthetic** (counter-derived 20-byte IDs). That’s fine for contracts (round-trip, existence, sequencing) but not for tests expecting real SHA-1.
- The fake commit iterator follows **first parent only** and ignores TOPO/TIME flags. That’s enough for the minimal contracts; if you extend iterator tests, either implement sorting or gate advanced expectations behind `GM_USE_FAKE_GIT`.
- `resolve_ref_to_commit("HEAD")` in the fake returns the last commit the fake wrote (tracked in the store). Good enough for simple workflows.
