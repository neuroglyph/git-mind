---
title: Technical Decision — Memory Allocation Strategy
description: Formal decision, requirements, and design for GitMind’s memory allocation & ownership model. Includes enforcement & migration plan.
audience: [contributors, maintainers]
domain: [architecture]
tags: [memory, hexagonal-architecture, testing, reliability, performance]
status: accepted
last_updated: 2025-10-08
---
<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->
<!-- © 2025 J. Kirby Ross / Neuroglyph Collective -->

# Memory Allocation Strategies

## **1) Decision Summary (The Short, Sharp Bit)**

- **Allocator is a port** injected via gm_context_t (no raw malloc in domain/services). Default wraps system malloc; tests can swap in arenas/fakes/tracers.
- **Operation-scoped arenas** for bursty, short-lived allocations (e.g., cache rebuilds, attribution scans, journal decodes).
- **Small-object slabs** for hot bins (32/64/128/256 bytes) used by errors, telemetry kv pairs, tiny edge pods.
- **String & OID interning** (service lifetime) with refcounted handles; pointer equality implies value equality.
- **Explicit ownership protocol** via suffixes: _owned (caller frees), _borrowed (callee owns), _view (slice; parent controls lifetime). Ports that return heap also ship a *_free or a result_ptr{ptr,dtor}.
- **Allocation telemetry**: bytes outstanding, peak bytes, alloc failure count, per-operation tags; budgets enforced in CI.

These choices give us predictably low latency, bounded peaks, easy failure injection, and contributor-friendly lifetimes—without a language rewrite.

## **2) Why These Choices (and Why Not the Others)**

### **2.1 Allocator as a Port**

**Why:** enables testable failure modes (Nth-alloc OOM), swap-in backends (jemalloc/tracing), and keeps memory in the infrastructure ring with FS/Logger/Metrics.

**Not chosen:** raw malloc (un-auditable), global singleton allocators (hidden state/test bleed), “just jemalloc globally” (helps fragmentation, not lifetimes or tests).

### **2.2 Operation-Scoped Arenas**

**Why:** O(1) teardown, near-zero fragmentation, tight cache locality for big scans and rebuilds.

**Not chosen:** hand-free everything (brittle), RC/GC (complex/pausy), TLS scratch everywhere (hidden state; test pain).

### **2.3 Small-Object Slabs**

**Why:** tiny hot structs dominate counts; slab bins de-jitter latency and reduce fragmentation.

**Not chosen:** buddy allocators (page-oriented; poor for sub-256B), per-type freelists (surface-area explosion).

### **2.4 Interning (paths, branch names, SHAs/OIDs)**

**Why:** dedup by construction, identity semantics, fewer long-lived duplicates.

**Not chosen:** global tables (contention), rope/rope-like strings (overhead; slicing rules everywhere).

### **2.5 Ownership Protocol + result_ptr**

**Why:** reviewable lifetimes by name; doc-lintable; portable C idiom.

**Not chosen:** “we’ll document it” (people forget), pervasive smart-pointer emulation (hidden costs; boilerplate).

### **2.6 Alloc Telemetry + Budgets**

**Why:** measurable, enforceable targets; catch regressions early; op-scoped attribution.

**Not chosen:** profiler-only culture (not CI-able), ad-hoc logs (noisy, unstructured).

> Alternative path we _will_ benchmark for due diligence: **jemalloc under the port + ownership protocol, no arenas**. Expect we’ll still adopt arenas for rebuild/scan flows due to O(1) teardown wins.

## **3) Requirements (Normative)**

### **3.1 Allocator Port**

- Define `gm_allocator_port_t` with `alloc`/`realloc`/`free`, `ctx` cookie, optional diagnostics: `bytes_outstanding()`, `peak_bytes()`, `failures_total()`.
- Expose `gm_xalloc(alloc,n)`, `gm_xcalloc`, `gm_xrealloc`, `gm_xfree` wrappers.
- `gm_context_t` must carry an allocator instance; all service/adapter code uses it (no raw `malloc`/`free`).

