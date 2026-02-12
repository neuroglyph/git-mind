# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.0-alpha.3] - 2026-02-12

### Added

- **`git mind export` command** — Serialize the graph to YAML or JSON in v1 import-compatible format, enabling round-trip workflows. Supports `--format yaml|json`, `--prefix <prefix>` filtering, file output or stdout, and `--json` for structured metadata (#195)
- **Export API** — `exportGraph(graph, opts)`, `serializeExport(data, format)`, `exportToFile(graph, path, opts)` in `src/export.js` (#195)
- **`git mind import --from-markdown` command** — Import nodes and edges from markdown file frontmatter. Auto-generates `doc:` IDs from file paths, recognizes all 8 edge types as frontmatter fields. Supports `--dry-run`, `--json`, glob patterns (#196)
- **Frontmatter API** — `parseFrontmatter(content)`, `extractGraphData(path, frontmatter)`, `findMarkdownFiles(basePath, pattern)`, `importFromMarkdown(graph, cwd, pattern, opts)` in `src/frontmatter.js` (#196)
- **`importData` shared pipeline** — Extracted from `importFile` in `src/import.js` for reuse by frontmatter import and future merge (#196)
- **Cross-repo edge protocol** — `repo:owner/name:prefix:identifier` syntax for referencing nodes in other repositories. `git mind link --remote <owner/name>` qualifies local IDs. Validators accept cross-repo format, `extractPrefix` returns inner prefix (#197)
- **Remote API** — `parseCrossRepoId`, `buildCrossRepoId`, `isCrossRepoId`, `extractRepo`, `qualifyNodeId` in `src/remote.js` (#197)
- **`git mind merge` command** — Merge another repository's graph into the local graph with cross-repo qualification. Supports `--from <path>`, `--repo-name <owner/name>`, `--dry-run`, `--json`. Auto-detects repo identifier from origin remote (#198)
- **Merge API** — `mergeFromRepo(localGraph, remotePath, opts)`, `detectRepoIdentifier(repoPath)` in `src/merge.js` (#198)
- **GitHub Action** — Composite action (`action.yml`) that runs `git mind suggest` on PRs and posts formatted suggestions as a comment. Configurable agent command via action input or `.github/git-mind.yml` (#199)
- **PR suggestion display** — `formatSuggestionsAsMarkdown` renders suggestions as a markdown table with `/gitmind accept|reject|accept-all` commands. `parseReviewCommand` parses slash commands from comment bodies (#200)
- **Slash command workflow** — `.github/workflows/gitmind-review.yml` handles `/gitmind accept N`, `/gitmind reject N`, and `/gitmind accept-all` commands in PR comments (#200)

### Changed

- **`repo` added to `SYSTEM_PREFIXES`** — Cross-repo IDs use the `repo:` prefix, now classified as system (#197)
- **Test count** — 283 tests across 18 files (was 208 across 13)

## [2.0.0-alpha.2] - 2026-02-11

### Added

- **`git mind doctor` command** — Graph integrity checking with four composable detectors: dangling edges (error), orphan milestones (warning), orphan nodes (info), low-confidence edges (info). Supports `--fix` to auto-remove dangling edges, `--json` for structured output. Exit code 1 on errors (#193)
- **Doctor API** — `runDoctor(graph)`, `fixIssues(graph, issues)`, and individual detectors (`detectDanglingEdges`, `detectOrphanMilestones`, `detectOrphanNodes`, `detectLowConfidenceEdges`) in `src/doctor.js` (#193)
- **Git context extraction** — `src/context.js` extracts file, commit, and graph context for LLM prompts. Language inference from file extensions. Size-bounded prompt generation (~4000 chars) (#193)
- **`git mind suggest` command** — AI-powered edge suggestions via `GITMIND_AGENT` env var. Shells out to any command (stdin prompt, stdout JSON). Supports `--agent <cmd>`, `--context <sha-range>`, `--json`. Zero new dependencies (#193)
- **Suggest API** — `callAgent(prompt)`, `parseSuggestions(text)` (handles raw JSON and markdown code fences), `filterRejected(suggestions, graph)`, `generateSuggestions(cwd, graph)` in `src/suggest.js` (#193)
- **`git mind review` command** — Interactive review of pending suggestions with `[a]ccept / [r]eject / [s]kip` prompts via readline. Non-interactive batch mode via `--batch accept|reject`. `--json` output (#193)
- **Review API** — `getPendingSuggestions(graph)`, `acceptSuggestion` (promotes confidence to 1.0), `rejectSuggestion` (removes edge), `adjustSuggestion` (updates edge props), `skipSuggestion` (no-op), `getReviewHistory`, `batchDecision` in `src/review.js` (#193)
- **Decision provenance** — Review decisions stored as `decision:` prefixed nodes with action, source, target, edgeType, confidence, rationale, timestamp, and reviewer properties. Rejected edges excluded from future suggestions (#193)
- **`coverage` view** — Code-to-spec gap analysis: identifies `crate:`/`module:`/`pkg:` nodes lacking `implements` edges to `spec:`/`adr:` targets. Returns `meta.linked`, `meta.unlinked`, and `meta.coveragePct` (#191)
- **Echo ecosystem seed fixture** — `test/fixtures/echo-seed.yaml` with 55 nodes and 70 edges for integration testing (#191)
- **PROVING GROUND integration tests** — `test/proving-ground.test.js` validates 5 real project management questions against the Echo seed with deterministic ground truth (#191)
- **Dogfood session transcript** — `docs/dogfood-session.md` documents CLI walkthrough of all 5 questions (#191)

### Fixed

- **`parseFlags` boolean flag handling** — `--json` and `--fix` no longer consume the next argument as a value, fixing `git mind suggest --json --agent <cmd>` (#193)
- **Shell injection in `extractCommitContext`** — `opts.range` validated against shell metacharacters; commit SHAs validated as hex before interpolation into `execSync` (#193)
- **ReDoS in `parseSuggestions`** — Replaced polynomial regex for code fence extraction with non-backtracking pattern; replaced greedy array regex with `indexOf`/`lastIndexOf` (#193)
- **Agent subprocess timeout** — `callAgent` now enforces a configurable timeout (default 2 min) via `opts.timeout`, killing hung agent processes (#193)
- **Readline leak in interactive review** — `rl.close()` now called via `try/finally` to prevent terminal state corruption on error (#193)
- **Non-atomic edge type change in `adjustSuggestion`** — New edge created before old edge removed, preventing data loss if `createEdge` throws (#193)
- **Magic confidence default** — `adjustSuggestion` now preserves `original.confidence` instead of silently defaulting to 0.8 (#193)
- **`batchDecision` action validation** — Throws on invalid action instead of silently falling through to reject (#193)
- **Loose file node matching** — `extractGraphContext` uses exact match (`file:${fp}`) or suffix match instead of `includes()` to prevent false positives (#193)
- **`fixResult.details` guard** — `formatDoctorResult` handles undefined `details` array with nullish coalescing (#193)
- **`makeDecisionId` JSDoc** — Updated to say "unique" instead of "deterministic" since it includes `Date.now()` (#193)
- **`fixIssues` named properties** — Uses `issue.source`/`issue.target`/`issue.edgeType` instead of positional destructuring (#193)
- **N+1 query optimization** — `getPendingSuggestions`, `getReviewHistory`, and `filterRejected` use `Promise.all` for concurrent node prop fetches (#193)
- **Consistent flag handling** — `doctor`, `suggest`, `review` CLI commands read `--json`/`--fix` from `parseFlags` instead of mixing `args.includes()` (#193)
- **Sanitize `opts.limit`** — `extractCommitContext` coerces limit to safe integer (1–100) before shell interpolation (#193)
- **Expanded sanitization blocklist** — `sanitizeGitArg` now also rejects `<`, `>`, `\n`, `\r` (#193)
- **Unused import removed** — `extractPrefix` import removed from `src/doctor.js` (#193)
- **Decision nodes excluded from orphan detection** — `detectOrphanNodes` skips `decision:` prefix nodes (#193)
- **Defensive guard on `result.errors`** — `formatSuggestions` uses optional chaining for `result.errors` (#193)
- **ReDoS fence regex eliminated** — Replaced regex-based code fence extraction with `indexOf`-based approach (#193)
- **`skipSuggestion` documented as deferred** — JSDoc clarifies skip is intentional defer, not dismiss (#193)
- **Single-writer assumption documented** — `acceptSuggestion` and `adjustSuggestion` JSDoc notes edge must exist (#193)
- **`formatDecisionSummary` guard** — `result.decisions` now defaults to `[]` via nullish coalescing to prevent TypeError (#193)
- **`DoctorIssue` typedef updated** — Added optional `source`, `target`, `edgeType` properties used by dangling-edge issues (#193)
- **`adjustSuggestion` sets `reviewedAt` on type change** — New edge created during type change now receives a `reviewedAt` timestamp (#193)
- **`generateSuggestions` rejection diagnostic** — Returns `rejectedCount` and logs a diagnostic when all suggestions were previously rejected (#193)
- **`child.stdin` error handler** — `callAgent` attaches a no-op error listener on stdin to prevent uncaught EPIPE exceptions (#193)
- **Doctor test fixture corrected** — Dangling-edge test issue now includes `source`/`target`/`edgeType` matching `fixIssues` expectations (#193)
- **`buildPrompt` defensive guards** — Handles nullish `context.graph`/`commits`/`files` with defaults instead of throwing TypeError (#193)
- **`fetchDecisionProps` shared helper** — Extracted duplicated decision-node fetch logic from `getPendingSuggestions` and `getReviewHistory` into a reusable helper (#193)

### Changed

- **`suggest` and `review` stubs replaced** with full implementations (#193)
- **Test count** — 208 tests across 13 files (was 143 across 8)

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

[2.0.0-alpha.3]: https://github.com/neuroglyph/git-mind/releases/tag/v2.0.0-alpha.3
[2.0.0-alpha.2]: https://github.com/neuroglyph/git-mind/releases/tag/v2.0.0-alpha.2
[2.0.0-alpha.0]: https://github.com/neuroglyph/git-mind/releases/tag/v2.0.0-alpha.0
