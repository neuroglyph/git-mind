# Memory Allocation Strategy — Execution Checklist

- [ ] Global Toggles & CI Variants
> [!info]- Implementation Details
> **Scope:** Introduce runtime kill-switches (`GM_DISABLE_CUSTOM_ALLOCATORS`, tracing toggle) and deterministic CI variants (ASAN/LSAN, release, budgeted runs).  
> **Steps:** wire env parsing into context bootstrap; update CI configs/docker scripts; pin budget job to a vendored fixture repo snapshot.  
> **Acceptance:** Dedicated CI matrix green (asan+lsan, release, budgeted fixture); setting `GM_DISABLE_CUSTOM_ALLOCATORS=1` forces legacy system allocator with zero code changes.

- [ ] Allocator Port Scaffolding
> [!info]- Implementation Details
> **Scope:** Add `gitmind/ports/allocator_port.h` and `gitmind/util/alloc_wrappers.h` with helpers (`gm_xalloc`, etc.) and diagnostics hooks.  
> **Steps:** define `gm_allocator_port_t` (function pointers, diagnostics), inline wrappers, umbrella-safe includes.  
> **Acceptance:** Project builds; no domain/service file includes `<stdlib.h>` for allocation APIs.

- [ ] System Allocator Adapter & Tests
> [!info]- Implementation Details
> **Scope:** Default adapter wrapping `malloc`/`realloc`/`free`, storing diagnostics with atomics; place under `core/src/adapters/memory/`.  
> **Steps:** implement adapter factory; add unit/TSAN smoke tests validating `bytes_outstanding`, `peak`, `failures_total`.  
> **Acceptance:** Tests confirm counters behave under concurrency; adapter passes TSAN or documents thread safety w/ atomics.

- [ ] Thread Allocator Through gm_context_t
> [!info]- Implementation Details
> **Scope:** Make allocator mandatory in `gm_context_t`, composition roots, and test fixtures.  
> **Steps:** update struct, init helpers, fakes; run `make header-compile`.  
> **Acceptance:** All builds succeed; null allocator use prevented at compile time.

- [ ] Raw Alloc Guardrails (CI + Pre-commit)
> [!info]- Implementation Details
> **Scope:** Script `scripts/check-no-raw-allocs.sh` and register in pre-commit & CI; allowlist allocator implementation files only.  
> **Steps:** author POSIX shell script, integrate with Meson/Make, document in `AGENTS.md`.  
> **Acceptance:** Introducing raw `malloc`/`free` outside allowlist fails pre-commit and CI.

- [ ] FS Temp Ports via Allocator & gm_fs_path_free_owned
> [!info]- Implementation Details
> **Scope:** Inject allocator into posix/fake temp adapters; canonical paths `_owned`; add shared destroy helper.  
> **Steps:** adjust adapters, tests, and call sites (journal/cache) to use `gm_fs_path_free_owned`.  
> **Acceptance:** All canonical paths freed via helper; LSAN/tracing show zero leaks.

- [ ] Arena Utility (Thread-confined, max_align_t)
> [!info]- Implementation Details
> **Scope:** Implement `gitmind/util/arena.{h,c}` with geometric growth, cap enforcement, `max_align_t` guarantee, optional `*_alloc_aligned`, and optional zero_on_free behaviour; arenas are per-operation and thread-confined.  
> **Steps:** use allocator port for backing blocks, enforce `max_align_t` alignment by default, provide aligned variant, expose zeroize toggle for security-sensitive buffers, add unit tests for cap, reset, alignment, and zeroization flag behaviour.  
> **Acceptance:** Unit tests pass; arena rejects cap overflow; documentation states thread-confined policy.

- [ ] Cache Rebuild Integration with Arena
> [!info]- Implementation Details
> **Scope:** Wrap cache rebuild entry in arena begin/end; move temp buffers into arena; ensure nothing escapes scope.  
> **Steps:** refactor `cache_rebuild_service.c`, audit early returns; add LSAN regression tests.  
> **Acceptance:** Rebuild behaviour unchanged; LSAN/tracing clean; arena freed on all paths.

- [ ] Allocation Metrics & Deterministic Budget
> [!info]- Implementation Details
> **Scope:** Emit `alloc.peak_bytes` with `alloc.op=cache.rebuild`; deterministic budget using vendored fixture tarball + pinned SHA embedded in repo (no network).  
> **Steps:** measure via tracing allocator, capture allocator ID to guard cross-free attempts, warn ≥80% cap (204 MiB), fail >256 MiB, log fixture SHA/hash and environment metadata.  
> **Acceptance:** Budget job reports peak and hash; job fails if over cap; telemetry errors freed.

