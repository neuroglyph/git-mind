# 2025-10-09 — PR Feedback Remediation Notes

## Summary
- Document logging/diagnostics adapter improvements in `apps/cli/main.c`, including logger level typing, adapter disposal, and diagnostics opt-in.
- Publicized telemetry formatter types via `core/include/gitmind/telemetry/log_format.h` and updated all dependents to consume the new header.
- Added lifecycle management for diagnostics ports (`gm_diag_reset`, optional vtable disposer) and implemented real disposers for stderr and fake diagnostics adapters.
- Hardened journal codec helpers with capacity-aware decode and corrected encode length accounting; updated reader call sites accordingly.
- Secured libgit2 ref adapter directory creation with normalized refs, traversal checks, and strict truncation handling.
- Refined telemetry extras handling (derived cap) and ensured hashing helpers build with explicit includes.
- Reworked test temp repo helpers into a shared implementation with cleanup helpers, plus a `gitmind_test_support` static library linked into integration/unit tests relying on repo fakes.
- `tools/ci/ci_local.sh` now has cross-platform `mktemp` fallback and explicit `core/**` inclusion to avoid rsync omission.
- Removed obsolete `.github/workflows/apply-feedback.yml` workflow per feedback.
- Eliminated the diagnostics/cache crash by treating `gm_fs_temp_port_canonicalize_ex` results as `_view` data (copy instead of `free`), updating cache/journal callers, and validating with ASAN plus `make ci-local`.
- Introduced `tools/debug/debug-container.sh` and documented the workflow in `docs/operations/Debug_Container.md`, including Neo4j linkage steps for docs ↔ code traceability.
- Added regression coverage (`core/tests/integration/test_cache_rebuild_canonicalize.c`) to ensure cache rebuild + journal append reuse canonicalized paths safely.

## Follow-ups
1. **Staging cleanup:** remove stale `/var/folders/*/gitmind-ci-*` staging directories if no longer needed.
2. **Neo4j doc linkage:** keep the Neo4j map refreshed for new docs/scripts (`Debug_Container`, fs-temp owners) so future refactors know their documentation footprint.
3. **Cache tidy hygiene:** legacy include-cleaner + identifier warnings (e.g., `telemetry/internal/config.h`, `cache/internal/oid_prefix.h`) still need focused lint cleanup in a dedicated pass.