#### **Header (new)**

- `gitmind/ports/allocator_port.h` — public port
- `gitmind/util/alloc_wrappers.h` — inline helpers

### **3.2 Arenas**

- `gm_arena_begin(alloc, hint_bytes)` -> `gm_arena_t*`, `gm_arena_alloc(arena, size, align)`, `gm_arena_end(arena)`.
- Growth: geometric (×2) until cap; fail with `gm_result_*` on cap hit.
- **No per-object frees**; objects from arenas must not escape operation scope.
- **Defaults** (tunable via config/env):
    - `ARENA_HINT_CACHE_REBUILD=8 MiB`, `ARENA_CAP_CACHE_REBUILD=256 MiB`
    - `ARENA_HINT_ATTR_QUERY=4 MiB`, `ARENA_CAP_ATTR_QUERY=128 MiB`

#### **Header (new)**

- `gitmind/util/arena.h`

### **3.3 Small-Object Slabs**

- Bins: 32/64/128/256 bytes (aligned); superblock = 64 KiB; return whole superblock when fully free.
- Public API opaque; consumers call `gm_slab_alloc(size)`; sizes outside bins fall back to base allocator.

#### **Header (new)**

- `gitmind/util/slab.h`

### **3.4 Intern Tables**

- Tables per service (e.g., `cache`, `journal`): `gm_intern_table_t* gm_intern_table_new(alloc)`.
- `gm_intern_acquire(tbl, "string", &out_handle)`, `gm_intern_retain`, `gm_intern_release`.
- Expose `gm_intern_ptr(handle)`; pointer validity tied to table lifetime.

#### **Header (new)**

- `gitmind/util/intern.h`

### **3.5 Ownership Protocol**

- Function return types carrying heap must either:
    - Return _owned pointers with a matching *_free, **or**
    - Return `gm_result_ptr_t { void* ptr; void (*destroy)(void*); }`.
- Borrowed and view lifetimes must be suffix-annotated in the symbol name and documented in headers.
- CI doc-lint enforces suffix rules and presence of destroyer.

### **3.6 Telemetry**

- Allocator exposes metrics; services tag ops: `cache.rebuild`, `attr.query`, `journal.read`.
- Emit per-op: `alloc.peak_bytes`, `alloc.failures_total`, `arena.grow.bytes (sampled)`.
- Budgets enforced in CI by operation type (see §6).

## **4) Design (Informative, but specific)**

## **4.1 Interfaces**

```c
// ports/allocator_port.h
typedef void* (*gm_alloc_fn)(void* ctx, size_t n);
typedef void* (*gm_realloc_fn)(void* ctx, void* p, size_t n);
typedef void  (*gm_free_fn)(void* ctx, void* p);

typedef struct {
  void*           ctx;
  gm_alloc_fn     alloc;
  gm_realloc_fn   realloc;
  gm_free_fn      free;
  // optional diagnostics:
  uint64_t      (*bytes_outstanding)(void* ctx);
  uint64_t      (*peak_bytes)(void* ctx);
  uint64_t      (*failures_total)(void* ctx);
} gm_allocator_port_t;

typedef struct {
  void*  ptr;
  void (*destroy)(void*);
} gm_result_ptr_t;
```

```c
// util/arena.h (sketch)
typedef struct gm_arena gm_arena_t;
gm_arena_t* gm_arena_begin(const gm_allocator_port_t* alloc, size_t hint, size_t cap);
void*       gm_arena_alloc(gm_arena_t* a, size_t size, size_t align);
void        gm_arena_reset(gm_arena_t* a); // optional reuse
void        gm_arena_end(gm_arena_t* a);
```

```c
// util/slab.h (sketch)
void* gm_slab_alloc(const gm_allocator_port_t* alloc, size_t size);
void  gm_slab_free (const gm_allocator_port_t* alloc, void* p, size_t size);
```

