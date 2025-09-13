# Test Plan

Table of Contents
- [Executive Summary](#executive-summary)
- [Coverage Goals](#coverage-goals)
- [Test Scenarios for Critical Paths](#test-scenarios-for-critical-paths)
- [Performance Benchmarks](#performance-benchmarks)
- [Security Testing Approach](#security-testing-approach)

## Executive Summary
Testing emphasizes determinism, backward compatibility, and performance. Targets are scoped to solo maintenance while keeping confidence high. See related: [Release Plans](../planning/Release_Plans.md) acceptance criteria.

## Coverage Goals
- Unit: ≥ 80% for new/changed core (IDs, CBOR, cache routing)
- Integration: Exercise CLI flows (link, list, cache-rebuild, cohesion-report)
- E2E: Repo-level tests with small synthetic graphs; branch merge scenarios

## Test Scenarios for Critical Paths
- ID Helpers
  - Unicode NFC cases; stable vectors across runs
  - Collision sampling (not guaranteed-free; assert low rate over corpus)
- CBOR Encoding/Decoding
  - Edges with/without names; tolerant decode of legacy blobs
  - Corrupt data handling (graceful errors)
- Cache Builder/Query
  - Name filters → ID parity checks
  - Incremental rebuild from journal tip; cache invalidation on schema change
- Cohesion Report
  - Scalar flips and set diffs across simulated merges
  - Depth capping for `implies` chains; cycle detection
- Advice Merge (if enabled)
  - LWW and OR-Set behaviors; tombstone wins/remains until re-added

## Performance Benchmarks
- Local sample graph (≥100k edges):
  - Median list query: < 10ms
  - P95 list query: < 50ms
  - Rebuild throughput: > 50k edges/sec
- Record hardware, dataset generator, and exact command lines in repo for repeatability.

## Security Testing Approach
- Static analysis: clang-tidy zero warnings in cache/journal/cbor
- Safe wrappers for `memcpy/snprintf/vsnprintf`; negative tests for bounds
- Fuzz minimal decoders (CBOR) with small AFL or libFuzzer corpus (time-boxed)
- Dependency audit: Verify libgit2 and CRoaring versions; license checks
