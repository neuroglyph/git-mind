# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased] — PROVING GROUND

### Added

- **`coverage` view** — Code-to-spec gap analysis: identifies `crate:`/`module:`/`pkg:` nodes lacking `implements` edges to `spec:`/`adr:` targets. Returns `meta.linked`, `meta.unlinked`, and `meta.coveragePct`
- **Echo ecosystem seed fixture** — `test/fixtures/echo-seed.yaml` with 55 nodes and 70 edges for integration testing (5 milestones, 5 specs, 5 ADRs, 5 docs, 15 crates, 11 tasks, 9 issues)
- **PROVING GROUND integration tests** — `test/proving-ground.test.js` validates 5 real project management questions against the Echo seed with deterministic ground truth
- **Dogfood session transcript** — `docs/dogfood-session.md` documents CLI walkthrough of all 5 questions with answers and timing

### Changed

- **Test count** — 162 tests across 9 files (was 143 across 8)

## [2.0.0-alpha.1] - 2026-02-11

### Added

- **Node query API** — `src/nodes.js` promotes nodes from implicit edge endpoints to first-class queryable entities: `getNodes()`, `hasNode()`, `getNode()`, `getNodesByPrefix()`
- **`getNode()` returns full node info** — ID, extracted prefix, prefix classification (`canonical`/`system`/`unknown`), and properties from the materialized graph
- **`git mind nodes` command** — List and inspect nodes with `--prefix <prefix>` filtering, `--id <nodeId>` single-node detail, and `--json` output
- **Node formatting** — `formatNode()` and `formatNodeList()` in `src/cli/format.js` for terminal display
- **`git mind status` command** — Graph health dashboard showing node counts by prefix, edge counts by type, blocked items, low-confidence edges, and orphan nodes. Supports `--json` for CI pipelines
- **Status computation API** — `computeStatus(graph)` in `src/status.js` returns structured summary of graph state
- **YAML import pipeline** — `git mind import <file>` with schema-validated ingestion (`version: 1` required), idempotent merge semantics, reference validation (no dangling edges), and atomic writes (all-or-nothing)
- **Import API** — `importFile(graph, path, { dryRun })`, `parseImportFile()`, `validateImportData()` in `src/import.js`; exported from public API
- **Import CLI flags** — `--dry-run` validates without writing, `--validate` (alias), `--json` for structured output
- **Node properties in import** — YAML nodes can declare `properties:` key/value maps, written via `patch.setProperty()`
- **Declarative view engine** — `declareView(name, config)` compiles prefix/type filter configs into views; existing `roadmap`, `architecture`, `backlog` views refactored to declarative configs
- **`milestone` view** — progress tracking per milestone: child task counts, completion percentage, blockers
- **`traceability` view** — spec-to-implementation gap analysis: identifies unimplemented specs/ADRs, reports coverage percentage
- **`blockers` view** — transitive blocking chain resolution with cycle detection, root blocker identification
- **`onboarding` view** — topologically-sorted reading order for doc/spec/ADR nodes with cycle detection
- **Schema validators** — `src/validators.js` enforces GRAPH_SCHEMA.md at runtime: node ID grammar (`prefix:identifier`), edge type validation, confidence type safety (rejects NaN/Infinity/strings), self-edge rejection for `blocks` and `depends-on`, prefix classification with warnings for unknown prefixes
- **Validator exports** — `validateNodeId`, `validateEdgeType`, `validateConfidence`, `validateEdge`, `extractPrefix`, `classifyPrefix`, plus constants `NODE_ID_REGEX`, `NODE_ID_MAX_LENGTH`, `CANONICAL_PREFIXES`, `SYSTEM_PREFIXES`

### Fixed