```c
// util/intern.h (sketch)
typedef struct gm_intern_handle { uint32_t id; } gm_intern_handle_t;
typedef struct gm_intern_table gm_intern_table_t;

gm_intern_table_t* gm_intern_table_new(const gm_allocator_port_t* alloc);
void               gm_intern_table_free(gm_intern_table_t* t);

gm_result_t gm_intern_acquire(gm_intern_table_t* t, const char* s, gm_intern_handle_t* out);
void        gm_intern_retain (gm_intern_table_t* t, gm_intern_handle_t h);
void        gm_intern_release(gm_intern_table_t* t, gm_intern_handle_t h);
const char* gm_intern_ptr    (const gm_intern_table_t* t, gm_intern_handle_t h);
```

### **4.2 Integration Points (current code)**

- **Temp FS adapters** (`posix_temp_adapter.c`, `fake_fs_temp_port.c`) must consume allocator port and return _owned canonical paths with explicit *_free.
- **Cache rebuild service** (`cache_rebuild_service.c`): wrap main routine in `gm_arena_begin`/`end`; move temp buffers and decoded structs into arena.
- **Reader/Writer** (`reader.c`, `writer.c`): journal decode/encode scratch → arena; small error payloads & log kvs → slab; stable names (paths/refs) → intern table.
  
### **4.3 Threading & Concurrency**

- No global allocators; allocator lives in `gm_context_t`.
- Arenas are per-operation; never shared across threads.
- Intern tables are per service; guard with fine-grained locks or sharded tables if contention appears (future enhancement).

### **4.4 Security & Safety**

- Maintain `gm_snprintf`/`gm_strcpy_safe` policy. Treat truncation as error.
- Zero sensitive buffers on free for security-critical payloads (journal secrets, keys).
- Never allocate within signal handlers; never free across allocator instances.

## **5) Enforcement (Tooling & CI)**

### **5.1 Linting & Static Checks**

- **Forbidden**: direct `malloc`/`calloc`/`realloc`/`free` in non-allocator units (clang-tidy pattern-list + CI grep).
- **Header policy**: if a function returns `*` and includes _owned in its name, linter checks for a sibling *_free symbol.
- **Suffix check**: exported symbols returning views must end with _view; borrowed refs with _borrowed.

### **5.2 Test Fakes & Harness**

- **OOM-after-N allocator**: configurable via env (`GM_TEST_OOM_N`); used in unit and integration tests across services.
- **Tracing allocator**: asserts `bytes_outstanding()==0` at test end; CI fails on leaks.
- **Concurrency smoke**: run parallel cache rebuilds using separate arenas; assert no cross-talk, record per-op peaks.

### **5.3 Budgets in CI**

- Cache rebuild: `peak_bytes <= 256 MiB` (warn at 80%, fail at 100%).
- Attribution scan (standard repo fixture): `peak_bytes <= 128 MiB`.
- Any alloc failure count > 0 outside OOM tests => fail.  
> Budgets are recorded per op name; telemetry exporter already exists—wire allocator metrics to it and gate in CI.

## **6) Migration Plan (Three Focused PRs)**

### **PR-1: Allocator Port + Wiring (Infrastructure)**

- Add `allocator_port.h`/`alloc_wrappers.h`; inject into `gm_context_t`.
- Convert temp FS code paths to use the port; return _owned buffers + gm_fs_*_free.
- CI: add forbidden-API check for raw malloc/free.

### **PR-2: Arenas in Cache Rebuild (Service)**

- Wrap rebuild service call chain with `gm_arena_begin`/`end`.
- Move temp structs/buffers to `gm_arena_alloc`.
- Emit `alloc.peak_bytes` per rebuild; add CI budget check.

### **PR-3: Slabs + Interning (Cross-cutting)**