- [ ] Slab Allocator Utility (Per-thread policy)
> [!info]- Implementation Details
> **Scope:** `gitmind/util/slab.{h,c}` with bins 32/64/128/256, 64 KiB superblocks, `max_align_t` guarantee plus optional aligned variant; instantiate per-thread via TLS or context injection with documented policy.  
> **Steps:** implement per-thread slab map (or TLS), ensure cross-thread access is guarded or forbidden, add tests for bin selection, reuse, TSAN safety, and zeroization option.  
> **Acceptance:** Unit/TSAN tests green; documentation states thread policy; alignment guaranteed.

- [ ] Route Small Objects Through Slab
> [!info]- Implementation Details
> **Scope:** Update `gm_error_t`, telemetry kv structs, other ≤256B pods to allocate via slab API.  
> **Steps:** refactor constructors/destructors; ensure slab frees on destroy; benchmark jitter before/after.  
> **Acceptance:** Benchmarks show reduced small-alloc jitter; LSAN/tracing remain clean.

- [ ] Intern Table Utility (Service-scoped, refcounted)
> [!info]- Implementation Details
> **Scope:** `gitmind/util/intern.{h,c}` offering acquire/retain/release, pointer access, `max_align_t` storage; implement internal sharded locks (or documented single-thread guarantee) for multi-thread safety.  
> **Steps:** implement hash table with sharded locks, ref counting, tests for churn/dedup and threaded contention.  
> **Acceptance:** Unit tests cover dedup & retain/release; documented thread model; memory steady after churn.

- [ ] Apply Interning in Hot Services
> [!info]- Implementation Details
> **Scope:** Integrate intern tables into journal reader/writer, cache telemetry, other repeated path/OID usage.  
> **Steps:** replace `strdup/gm_strcpy_safe` loops with intern handles; ensure service lifetime manages table; update metrics/test coverage.  
> **Acceptance:** Functional parity; instrumentation shows drop in duplicate strings; docs updated.

- [ ] OOM-after-N Allocator Wrapper & Test Harness
> [!info]- Implementation Details
> **Scope:** Wrapper around allocator port decrementing counter; env `GM_TEST_OOM_N` for tests.  
> **Steps:** implement wrapper, add Meson test cases with N∈{1,3,7,37}; ensure deterministic resets.  
> **Acceptance:** Tests pass under OOM scenarios; no infinite retries; tracing confirms no leaks post-failure.

- [ ] Tracing Allocator Wrapper & Leak Assertions
> [!info]- Implementation Details
> **Scope:** Wrapper tracking outstanding bytes, peak, failures; stamps allocator ID (debug) to forbid cross-allocator frees; asserts zero outstanding in teardown; optional zeroization toggle.  
> **Steps:** implement wrapper with allocator-ID tagging (debug builds), integrate into test fixtures, add optional secure wipe hook and instrumentation toggles.  
> **Acceptance:** Tests fail fast on non-zero outstanding bytes with informative diagnostics; zeroization optional flag works.

- [ ] Ownership Suffix Doc-lint (Public Headers Only)
> [!info]- Implementation Details
> **Scope:** Script ensuring `_owned` exports have destroy helper, `_view`/`_borrowed` documented; ignore static/private symbols.  
> **Steps:** parse headers, enforce doc presence, integrate with CI; allowlist legacy items with TODO+issue.  
> **Acceptance:** Missing destroy/doc fails CI immediately; script skips non-public symbols.

- [ ] Allocation Histogram Instrumentation (Test-only)
> [!info]- Implementation Details
> **Scope:** Optional histogram sampling controlled by `GM_ALLOC_HISTO` to log p50/p90/p99 during tests.  
> **Steps:** implement lightweight sampler in tracing allocator; ensure disabled in release builds.  
> **Acceptance:** When enabled, tests print percentiles & env metadata; disabled by default to avoid perf hit.

- [ ] Documentation & Release Notes
> [!info]- Implementation Details
> **Scope:** Update Memory Allocation Policy, Hexagonal Architecture doc, Telemetry Strategy, `AGENTS.md`; add release notes & benchmark tables with env hashes.  
> **Steps:** describe allocator port usage, thread policy, kill-switch, CI budgets; record benchmark methodology.  
> **Acceptance:** `make docs-verify` passes; release notes include rollback plan and fixture/hash references.