- **`buildChain` stack overflow on cyclic graphs** — Root blocker leading into a cycle (e.g., `C → A → B → A`) caused infinite recursion; added visited guard (#189)
- **Duplicate cycle reports in blockers view** — Per-root DFS visited sets caused the same cycle to be reported from multiple entry points; switched to global visited set (#189)
- **O(n*m) lookups in traceability/onboarding views** — Replaced `Array.includes()` with `Set.has()` for spec and sorted membership checks (#189)
- **YAML arrays now rejected by `parseImportFile`** — `typeof [] === 'object'` no longer bypasses the guard; arrays produce "not an object" error instead of a confusing "Missing version" (#187)
- **Array-typed `node.properties` rejected during validation** — `validateImportData` now rejects arrays in `properties`, preventing `Object.entries` from writing numeric-indexed keys (#187)
- **Edge `createdAt` renamed to `importedAt`** — The timestamp on imported edges now honestly reflects import time; avoids misleading "creation" semantics on re-import (#187)
- **`--validate` alias documented in CLI help** — Usage text now shows `--dry-run, --validate` (#187)

### Changed

- **`blockedItems` now counts distinct blocked targets** — Previously counted `blocks` edges; now uses a `Set` on edge targets so two edges blocking the same node count as one blocked item (#185)
- **`isLowConfidence()` shared helper** — Low-confidence threshold (`< 0.5`) extracted from `status.js` and `views.js` into `validators.js` to keep the threshold in one place (#185)
- **`createEdge()` now validates all inputs** — Node IDs must use `prefix:identifier` format, confidence must be a finite number, self-edges rejected for blocking types
- **`EDGE_TYPES` canonical source** moved to `validators.js` (re-exported from `edges.js` for backwards compatibility)
- **`resetViews()` for test cleanup** — Removes test-registered views from the module-level registry, restoring built-in-only state (#189)
- **`builtInNames` initialized defensively** — Prevents `TypeError` if `resetViews()` is called before module finishes init (#189)
- **Removed dead `|| 0` fallback in onboarding view** — `inDegree` map is pre-initialized for all doc nodes, so the guard was unreachable (#189)
- **Milestone view returns self-contained subgraph** — Edge filter tightened from `||` to `&&` so returned edges only reference nodes in the result; eliminates dangling `implements` references to spec nodes (#189)
- **Onboarding view returns self-contained subgraph** — Same `||` → `&&` fix applied to `docEdges` filter; prevents non-doc nodes (e.g., `file:`) from appearing as dangling edge endpoints (#189)
- **`declareView` validates `config.prefixes`** — Throws on missing or empty prefixes array, surfacing misconfiguration early (#189)
- **Milestone view O(M×E) → O(E+M) edge lookups** — Pre-indexes `belongs-to` and `blocks` edges by target before the milestone loop (#189)
- **Onboarding ordering loop uses pre-filtered `docEdges`** — Eliminates redundant `docSet.has()` checks in dependency graph construction (#189)
- **Test count** — 143 tests across 8 files (was 74)

## [2.0.0-alpha.0] - 2026-02-07

Complete rewrite from C23 to Node.js on `@git-stunts/git-warp`.

### Added

- **Graph module** — Initialize, load, and checkpoint WARP graphs in any Git repo
- **Edge CRUD** — Create, query, and remove typed edges with confidence scores
- **8 edge types** — `implements`, `augments`, `relates-to`, `blocks`, `belongs-to`, `consumed-by`, `depends-on`, `documents`
- **Observer views** — Filtered projections: `roadmap`, `architecture`, `backlog`, `suggestions`
- **Commit hooks** — Parse directives (`IMPLEMENTS:`, `AUGMENTS:`, etc.) from commit messages to auto-create edges
- **CLI** — `git mind init`, `link`, `list`, `view`, `suggest` (stub), `review` (stub)
- **25 tests** — Full coverage of graph, edges, views, and hooks modules

### Changed

- **License** — Changed from MIND-UCAL-1.0 to Apache-2.0
- **Architecture** — Replaced C23 hexagonal architecture (libgit2, libsodium, CBOR, Roaring Bitmaps) with a thin Node.js wrapper around git-warp's CRDT graph engine

### Removed

- All C23 source code (archived on `archive/c23-final` branch)
- Meson build system
- Docker-based CI/CD
- All C-specific documentation

[2.0.0-alpha.0]: https://github.com/neuroglyph/git-mind/releases/tag/v2.0.0-alpha.0