- Introduce `slab.h` and route `gm_error_t`, telemetry `kv` structs, and other ≤256B pods.
- Add `intern.h` and replace repeated path/branch/OID strings with interned handles in hot services.
- Add histogram instrumentation for alloc sizes; tune bins in follow-up PR.

## **7) Alternatives Considered (and Rejected)**

- **Rust rewrite now:** wins ownership at compile time; loses calendar. Keep this design to ease a future Rust module boundary. Plus, this is a C project. Get outta here!
- **`jemalloc` everywhere only:** keep it under the port as a default backend; still need arenas/ownership.
- **Conservative GC:** unpredictable pauses; pointer ambiguity; not CI-friendly.
- **TLS scratch allocators:** tempting latency win; encourages hidden state; hard to test/trace; we’ll use arenas instead.

## **8) Operational Guidance (Daily Use)**

- Domain ring: avoid heap entirely.
- If you return heap, you **must** expose a destroyer and annotate ownership in the name.
- Anything outliving an operation must **not** come from an arena (duplicate to service allocator or intern it).
- On error, **one cleanup: path** frees in reverse order; owned outputs are zeroed on failure.

## **9) Acceptance Criteria**

- No direct `malloc`/`calloc`/`realloc`/`free` calls remain in touched modules (enforced in CI).
- Cache rebuild & attribution tests pass with OOM-after-N at multiple thresholds.    
- CI enforces peak-memory budgets; failures provide op-scoped metrics.
- Headers export _owned/_borrowed/_view consistently; lint is green.
- Bench: equal or better P50/P99 for rebuilds and scans; memory peaks stable across runs.

---

## **Appendix A — Code Skeletons (Drop-in)**

```c
// gm_result_ptr helpers
static inline gm_result_ptr_t gm_result_ptr(void* p, void(*d)(void*)) {
  gm_result_ptr_t r = { .ptr = p, .destroy = d }; return r;
}
static inline void gm_result_ptr_free(gm_result_ptr_t* rp) {
  if (rp && rp->ptr && rp->destroy) rp->destroy(rp->ptr);
  if (rp) rp->ptr = NULL, rp->destroy = NULL;
}
```

```c
// OOM-after-N fake (diagnostics ctx carries a countdown)
typedef struct { const gm_allocator_port_t* base; uint64_t n_left; } gm_oom_after_n_t;
static void* oom_alloc(void* ctx, size_t n) {
  gm_oom_after_n_t* s = ctx; if (s->n_left && --s->n_left==0) return NULL;
  return s->base->alloc(s->base->ctx, n);
}
// … implement realloc/free similarly and plug diagnostics counters.
```

```c
// Tracing allocator (bytes_outstanding/peak)
typedef struct { const gm_allocator_port_t* base; uint64_t out, peak, fails; } gm_trace_alloc_t;
static void* trace_alloc(void* c, size_t n){ gm_trace_alloc_t*s=c; void*p=s->base->alloc(s->base->ctx,n);
  if(!p){s->fails++;return NULL;} s->out+=n; if(s->out>s->peak)s->peak=s->out; return p; }
```

---

# **Appendix B — Policy Lint (cheap & cheerful)**

- **No raw allocs:** `rg -n "(^|[^_])\b(malloc|calloc|realloc|free)\b" --glob '!**/util/**'`
- **Owned requires destroy:** script parses public headers; for any *_owned return, assert a *_free exists.
- **Suffixes:** `rg -n ".*_view\("` in headers; check docs include lifetime note

---

# **Appendix C — Configuration**

- Env overrides:    
    - `GM_ARENA_HINT_CACHE_REBUILD`, `GM_ARENA_CAP_CACHE_REBUILD`
    - `GM_ARENA_HINT_ATTR_QUERY`, `GM_ARENA_CAP_ATTR_QUERY`
    - `GM_TEST_OOM_N` for fail-injection in tests.
- Telemetry tags: `alloc.op=cache.rebuild|attr.query|journal.read`.