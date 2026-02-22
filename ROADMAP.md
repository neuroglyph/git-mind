# ROADMAP — git-mind v2

> **"A knowledge graph that thinks alongside you."**
>
> This roadmap is itself tracked as a DAG inside git-mind's own graph.
> Run `git mind view roadmap` to see it live.

---

## Vision

git-mind is a **project knowledge graph** built on git-warp. It turns any Git repository into a semantic graph of relationships between files, specs, milestones, crates, issues, and ideas — all stored in Git itself, powered by CRDTs.

The first consumer is the [Echo](https://github.com/neuroglyph/echo) ecosystem, where git-mind tracks the relationships between milestones, specs, ADRs, crates, and issues across a multi-repo architecture.

---

## Current State (v2.0.0-alpha.5)

All **8 milestones** shipped on `main` — Feb 2026. 342 tests across 20 files.

**v2.0.0-alpha.0** — Complete rewrite from C23 to Node.js on `@git-stunts/git-warp`.

**Shipped features (17 CLI commands):**
- Graph core (init, load, checkpoint via WARP CRDT)
- Edge CRUD with 8 typed edges + confidence scores
- Node query API (`nodes`, `--prefix`, `--id`, `--json`)
- Graph status dashboard (`status`, `--json`)
- 10 observer views: `roadmap`, `architecture`, `backlog`, `suggestions`, `milestone`, `traceability`, `blockers`, `onboarding`, `coverage` + custom declarative views
- YAML import pipeline with schema validation, dry-run, atomic writes
- Markdown frontmatter import (`import --from-markdown`)
- Graph export (YAML/JSON, round-trip compatible)
- Runtime schema validators (node IDs, edge types, confidence, prefixes)
- Commit directive parser (auto-create edges from commit messages)
- Cross-repo edge protocol (`repo:owner/name:prefix:id`)
- Multi-repo graph merge (`merge --from`)
- Graph integrity doctor (`doctor`, `--fix`, `--json`)
- AI-powered edge suggestions (`suggest`, `--agent`, `--context`)
- Interactive review flow (`review`, `--batch`, decision provenance)
- Epoch-based time-travel (`at <ref>`, `--json`)
- Graph diff between commits (`diff <ref-a>..<ref-b>`, `--prefix`, `--json`)
- GitHub Action for PR suggestions + slash command review
- 342 tests across 20 files, CI green

---

## Milestones

| # | Codename | Theme | Status |
|---|----------|-------|--------|
| 1 | **BEDROCK** | Schema & Node Foundations | DONE |
| 2 | **INTAKE** | Data Ingestion Pipeline | DONE |
| 3 | **PRISM** | Views & Value Surfaces | DONE |
| 4 | **WATCHTOWER** | Dashboard & Observability | DONE |
| 5 | **PROVING GROUND** | Dogfood Validation | DONE |
| 6 | **ORACLE** | AI Intelligence & Curation | DONE |
| 7 | **NEXUS** | Integration & Federation | DONE |
| 8 | **CHRONICLE** | Graph Diff & History | DONE |
| 9 | **IRONCLAD** | GA Hardening & Adoption | IN PROGRESS |

---

# Milestone 1: BEDROCK

> **"Before you build the cathedral, you lay the bedrock."**

**Goal:** Establish the schema contract, runtime validators, and node query layer that everything else rests on.

**Refs:** #180

**Milestone Dependencies:** None — this is the foundation.

---

## Feature BDK-SCHEMA: Schema Contract

> Define the grammar of the graph. What is a node? What is an edge? What are the rules?

### Task BDK-001: Write GRAPH_SCHEMA.md Specification

**User Story:** As a contributor, I want a single authoritative document that defines the graph schema so that I never have to guess what a valid node ID or edge looks like.

**Requirements:**
- Define node ID grammar: `prefix:identifier` with allowed characters
- Define prefix taxonomy (milestone, feature, task, spec, adr, crate, issue, concept, decision, person, tool, event, metric)
- Define edge type semantics for all 8 types (implements, augments, relates-to, blocks, belongs-to, consumed-by, depends-on, documents)
- Define duplicate handling rules
- Define update semantics (upsert by default)
- Include examples for every rule

**Acceptance Criteria:**
- [ ] Document exists at `GRAPH_SCHEMA.md`
- [ ] Every prefix in the taxonomy is defined with purpose and examples
- [ ] Every edge type has a definition, directionality, and valid source/target prefix constraints
- [ ] A "Non-Examples" section shows what's invalid and why
- [ ] The document is referenced from README.md

**Scope:**
- *In:* Node ID grammar, prefix taxonomy, edge semantics, duplicate/update rules
- *Out:* Runtime validation code (that's BDK-002), import semantics (that's INTAKE)

**Complexity:** ~200 LoC (markdown)
**Est. Human Hours:** 3

**Definition of Done:**
- Document reviewed and merged
- All edge types and prefixes documented
- Cross-referenced from README.md

**Blocked By:** —
**Blocking:** BDK-002, BDK-003, INT-001, INT-005

**Test Plan:**
- *Golden Path:* N/A (documentation)
- *Failure Modes:* N/A
- *Edge Cases:* Ensure all 8 edge types are covered, no prefix collisions
- *Fuzz/Stress:* N/A

---

### Task BDK-002: Implement Schema Runtime Validators

**User Story:** As a developer, I want runtime validation functions that enforce the schema contract so that invalid data is rejected at the boundary.

**Requirements:**
- `validateNodeId(id)` — returns `{ valid, prefix, identifier, error }`
- `validateEdgeType(type)` — returns `{ valid, error }`
- `validateEdge({ source, target, type, confidence })` — full edge validation
- `validatePrefix(prefix)` — checks against the taxonomy
- All validators must be pure functions (no side effects)
- Error messages must quote the invalid input and state the rule violated

**Acceptance Criteria:**
- [ ] `src/schema.js` exports all four validators
- [ ] Valid inputs return `{ valid: true }`
- [ ] Invalid inputs return `{ valid: false, error: "..." }` with descriptive message
- [ ] Confidence must be a number in [0.0, 1.0]
- [ ] Unknown prefixes are rejected
- [ ] Unknown edge types are rejected

**Scope:**
- *In:* Validation functions, error messages, prefix+edge-type enums
- *Out:* Graph writes (validators don't write), import logic

**Complexity:** ~120 LoC
**Est. Human Hours:** 3

**Definition of Done:**
- All validators implemented and exported
- Test suite passes (BDK-007)
- JSDoc on every exported function

**Blocked By:** BDK-001
**Blocking:** BDK-007, INT-005, INT-006

**Test Plan:**
- *Golden Path:* Valid node IDs (`milestone:BEDROCK`, `task:BDK-001`) pass validation
- *Failure Modes:* Missing prefix, empty identifier, unknown prefix, malformed confidence
- *Edge Cases:* Unicode identifiers, very long IDs (>256 chars), empty string, prefix-only (`milestone:`), colon-only (`:`)
- *Fuzz/Stress:* Fuzz validateNodeId with 10K random strings, expect no throws (only `{ valid: false }`)

---

## Feature BDK-NODES: Node Query Layer

> Nodes exist implicitly when edges reference them. This feature makes them first-class citizens.

### Task BDK-003: Implement Node Query & Inspection API

**User Story:** As a CLI user, I want to query and inspect nodes in the graph so that I can understand what exists without having to look at raw edges.

**Requirements:**
- `getNodes(graph, filter)` — return all nodes, optionally filtered by prefix
- `getNode(graph, id)` — return a single node with all its properties and connected edges
- `getNodesByPrefix(graph, prefix)` — convenience wrapper
- `hasNode(graph, id)` — boolean existence check
- Return values include: `{ id, prefix, identifier, props, inEdges, outEdges }`

**Acceptance Criteria:**
- [ ] `src/nodes.js` exports all four functions
- [ ] `getNodes()` with no filter returns all nodes
- [ ] `getNodes({ prefix: 'task' })` returns only task nodes
- [ ] `getNode(id)` returns full node detail with connected edges
- [ ] Non-existent node returns `null` (not throw)
- [ ] Works with the existing graph (backward compatible)

**Scope:**
- *In:* Read-only query functions, prefix filtering, edge aggregation
- *Out:* Node creation (nodes are created implicitly via edges), node deletion, node property mutation

**Complexity:** ~150 LoC
**Est. Human Hours:** 4

**Definition of Done:**
- All functions implemented and exported
- Test suite passes (BDK-008)
- Integrates with existing graph.js without breaking changes

**Blocked By:** BDK-001
**Blocking:** BDK-005, PRI-001, WTC-003

**Test Plan:**
- *Golden Path:* Create edges → query nodes → verify node list matches expected
- *Failure Modes:* Query empty graph returns `[]`, query nonexistent node returns `null`
- *Edge Cases:* Node referenced only as source vs. only as target, node with 0 edges after edge removal
- *Fuzz/Stress:* Create 1000 edges → verify getNodes() performance <100ms

---

### Task BDK-004: Node Property Getters & Metadata

**User Story:** As a developer, I want to attach and retrieve properties on nodes so that nodes carry meaningful metadata beyond just their ID.

**Requirements:**
- `setNodeProps(graph, id, props)` — shallow merge properties onto a node
- `getNodeProps(graph, id)` — retrieve all properties of a node
- Properties are arbitrary key-value pairs (string keys, JSON-serializable values)
- Setting properties on a non-existent node is a no-op (returns error)

**Acceptance Criteria:**
- [ ] `setNodeProps` and `getNodeProps` exported from `src/nodes.js`
- [ ] Properties persist across save/load cycle
- [ ] Shallow merge: existing props not mentioned are preserved
- [ ] Non-existent node returns `null` props

**Scope:**
- *In:* Property get/set, shallow merge semantics
- *Out:* Deep merge, property deletion, property validation, property types

**Complexity:** ~60 LoC
**Est. Human Hours:** 2

**Definition of Done:**
- Functions implemented
- Properties survive checkpoint/reload
- Test coverage in BDK-008

**Blocked By:** BDK-003
**Blocking:** BDK-005, INT-002

**Test Plan:**
- *Golden Path:* Set props → get props → verify match
- *Failure Modes:* Set on nonexistent node → error, get on nonexistent → null
- *Edge Cases:* Overwrite existing prop, add new prop to node with existing props, empty props object
- *Fuzz/Stress:* Set 100 properties on a single node, verify all retrievable

---

## Feature BDK-CLI: Node CLI Command

> Give humans a way to ask "what's in this graph?"

### Task BDK-005: Implement `git mind nodes` Command

**User Story:** As a CLI user, I want to run `git mind nodes` to see all nodes in the graph so that I can quickly understand the graph's contents.

**Requirements:**
- `git mind nodes` — list all nodes (ID + prefix)
- `git mind nodes --prefix <prefix>` — filter by prefix
- `git mind nodes --id <id>` — show detail for a single node
- `git mind nodes --json` — output as JSON array
- Default output: human-readable table with columns: ID, Prefix, # Edges

**Acceptance Criteria:**
- [ ] Command registered in CLI entry point
- [ ] No-arg invocation lists all nodes
- [ ] `--prefix` filters correctly
- [ ] `--id` shows full detail (props + edges)
- [ ] `--json` outputs valid JSON
- [ ] Empty graph shows informative message, not error

**Scope:**
- *In:* CLI command, flag parsing, formatted output, JSON output
- *Out:* Interactive mode, pagination, color customization

**Complexity:** ~100 LoC
**Est. Human Hours:** 3

**Definition of Done:**
- Command works end-to-end
- Help text updated
- JSON output parseable by `jq`

**Blocked By:** BDK-003, BDK-004
**Blocking:** PRV-003

**Test Plan:**
- *Golden Path:* Seed graph → run `git mind nodes` → verify output contains expected nodes
- *Failure Modes:* Uninitialized graph → helpful error, unknown prefix → empty result with message
- *Edge Cases:* Graph with only edges (no explicit nodes yet), very long node IDs
- *Fuzz/Stress:* Graph with 500 nodes → verify output completes in <2s

---

### Task BDK-006: Node Command Flag Integration

**User Story:** As a power user, I want the `--prefix`, `--id`, and `--json` flags to compose correctly so that I can script against git-mind output.

**Requirements:**
- `--prefix` and `--id` are mutually exclusive (error if both provided)
- `--json` works with both `--prefix` and `--id`
- JSON output follows a stable schema (document in GRAPH_SCHEMA.md)
- Exit code 0 on success, 1 on error, 2 on no results

**Acceptance Criteria:**
- [ ] `--prefix task --json` outputs JSON array of task nodes
- [ ] `--id milestone:BEDROCK --json` outputs JSON object with full detail
- [ ] Conflicting flags produce clear error message
- [ ] Exit codes are correct

**Scope:**
- *In:* Flag composition, error handling, exit codes
- *Out:* New flags beyond prefix/id/json

**Complexity:** ~40 LoC
**Est. Human Hours:** 1

**Definition of Done:**
- All flag combinations tested
- Exit codes verified in tests

**Blocked By:** BDK-005
**Blocking:** PRV-003

**Test Plan:**
- *Golden Path:* `--prefix task --json | jq length` returns correct count
- *Failure Modes:* `--prefix --id` together → error, `--prefix unknown` → exit 2
- *Edge Cases:* `--json` with empty result → `[]`, `--id nonexistent` → exit 2
- *Fuzz/Stress:* N/A

---

## Feature BDK-TEST: Foundation Test Suite

### Task BDK-007: Schema Validation Test Suite

**User Story:** As a maintainer, I want comprehensive schema validation tests so that I can refactor with confidence.

**Requirements:**
- Test every validator function from schema.js
- Test every prefix in the taxonomy
- Test every edge type
- Test boundary conditions for confidence scores
- Minimum 20 test cases

**Acceptance Criteria:**
- [ ] Test file: `test/schema.test.js`
- [ ] All validators have positive and negative test cases
- [ ] Confidence boundaries tested: 0.0, 0.5, 1.0, -0.1, 1.1, NaN, null
- [ ] All tests pass in CI

**Scope:**
- *In:* Unit tests for schema.js validators
- *Out:* Integration tests, CLI tests

**Complexity:** ~200 LoC
**Est. Human Hours:** 3

**Definition of Done:**
- All tests green
- Coverage: every validator, every prefix, every edge type

**Blocked By:** BDK-002
**Blocking:** INT-005

**Test Plan:**
- *Golden Path:* Each valid input passes
- *Failure Modes:* Each invalid input fails with correct error
- *Edge Cases:* Boundary values, empty strings, special characters
- *Fuzz/Stress:* Property-based testing with random inputs (if vitest supports it)

---

### Task BDK-008: Node Query Test Suite

**User Story:** As a maintainer, I want node query tests so that the query layer is reliable.

**Requirements:**
- Test getNodes, getNode, getNodesByPrefix, hasNode
- Test with empty graph, single-node graph, multi-node graph
- Test prefix filtering
- Test node detail (edges included)
- Minimum 15 test cases

**Acceptance Criteria:**
- [ ] Test file: `test/nodes.test.js`
- [ ] All query functions have positive and negative test cases
- [ ] Edge aggregation (inEdges, outEdges) verified
- [ ] All tests pass in CI

**Scope:**
- *In:* Unit tests for nodes.js query functions
- *Out:* CLI tests, integration tests

**Complexity:** ~180 LoC
**Est. Human Hours:** 3

**Definition of Done:**
- All tests green
- Coverage: every query function, every branch

**Blocked By:** BDK-003, BDK-004
**Blocking:** —

**Test Plan:**
- *Golden Path:* Seed graph → query → verify results
- *Failure Modes:* Empty graph queries, nonexistent node queries
- *Edge Cases:* Node with only inbound edges, node with only outbound edges, self-referential edge
- *Fuzz/Stress:* Randomized graph with 100 nodes → verify consistency

---

# Milestone 2: INTAKE

> **"Data in, knowledge out. But only clean data gets through the gate."**

**Goal:** Build a robust YAML import pipeline that ingests project structure into the graph — idempotent, validated, atomic.

**Refs:** #180

**Milestone Dependencies:** BEDROCK (schema validators + node layer required)

---

## Feature INT-ENGINE: YAML Import Engine

> The core logic that reads YAML and writes graph operations.

### Task INT-001: Implement Core Import Logic

**User Story:** As a project maintainer, I want to import a YAML file describing my project's structure so that I can seed the knowledge graph without manually linking everything.

**Requirements:**
- Parse YAML file with `nodes` and `edges` sections
- Nodes section: array of `{ id, props? }` objects
- Edges section: array of `{ source, target, type, confidence?, rationale? }` objects
- Validate all inputs against schema.js before writing
- Return a report: `{ nodesCreated, edgesCreated, errors }`

**Acceptance Criteria:**
- [ ] `src/import.js` exports `importFromYAML(graph, yamlString, opts)`
- [ ] Valid YAML with nodes and edges creates the expected graph state
- [ ] Invalid YAML returns errors without writing anything
- [ ] Report includes counts of created nodes and edges

**Scope:**
- *In:* YAML parsing, schema validation, graph writes, error reporting
- *Out:* File I/O (caller provides YAML string), CLI flags, export

**Complexity:** ~200 LoC
**Est. Human Hours:** 5

**Definition of Done:**
- Core import works end-to-end
- All inputs validated
- Report is accurate

**Blocked By:** BDK-002, BDK-003
**Blocking:** INT-002, INT-003, INT-007

**Test Plan:**
- *Golden Path:* Import valid YAML → verify nodes and edges in graph
- *Failure Modes:* Malformed YAML → parse error, invalid node ID → validation error, unknown edge type → rejected
- *Edge Cases:* Empty nodes array, empty edges array, YAML with only nodes (no edges), YAML with only edges
- *Fuzz/Stress:* Import YAML with 500 nodes and 2000 edges → verify performance <5s

---

### Task INT-002: Implement Idempotent Merge Semantics

**User Story:** As a user, I want to re-import the same YAML file without creating duplicates so that import is safe to run repeatedly.

**Requirements:**
- Re-importing a node with the same ID: shallow-merge properties
- Re-importing an edge with the same (source, target, type): update confidence and rationale
- The result of importing twice must equal the result of importing once
- Report distinguishes `created` vs `updated` counts

**Acceptance Criteria:**
- [ ] Double-import produces identical graph state
- [ ] Updated edges reflect new confidence/rationale values
- [ ] Updated nodes reflect merged properties
- [ ] Report shows `{ nodesCreated: 0, nodesUpdated: N, edgesCreated: 0, edgesUpdated: M }`

**Scope:**
- *In:* Idempotent upsert logic, merge semantics
- *Out:* `--no-overwrite` flag (future), conflict resolution

**Complexity:** ~80 LoC
**Est. Human Hours:** 3

**Definition of Done:**
- Double-import test passes
- Property merge verified
- Edge update verified

**Blocked By:** INT-001, BDK-004
**Blocking:** INT-008

**Test Plan:**
- *Golden Path:* Import → re-import → verify graph unchanged, report shows updates
- *Failure Modes:* N/A (idempotency doesn't fail, it converges)
- *Edge Cases:* Import with changed props → verify merge, import with changed confidence → verify update
- *Fuzz/Stress:* Import same file 100 times → verify graph state identical each time

---

## Feature INT-CLI: Import CLI Command

### Task INT-003: Implement `git mind import` Command

**User Story:** As a CLI user, I want to run `git mind import graph.yaml` to import a YAML file into the graph.

**Requirements:**
- `git mind import <file>` — import YAML file
- Read file from disk, parse, validate, import
- Print human-readable report after import
- Exit code 0 on success, 1 on validation errors

**Acceptance Criteria:**
- [ ] Command registered in CLI entry point
- [ ] File path resolved relative to CWD
- [ ] Success prints report with counts
- [ ] Validation errors printed with line context if possible

**Scope:**
- *In:* CLI command, file reading, report display
- *Out:* Stdin import, URL import, interactive mode

**Complexity:** ~80 LoC
**Est. Human Hours:** 2

**Definition of Done:**
- Command works end-to-end with a test YAML file
- Help text updated

**Blocked By:** INT-001
**Blocking:** INT-004, PRV-002

**Test Plan:**
- *Golden Path:* Create YAML file → run import → verify graph state
- *Failure Modes:* File not found → error, file not YAML → parse error, invalid schema → validation error
- *Edge Cases:* Empty file, file with only comments, file with BOM
- *Fuzz/Stress:* N/A

---

### Task INT-004: Import Command Flags (--dry-run, --validate, --json)

**User Story:** As a power user, I want `--dry-run` to preview changes and `--validate` to check syntax without writing so that I can safely prepare imports.

**Requirements:**
- `--dry-run` — parse, validate, compute changes, but don't write. Print what would happen.
- `--validate` — parse and validate only. Print validation result. Don't compute changes.
- `--json` — output report as JSON
- Flags compose: `--dry-run --json` outputs dry-run report as JSON

**Acceptance Criteria:**
- [ ] `--dry-run` shows changes without writing to graph
- [ ] `--validate` only checks schema validity
- [ ] `--json` outputs machine-readable report
- [ ] Graph is unchanged after `--dry-run` and `--validate`

**Scope:**
- *In:* Three flags, flag composition
- *Out:* `--force`, `--no-overwrite`, interactive confirmation

**Complexity:** ~60 LoC
**Est. Human Hours:** 2

**Definition of Done:**
- All three flags work individually and composed
- Graph integrity verified after dry-run

**Blocked By:** INT-003
**Blocking:** PRV-002

**Test Plan:**
- *Golden Path:* `--dry-run` shows report, graph empty afterward. `--validate` on valid file → exit 0
- *Failure Modes:* `--validate` on invalid file → exit 1 with errors
- *Edge Cases:* `--dry-run --validate` → validate takes precedence (least-write wins)
- *Fuzz/Stress:* N/A

---

## Feature INT-GUARD: Schema Enforcement

### Task INT-005: Version Field Enforcement

**User Story:** As a maintainer, I want import files to declare a schema version so that we can evolve the format without breaking old files.

**Requirements:**
- YAML must contain `version: 1` at root level
- Missing version → hard error with helpful message
- Unknown version (e.g., `version: 2`) → hard error: "Unknown schema version 2. This version of git-mind supports version 1."
- Version checked before any other validation

**Acceptance Criteria:**
- [ ] Import rejects files without `version` field
- [ ] Import rejects files with unknown version
- [ ] Error messages are specific and actionable
- [ ] Version check runs before node/edge validation

**Scope:**
- *In:* Version field checking, error messages
- *Out:* Version migration, multi-version support

**Complexity:** ~30 LoC
**Est. Human Hours:** 1

**Definition of Done:**
- Version enforcement in import pipeline
- Test coverage for all version scenarios

**Blocked By:** BDK-002, BDK-007
**Blocking:** INT-008

**Test Plan:**
- *Golden Path:* `version: 1` → accepted
- *Failure Modes:* Missing version → error, `version: 0` → error, `version: 2` → error, `version: "one"` → error
- *Edge Cases:* `version: 1.0` (float) → should it pass? Decide and document. `version: null` → error
- *Fuzz/Stress:* N/A

---

### Task INT-006: Reference Validation

**User Story:** As a user, I want the import to verify that all edge targets exist (either in the import file or already in the graph) so that I don't create dangling edges.

**Requirements:**
- Before writing, collect all node IDs (from import + existing graph)
- Every edge source and target must reference a known node
- Dangling references → hard error listing all unresolved refs
- Check runs after schema validation, before write phase

**Acceptance Criteria:**
- [ ] Dangling edge source detected and reported
- [ ] Dangling edge target detected and reported  
- [ ] Edges referencing nodes in the same import file pass
- [ ] Edges referencing existing graph nodes pass
- [ ] Error lists ALL dangling refs (not just first)

**Scope:**
- *In:* Reference validation against import + existing graph
- *Out:* Auto-creating missing nodes (explicit is better than implicit)

**Complexity:** ~60 LoC
**Est. Human Hours:** 2

**Definition of Done:**
- Reference validation integrated into import pipeline
- Test coverage for dangling refs

**Blocked By:** BDK-002, INT-001
**Blocking:** INT-007, INT-008

**Test Plan:**
- *Golden Path:* All refs resolve → import succeeds
- *Failure Modes:* Edge targets nonexistent node → error listing ref, multiple dangling → all reported
- *Edge Cases:* Self-referential edge (source == target), edge to node defined later in same file
- *Fuzz/Stress:* Import with 100 dangling refs → verify all 100 reported

---

## Feature INT-ATOMIC: Atomic Writes

### Task INT-007: Build-Validate-Write Pipeline

**User Story:** As a user, I want imports to be all-or-nothing so that a failed import never leaves my graph in a partial state.

**Requirements:**
- Phase 1: Parse YAML → build operation list
- Phase 2: Validate all operations (schema + refs)
- Phase 3: Execute all writes in a single patch
- If Phase 1 or 2 fails → zero writes
- Use `graph.createPatch()` to batch all operations

**Acceptance Criteria:**
- [ ] Validation failure → graph unchanged
- [ ] Parse failure → graph unchanged
- [ ] Partial data (10 valid nodes, 1 invalid) → zero nodes written
- [ ] Successful import → all operations in single patch commit

**Scope:**
- *In:* Three-phase pipeline, atomic batch via createPatch
- *Out:* Rollback (not needed if we don't write on failure), partial success mode

**Complexity:** ~100 LoC
**Est. Human Hours:** 3

**Definition of Done:**
- Pipeline implemented
- Atomicity verified by tests
- No partial writes possible

**Blocked By:** INT-001, INT-006
**Blocking:** INT-008

**Test Plan:**
- *Golden Path:* Valid import → all nodes and edges created in single patch
- *Failure Modes:* Import with 1 invalid node among 50 → zero writes, graph unchanged
- *Edge Cases:* Import that would update existing nodes + create new → all-or-nothing applies
- *Fuzz/Stress:* Import 1000 nodes atomically → verify single checkpoint created

---

### Task INT-008: Import Test Suite

**User Story:** As a maintainer, I want comprehensive import tests so that the pipeline is reliable.

**Requirements:**
- Test happy path (valid YAML → expected graph)
- Test idempotency (double import → same result)
- Test determinism (same input → same output, regardless of order)
- Test rejection (invalid schema, dangling refs, missing version)
- Test dry-run (no writes)
- Minimum 25 test cases

**Acceptance Criteria:**
- [ ] Test file: `test/import.test.js`
- [ ] All scenarios from above covered
- [ ] Test fixtures in `test/fixtures/` directory
- [ ] All tests pass in CI

**Scope:**
- *In:* Unit + integration tests for import pipeline
- *Out:* CLI tests (covered by command-level tests), performance benchmarks

**Complexity:** ~300 LoC
**Est. Human Hours:** 4

**Definition of Done:**
- All tests green
- Fixtures committed
- Coverage: every branch in import.js

**Blocked By:** INT-001, INT-002, INT-005, INT-006, INT-007
**Blocking:** —

**Test Plan:**
- *Golden Path:* Each test scenario as described
- *Failure Modes:* Tests themselves should be reviewed for false positives
- *Edge Cases:* Test ordering independence (tests don't depend on each other)
- *Fuzz/Stress:* Property-based: generate random valid YAML → import → export → re-import → verify identical

---

# Milestone 3: PRISM

> **"Raw data is noise. Views are signal. PRISM refracts the graph into actionable insight."**

**Goal:** Replace the current hardcoded views with a declarative view engine, then build four high-value views.

**Refs:** #180

**Milestone Dependencies:** BEDROCK (node query layer required for view rendering)

---

## Feature PRI-ENGINE: Declarative View Engine

### Task PRI-001: Refactor View System to Declarative Config

**User Story:** As a developer, I want to define new views by writing a config object instead of code so that adding a view is a 5-minute task, not a 2-hour task.

**Requirements:**
- View definition: `{ name, description, prefixes, edgeTypes, filter, sort, format }`
- `prefixes` — which node prefixes to include
- `edgeTypes` — which edge types to traverse
- `filter` — predicate function for additional filtering
- `sort` — sort order for results
- `format` — output template (human-readable)
- Existing 4 views (roadmap, architecture, backlog, suggestions) migrated to new format

**Acceptance Criteria:**
- [ ] `src/views.js` refactored to consume config objects
- [ ] Existing 4 views work identically after refactor
- [ ] Adding a new view requires only a config object
- [ ] View configs exportable for inspection

**Scope:**
- *In:* View engine refactor, config schema, migration of existing views
- *Out:* View composition, view inheritance, user-defined views

**Complexity:** ~200 LoC
**Est. Human Hours:** 5

**Definition of Done:**
- All 4 existing views pass regression tests
- New view can be added with just a config object
- No behavioral changes to existing output

**Blocked By:** BDK-003
**Blocking:** PRI-003, PRI-004, PRI-005, PRI-006

**Test Plan:**
- *Golden Path:* Existing views produce identical output before and after refactor
- *Failure Modes:* Invalid config → helpful error at registration time
- *Edge Cases:* View with no matching nodes → empty but valid output, view with circular edges
- *Fuzz/Stress:* Register 50 views → verify no performance degradation

---

### Task PRI-002: View Config Schema & Validation

**User Story:** As a developer, I want view configs validated at registration time so that typos in config are caught immediately, not at render time.

**Requirements:**
- Validate view name (non-empty string, unique)
- Validate prefixes (array of known prefixes)
- Validate edgeTypes (array of known edge types)
- Validation runs when view is registered, not when rendered
- Invalid config throws with specific error

**Acceptance Criteria:**
- [ ] Invalid view name → error at registration
- [ ] Unknown prefix in config → error at registration
- [ ] Unknown edge type → error at registration
- [ ] Duplicate view name → error at registration

**Scope:**
- *In:* View config validation
- *Out:* Dynamic view creation at runtime, user-provided configs

**Complexity:** ~60 LoC
**Est. Human Hours:** 2

**Definition of Done:**
- Validation integrated into view registration
- Test coverage for all validation rules

**Blocked By:** PRI-001, BDK-002
**Blocking:** PRI-007

**Test Plan:**
- *Golden Path:* Valid config registers successfully
- *Failure Modes:* Each invalid field detected and reported
- *Edge Cases:* Config with empty arrays, config with all fields, config with only required fields
- *Fuzz/Stress:* N/A

---

## Feature PRI-MILESTONE: Milestone View

### Task PRI-003: Implement Milestone Progress View

**User Story:** As a project lead, I want to see milestone progress (tasks done / total, blockers, timeline) so that I can track project health at a glance.

**Requirements:**
- Show each milestone with: name, completion %, task counts (done/in-progress/blocked/total)
- Show blocking chains (what blocks this milestone)
- Sort by completion % ascending (least done first)
- Support `--json` output

**Acceptance Criteria:**
- [ ] `git mind view milestone` shows milestone progress
- [ ] Completion % calculated from `belongs-to` edges
- [ ] Blockers identified via `blocks` edges
- [ ] JSON output includes all computed fields

**Scope:**
- *In:* Milestone view config, completion calculation, blocker detection
- *Out:* Gantt charts, timeline estimation, burndown

**Complexity:** ~80 LoC
**Est. Human Hours:** 3

**Definition of Done:**
- View renders correctly against test fixture
- Matches expected output exactly

**Blocked By:** PRI-001
**Blocking:** PRV-003

**Test Plan:**
- *Golden Path:* Fixture graph with 2 milestones, 5 tasks each → verify output matches snapshot
- *Failure Modes:* Milestone with no tasks → shows 0% complete, not error
- *Edge Cases:* Milestone with all tasks complete → 100%, milestone with circular blockers
- *Fuzz/Stress:* 50 milestones, 500 tasks → verify renders in <1s

---

## Feature PRI-TRACE: Traceability View

### Task PRI-004: Implement Spec-to-Implementation Traceability

**User Story:** As an architect, I want to see which specs have implementations and which don't so that I can identify gaps in coverage.

**Requirements:**
- List all `spec:*` nodes
- For each spec, show `implements` edges → what implements it
- Flag specs with zero implementations as "unimplemented"
- Flag implementations with no spec as "undocumented"
- Sort: unimplemented first, then by spec name

**Acceptance Criteria:**
- [ ] `git mind view traceability` shows spec coverage
- [ ] Unimplemented specs highlighted
- [ ] Undocumented implementations highlighted
- [ ] Bidirectional: specs → implementations AND implementations → specs

**Scope:**
- *In:* Traceability view, gap detection, bidirectional linking
- *Out:* Auto-linking (suggesting implementations for specs), coverage percentages

**Complexity:** ~80 LoC
**Est. Human Hours:** 3

**Definition of Done:**
- View renders correctly against test fixture
- Gaps are clearly visible

**Blocked By:** PRI-001
**Blocking:** PRV-003

**Test Plan:**
- *Golden Path:* 3 specs, 2 implemented, 1 not → verify output shows gap
- *Failure Modes:* No specs in graph → informative message
- *Edge Cases:* Spec implemented by multiple things, implementation linked to multiple specs
- *Fuzz/Stress:* 100 specs, 200 implementations → verify renders in <1s

---

## Feature PRI-BLOCK: Blockers View

### Task PRI-005: Implement Blocker Analysis View

**User Story:** As a project lead, I want to see all blocked items and their dependency chains so that I can identify and unblock critical paths.

**Requirements:**
- List all nodes that have incoming `blocks` edges (i.e., are blocked)
- For each blocked item, show the full blocking chain (transitive)
- Identify "root blockers" — items that block others but aren't themselves blocked
- Sort by chain length descending (deepest blockers first)

**Acceptance Criteria:**
- [ ] `git mind view blockers` shows all blocked items
- [ ] Transitive blocking chains shown (A blocks B blocks C → show full chain)
- [ ] Root blockers clearly identified
- [ ] Circular blocking detected and flagged (not infinite loop)

**Scope:**
- *In:* Blocker view, transitive chain resolution, cycle detection
- *Out:* Automatic unblocking suggestions, priority calculation

**Complexity:** ~120 LoC
**Est. Human Hours:** 4

**Definition of Done:**
- View renders correctly against test fixture
- Cycle detection works (no infinite loops)
- Critical path visible

**Blocked By:** PRI-001
**Blocking:** PRV-003

**Test Plan:**
- *Golden Path:* A→B→C blocking chain → verify full chain shown
- *Failure Modes:* No blockers → "No blocked items" message
- *Edge Cases:* Circular blocking (A blocks B blocks A), self-blocking (A blocks A), diamond dependency
- *Fuzz/Stress:* 20-deep blocking chain → verify renders without stack overflow

---

## Feature PRI-ONBOARD: Onboarding View

### Task PRI-006: Implement Onboarding Reading Order View

**User Story:** As a new engineer joining the project, I want a recommended reading order so that I can get up to speed efficiently.

**Requirements:**
- Traverse `documents` edges to find documentation nodes
- Order by dependency (read foundations before advanced topics)
- Include: specs, ADRs, guides, and onboarding-tagged nodes
- Show estimated reading time per item (from node props, if available)

**Acceptance Criteria:**
- [ ] `git mind view onboarding` shows ordered reading list
- [ ] Dependency-aware ordering (topological sort on `depends-on` edges)
- [ ] Items without dependencies shown first
- [ ] Reading time shown if available

**Scope:**
- *In:* Onboarding view, topological sort, reading time display
- *Out:* Interactive walkthrough, quiz/verification, personalized paths

**Complexity:** ~100 LoC
**Est. Human Hours:** 3

**Definition of Done:**
- View renders correctly against test fixture
- Topological order is correct

**Blocked By:** PRI-001
**Blocking:** PRV-003

**Test Plan:**
- *Golden Path:* 5 docs with dependencies → verify topological order
- *Failure Modes:* No documentation nodes → informative message
- *Edge Cases:* Circular dependencies → break cycle and warn, disconnected docs (no deps) → alphabetical
- *Fuzz/Stress:* 100 documentation nodes → verify topological sort <100ms

---

### Task PRI-007: View Fixture Test Suite

**User Story:** As a maintainer, I want every view tested against a known fixture graph with exact expected output so that view changes are intentional.

**Requirements:**
- Create a fixture graph (YAML) with enough data to exercise all views
- Snapshot tests: render each view → compare to expected output
- Test both human-readable and JSON output
- Minimum 10 test cases (2+ per view)

**Acceptance Criteria:**
- [ ] Test file: `test/views.test.js`
- [ ] Fixture graph in `test/fixtures/views-fixture.yaml`
- [ ] All 8 views tested (4 existing + 4 new)
- [ ] Both human and JSON output verified

**Scope:**
- *In:* View snapshot tests, fixture graph, JSON output tests
- *Out:* Visual regression tests, performance benchmarks

**Complexity:** ~250 LoC
**Est. Human Hours:** 4

**Definition of Done:**
- All tests green
- Fixtures committed
- Snapshot updates require explicit approval

**Blocked By:** PRI-001, PRI-003, PRI-004, PRI-005, PRI-006
**Blocking:** —

**Test Plan:**
- *Golden Path:* Each view renders expected output from fixture
- *Failure Modes:* Test detects unintentional output changes
- *Edge Cases:* Empty graph views, graph with only one node type
- *Fuzz/Stress:* N/A

---

# Milestone 4: WATCHTOWER

> **"You can't manage what you can't measure. WATCHTOWER gives you the numbers."**

**Goal:** A single command that tells you the health of your knowledge graph.

**Refs:** #180

**Milestone Dependencies:** BEDROCK (node queries), PRISM (view rendering for blockers)

---

## Feature WTC-STATUS: Status Command

### Task WTC-001: Implement `git mind status` Command

**User Story:** As a project lead, I want to run `git mind status` and instantly see the health of my knowledge graph — node counts, edge counts, blockers, and quality signals.

**Requirements:**
- Display: total nodes, total edges
- Display: nodes by prefix (table)
- Display: edges by type (table)
- Display: blocked items count
- Display: low-confidence edges count (confidence < 0.5)
- Display: orphan nodes count (nodes with 0 edges)

**Acceptance Criteria:**
- [ ] `git mind status` shows all metrics
- [ ] Output is human-readable with clear section headers
- [ ] Empty graph shows zeros, not errors
- [ ] Metrics are accurate (verified by tests)

**Scope:**
- *In:* Status command, metric computation, formatted output
- *Out:* Historical tracking, trend analysis, alerts

**Complexity:** ~120 LoC
**Est. Human Hours:** 4

**Definition of Done:**
- Command works end-to-end
- All metrics verified against fixture
- Help text updated

**Blocked By:** BDK-003, BDK-005
**Blocking:** WTC-002, PRV-003

**Test Plan:**
- *Golden Path:* Known fixture → verify all counts match
- *Failure Modes:* Empty graph → all zeros, uninitialized graph → helpful error
- *Edge Cases:* Graph with only nodes (no edges), graph with only edges (implied nodes)
- *Fuzz/Stress:* Graph with 1000 nodes, 5000 edges → verify status completes in <2s

---

### Task WTC-002: Status Command --json Flag

**User Story:** As a CI pipeline, I want `git mind status --json` so that I can programmatically check graph health.

**Requirements:**
- JSON output includes all metrics from WTC-001
- Schema: `{ totals: { nodes, edges }, byPrefix: {...}, byType: {...}, blocked, lowConfidence, orphans }`
- JSON is valid and parseable by `jq`

**Acceptance Criteria:**
- [ ] `git mind status --json` outputs valid JSON
- [ ] JSON schema matches documented format
- [ ] `git mind status --json | jq .totals.nodes` returns correct count

**Scope:**
- *In:* JSON output format
- *Out:* Other output formats (YAML, CSV)

**Complexity:** ~30 LoC
**Est. Human Hours:** 1

**Definition of Done:**
- JSON output works
- Schema documented
- Parseable by jq

**Blocked By:** WTC-001
**Blocking:** PRV-003

**Test Plan:**
- *Golden Path:* Parse JSON output, verify all fields present
- *Failure Modes:* N/A (if status works, JSON wrapping is trivial)
- *Edge Cases:* Empty graph → valid JSON with zeros
- *Fuzz/Stress:* N/A

---

## Feature WTC-METRICS: Graph Metrics

### Task WTC-003: Node Metrics by Prefix

**User Story:** As a user, I want to see how many nodes exist per prefix so that I understand the graph's composition.

**Requirements:**
- Count nodes by prefix
- Sort by count descending
- Show percentage of total for each prefix

**Acceptance Criteria:**
- [ ] Prefix breakdown shown in status output
- [ ] Percentages sum to ~100% (rounding)
- [ ] Prefixes with 0 nodes omitted from display

**Scope:**
- *In:* Prefix counting, percentage calculation
- *Out:* Historical comparisons, growth tracking

**Complexity:** ~40 LoC
**Est. Human Hours:** 1

**Definition of Done:**
- Metrics accurate against fixture
- Integrated into status output

**Blocked By:** BDK-003
**Blocking:** WTC-005

**Test Plan:**
- *Golden Path:* Known fixture → verify counts match
- *Failure Modes:* No nodes → empty table
- *Edge Cases:* All nodes same prefix, each node different prefix
- *Fuzz/Stress:* N/A

---

### Task WTC-004: Edge Metrics & Quality Signals

**User Story:** As a project lead, I want to see edge quality signals (low confidence, blocked items) so that I can prioritize graph curation.

**Requirements:**
- Count edges by type
- Identify low-confidence edges (< 0.5)
- Identify blocked items (nodes with incoming `blocks` edges)
- Identify orphan nodes (0 total edges)

**Acceptance Criteria:**
- [ ] Edge type breakdown shown in status
- [ ] Low-confidence edges listed with current confidence
- [ ] Blocked items count shown
- [ ] Orphan nodes count shown

**Scope:**
- *In:* Edge counting, quality signal detection
- *Out:* Confidence decay, automatic remediation suggestions

**Complexity:** ~60 LoC
**Est. Human Hours:** 2

**Definition of Done:**
- All quality signals computed correctly
- Integrated into status output

**Blocked By:** BDK-003
**Blocking:** WTC-005

**Test Plan:**
- *Golden Path:* Fixture with mix of high/low confidence → verify counts
- *Failure Modes:* No edges → all zeros
- *Edge Cases:* All edges low confidence, no blocked items, all nodes orphaned
- *Fuzz/Stress:* N/A

---

## Feature WTC-TEST: Dashboard Test Suite

### Task WTC-005: Status Output Test Suite

**User Story:** As a maintainer, I want status output verified against a known fixture so that metrics are trustworthy.

**Requirements:**
- Test with known fixture graph
- Verify every metric count
- Test both human and JSON output
- Minimum 10 test cases

**Acceptance Criteria:**
- [ ] Test file: `test/status.test.js`
- [ ] All metric counts verified
- [ ] JSON output schema verified
- [ ] All tests pass in CI

**Scope:**
- *In:* Status output tests
- *Out:* Performance benchmarks, visual tests

**Complexity:** ~150 LoC
**Est. Human Hours:** 2

**Definition of Done:**
- All tests green
- Every metric verified

**Blocked By:** WTC-001, WTC-003, WTC-004
**Blocking:** —

**Test Plan:**
- *Golden Path:* Known fixture → expected metrics
- *Failure Modes:* Test detects count mismatches
- *Edge Cases:* Empty graph, single-node graph
- *Fuzz/Stress:* N/A

---

# Milestone 5: PROVING GROUND

> **"Theory meets reality. We eat our own dogfood — in the Echo repo."**

**Goal:** Validate that git-mind can answer 5 real project management questions about the Echo ecosystem in under 60 seconds total.

**Refs:** #180

**Milestone Dependencies:** BEDROCK, INTAKE, PRISM, WATCHTOWER (all prior milestones)

---

## Feature PRV-SEED: Echo Seed

### Task PRV-001: Create Echo Project YAML Seed File

**User Story:** As a validator, I want a YAML file that represents the Echo project's structure so that I can seed git-mind with real data.

**Requirements:**
- Model Echo's milestones, specs, ADRs, crates, and issues
- Include at least: 5 milestones, 10 specs, 5 ADRs, 15 crates, 20 issues
- Include edges: implements, blocks, depends-on, documents, belongs-to
- Include realistic confidence scores
- Schema version 1

**Acceptance Criteria:**
- [ ] File: `docs/echo-seed.yaml`
- [ ] Validates against schema (version 1)
- [ ] Represents real Echo project structure (not fake data)
- [ ] Import succeeds with `git mind import docs/echo-seed.yaml`

**Scope:**
- *In:* YAML seed file with realistic Echo data
- *Out:* Automated scraping from Echo repos, live sync

**Complexity:** ~400 LoC (YAML)
**Est. Human Hours:** 4

**Definition of Done:**
- Seed file committed
- Import succeeds
- Data represents real Echo structure

**Blocked By:** INT-003, INT-004
**Blocking:** PRV-002, PRV-003

**Test Plan:**
- *Golden Path:* Import seed → verify node/edge counts match expected
- *Failure Modes:* N/A (seed file is hand-crafted, validated before commit)
- *Edge Cases:* N/A
- *Fuzz/Stress:* N/A

---

### Task PRV-002: Import Seed into Echo Repo

**User Story:** As a validator, I want the seed data imported into the Echo repo's git-mind graph so that I can run queries against it.

**Requirements:**
- Run `git mind init` in Echo repo
- Run `git mind import docs/echo-seed.yaml`
- Verify import report matches expected counts
- Checkpoint the graph

**Acceptance Criteria:**
- [ ] Graph initialized in Echo repo
- [ ] Import report shows expected counts
- [ ] `git mind status` shows correct totals
- [ ] Graph persists across sessions

**Scope:**
- *In:* Import execution, verification
- *Out:* Automated seeding CI, graph updates

**Complexity:** ~20 LoC (scripts/commands)
**Est. Human Hours:** 1

**Definition of Done:**
- Import complete and verified
- Status shows correct counts

**Blocked By:** PRV-001, INT-003
**Blocking:** PRV-003

**Test Plan:**
- *Golden Path:* Import → status → verify
- *Failure Modes:* Import errors → fix seed file
- *Edge Cases:* Re-import (idempotency check)
- *Fuzz/Stress:* N/A

---

## Feature PRV-VALIDATE: Validation Suite

### Task PRV-003: Answer 5 Echo Questions via CLI

**User Story:** As a validator, I want to answer 5 specific project management questions using only git-mind CLI commands in under 60 seconds total.

**Requirements:**

The 5 questions:
1. **What blocks M2?** → `git mind view blockers` filtered for milestone:M2
2. **Which ADRs lack implementation?** → `git mind view traceability` filtered for ADR nodes
3. **Which crates are unlinked to specs?** → `git mind nodes --prefix crate` + verify edges
4. **What should a new engineer read first?** → `git mind view onboarding`
5. **What's low-confidence and needs review?** → `git mind view suggestions` + `git mind status`

**Acceptance Criteria:**
- [ ] Each question answered with a single CLI command (or pipeline)
- [ ] Answers are correct (verified against known truth)
- [ ] Total time for all 5 < 60 seconds
- [ ] Commands documented in transcript

**Scope:**
- *In:* 5 specific questions, CLI commands, timing
- *Out:* Interactive exploration, ad-hoc queries, AI-assisted answers

**Complexity:** ~50 LoC (script/transcript)
**Est. Human Hours:** 3

**Definition of Done:**
- All 5 questions answered correctly
- Total time under 60s
- Transcript committed

**Blocked By:** PRV-002, BDK-005, WTC-001, PRI-003, PRI-004, PRI-005, PRI-006
**Blocking:** PRV-004

**Test Plan:**
- *Golden Path:* Run all 5 commands → verify answers
- *Failure Modes:* Wrong answer → investigate and fix view/query logic
- *Edge Cases:* N/A (questions are fixed)
- *Fuzz/Stress:* Time all 5 sequentially, verify total < 60s

---

### Task PRV-004: Validate Answers Against Known Truth

**User Story:** As a validator, I want to verify that the CLI answers match independently verified truth so that we can trust git-mind's output.

**Requirements:**
- For each of the 5 questions, establish ground truth independently
- Compare CLI output to ground truth
- Document discrepancies and resolutions
- All 5 must match

**Acceptance Criteria:**
- [ ] Ground truth documented for each question
- [ ] CLI output matches ground truth for all 5
- [ ] Any discrepancies resolved and documented

**Scope:**
- *In:* Verification of 5 specific answers
- *Out:* Automated verification CI, regression testing

**Complexity:** ~30 LoC (documentation)
**Est. Human Hours:** 2

**Definition of Done:**
- All 5 answers verified
- Discrepancies (if any) resolved

**Blocked By:** PRV-003
**Blocking:** PRV-005

**Test Plan:**
- *Golden Path:* All answers match
- *Failure Modes:* Mismatch → bug report and fix
- *Edge Cases:* N/A
- *Fuzz/Stress:* N/A

---

## Feature PRV-DOCS: Demo Documentation

### Task PRV-005: Record and Commit Demo Transcript

**User Story:** As a stakeholder, I want a demo transcript showing git-mind answering real questions so that I can see the tool in action.

**Requirements:**
- Full terminal session transcript
- Includes timing for each command
- Shows both command and output
- Includes commentary explaining what each answer means
- Committed as `docs/dogfood-session.md`

**Acceptance Criteria:**
- [ ] Transcript committed at `docs/dogfood-session.md`
- [ ] All 5 questions and answers included
- [ ] Timing shown for each command
- [ ] Commentary explains results

**Scope:**
- *In:* Terminal transcript, commentary
- *Out:* Video recording, interactive demo, web demo

**Complexity:** ~200 LoC (markdown)
**Est. Human Hours:** 2

**Definition of Done:**
- Transcript committed and reviewed
- Results match validation (PRV-004)

**Blocked By:** PRV-004
**Blocking:** —

**Test Plan:**
- *Golden Path:* N/A (documentation)
- *Failure Modes:* N/A
- *Edge Cases:* N/A
- *Fuzz/Stress:* N/A

---

# Milestone 6: ORACLE

> **"The graph knows patterns you haven't seen yet. ORACLE finds them."**

**Goal:** Add AI-powered edge suggestions, interactive review, and graph integrity checking.

**Refs:** Future

**Milestone Dependencies:** PROVING GROUND (validated graph must exist first)

---

## Feature ORC-SUGGEST: AI Suggestions

### Task ORC-001: Implement `git mind suggest --ai` with LLM Integration

**User Story:** As a developer, I want AI-suggested relationships between my code and specs so that I discover connections I've missed.

**Requirements:**
- Analyze git diff / recent commits for context
- Use LLM to suggest new edges (source, target, type, confidence, rationale)
- Present suggestions for human review (don't auto-commit)
- Support `--context <sha-range>` to limit analysis scope
- Configurable LLM provider (environment variable)

**Acceptance Criteria:**
- [ ] `git mind suggest --ai` produces edge suggestions
- [ ] Suggestions include rationale
- [ ] Suggestions are not committed automatically
- [ ] Context window is configurable

**Scope:**
- *In:* LLM integration, suggestion generation, context extraction
- *Out:* Auto-commit suggestions, training/fine-tuning, custom models

**Complexity:** ~300 LoC
**Est. Human Hours:** 8

**Definition of Done:**
- Suggestions generated from real code context
- Rationale provided for each suggestion
- No auto-commits

**Blocked By:** PRV-003
**Blocking:** ORC-002, ORC-003

**Test Plan:**
- *Golden Path:* Run against known repo → verify suggestions are relevant
- *Failure Modes:* No LLM key → helpful error, LLM timeout → graceful retry/fail
- *Edge Cases:* Empty diff → "no suggestions", very large diff → context truncation
- *Fuzz/Stress:* Run against repo with 1000 files → verify completes in <30s

---

### Task ORC-002: Context Extraction from Code & Commits

**User Story:** As the AI subsystem, I need structured context from the codebase so that I can make relevant suggestions.

**Requirements:**
- Extract file-level metadata (path, language, size, last modified)
- Extract commit-level metadata (message, files changed, author, date)
- Extract existing graph context (what nodes/edges exist near these files)
- Build LLM prompt from extracted context

**Acceptance Criteria:**
- [ ] Context extraction produces structured data from git state
- [ ] Context includes existing graph relationships
- [ ] Prompt building produces well-formed LLM input
- [ ] Context size is bounded (truncation for large repos)

**Scope:**
- *In:* Context extraction, prompt building, size management
- *Out:* Code parsing/AST analysis, cross-repo context

**Complexity:** ~200 LoC
**Est. Human Hours:** 5

**Definition of Done:**
- Context extraction works against real repo
- Prompts are well-formed

**Blocked By:** ORC-001
**Blocking:** ORC-003

**Test Plan:**
- *Golden Path:* Extract context from test repo → verify structure
- *Failure Modes:* Empty repo → empty context, binary files → skipped
- *Edge Cases:* Renamed files, deleted files, merge commits
- *Fuzz/Stress:* Large repo (1000 files) → verify extraction <5s

---

## Feature ORC-REVIEW: Interactive Review

### Task ORC-003: Implement `git mind review` Interactive Flow

**User Story:** As a developer, I want to interactively review suggested edges — accept, reject, or adjust each one — so that the graph stays curated by humans.

**Requirements:**
- Present pending suggestions one at a time
- Options: accept (write to graph), reject (discard), adjust (modify and accept), skip (defer)
- Track review decisions in git-warp provenance
- `--batch` mode for non-interactive accept/reject

**Acceptance Criteria:**
- [ ] Interactive review loop works in terminal
- [ ] Accepted edges written to graph
- [ ] Rejected edges recorded but not written
- [ ] Adjusted edges written with modifications
- [ ] Review history queryable

**Scope:**
- *In:* Interactive review, decision tracking, batch mode
- *Out:* Web UI, collaborative review, review assignments

**Complexity:** ~250 LoC
**Est. Human Hours:** 6

**Definition of Done:**
- Interactive review works
- Decisions tracked
- Batch mode works

**Blocked By:** ORC-001, ORC-002
**Blocking:** ORC-005

**Test Plan:**
- *Golden Path:* Suggest → review → accept → verify edge in graph
- *Failure Modes:* No pending suggestions → "nothing to review", interrupt during review → no partial writes
- *Edge Cases:* Review same suggestion twice → idempotent, adjust confidence to 0 → effectively reject
- *Fuzz/Stress:* Review 50 suggestions in batch mode → verify all processed

---

### Task ORC-004: Accept/Reject/Adjust Edge Workflow

**User Story:** As a reviewer, I want my decisions to have clear semantics so that the graph reflects intentional curation.

**Requirements:**
- Accept: write edge with confidence from suggestion
- Reject: record as rejected, never re-suggest same edge
- Adjust: modify type/confidence/rationale, then write
- Skip: defer to next review session
- All decisions timestamped and attributed

**Acceptance Criteria:**
- [ ] Accept writes edge immediately
- [ ] Reject prevents re-suggestion
- [ ] Adjust allows modification before writing
- [ ] Skip defers without action
- [ ] All decisions have timestamps and attribution

**Scope:**
- *In:* Decision semantics, de-duplication, attribution
- *Out:* Undo/redo, decision appeals, team review

**Complexity:** ~100 LoC
**Est. Human Hours:** 3

**Definition of Done:**
- All four decision types implemented
- De-duplication working (no re-suggest rejected)

**Blocked By:** ORC-003
**Blocking:** ORC-005

**Test Plan:**
- *Golden Path:* Each decision type → verify expected outcome
- *Failure Modes:* Decision on nonexistent suggestion → error
- *Edge Cases:* Reject then re-suggest manually → allowed (only auto-suggest blocked)
- *Fuzz/Stress:* 100 decisions in sequence → verify all recorded

---

## Feature ORC-LEARN: Curation Loop

### Task ORC-005: Review History Provenance in git-warp

**User Story:** As the AI system, I want access to past review decisions so that future suggestions improve over time.

**Requirements:**
- Store review decisions in git-warp (not just in edges)
- Query history: "what did the user accept/reject for this file/node?"
- Use history to weight future suggestions
- Provenance is append-only (decisions are never deleted)

**Acceptance Criteria:**
- [ ] Review decisions persisted in git-warp
- [ ] History queryable by node, by file, by time range
- [ ] Suggestions incorporate past decisions
- [ ] History survives graph checkpoints

**Scope:**
- *In:* Decision storage, history queries, suggestion weighting
- *Out:* ML model training, cross-user learning, decision analytics

**Complexity:** ~150 LoC
**Est. Human Hours:** 5

**Definition of Done:**
- Provenance stored and queryable
- Suggestions improve with feedback

**Blocked By:** ORC-003, ORC-004
**Blocking:** —

**Test Plan:**
- *Golden Path:* Reject edge → re-run suggest → verify not re-suggested
- *Failure Modes:* Corrupted history → graceful degradation (suggest without history)
- *Edge Cases:* Conflicting decisions (accept then reject same edge), empty history
- *Fuzz/Stress:* 1000 decisions → verify query performance <100ms

---

## Feature ORC-DOCTOR: Integrity Checks

### Task ORC-006: Implement `git mind doctor` Command

**User Story:** As a maintainer, I want to run `git mind doctor` to find graph problems (dangling edges, orphans, duplicates) so that I can fix data quality issues.

**Requirements:**
- Check for dangling edges (source or target doesn't exist as a node)
- Check for orphan milestones (milestones with no tasks)
- Check for duplicate IDs
- Check for low-confidence edges (< 0.3)
- Report all issues with severity (error, warning, info)

**Acceptance Criteria:**
- [ ] `git mind doctor` runs all checks
- [ ] Issues reported with severity levels
- [ ] Exit code 0 if no errors, 1 if errors found
- [ ] `--json` output supported
- [ ] `--fix` flag auto-fixes safe issues (orphan cleanup, etc.)

**Scope:**
- *In:* Integrity checks, issue reporting, auto-fix for safe issues
- *Out:* Graph repair for complex issues, schema migration

**Complexity:** ~200 LoC
**Est. Human Hours:** 5

**Definition of Done:**
- All checks implemented
- Auto-fix for safe issues
- Test coverage

**Blocked By:** BDK-003, BDK-002
**Blocking:** ORC-007

**Test Plan:**
- *Golden Path:* Clean graph → no issues, dirty graph → all issues found
- *Failure Modes:* Uninitialized graph → helpful error
- *Edge Cases:* Graph with only dangling edges, graph with only orphans
- *Fuzz/Stress:* Graph with 100 intentional issues → verify all detected in <2s

---

### Task ORC-007: Dangling Edge, Orphan, and Duplicate Detection

**User Story:** As a developer, I want specific detection logic for each integrity issue type so that the doctor command is thorough.

**Requirements:**
- Dangling edge: edge references node not in graph
- Orphan milestone: `milestone:*` node with no `belongs-to` children
- Orphan node: any node with zero edges
- Duplicate ID: same node ID appears with conflicting properties
- Each detector returns `{ type, severity, message, affected }` objects

**Acceptance Criteria:**
- [ ] Each detector implemented as separate function
- [ ] Detectors composable (run individually or together)
- [ ] Affected items identified in reports
- [ ] All detectors have test coverage

**Scope:**
- *In:* Individual detectors, composability
- *Out:* Custom detector plugins, detector configuration

**Complexity:** ~150 LoC
**Est. Human Hours:** 4

**Definition of Done:**
- All detectors implemented
- Test suite for each detector

**Blocked By:** ORC-006
**Blocking:** —

**Test Plan:**
- *Golden Path:* Inject known issue → verify detection
- *Failure Modes:* False positive → fix detection logic
- *Edge Cases:* Node that's both orphan and has dangling edges, milestone that's also a task
- *Fuzz/Stress:* Graph with 500 nodes, inject 50 issues → verify all 50 detected

---

# Milestone 7: NEXUS

> **"The graph escapes the terminal. NEXUS connects git-mind to the world."**

**Goal:** GitHub Actions integration, PR-level suggestions, markdown import, and multi-repo federation.

**Refs:** Future

**Milestone Dependencies:** ORACLE (AI suggestions required for PR integration)

---

## Feature NEX-ACTION: GitHub Action

### Task NEX-001: Create GitHub Action for PR Suggestions

**User Story:** As a team lead, I want git-mind to automatically suggest edges on every PR so that the knowledge graph stays current with code changes.

**Requirements:**
- GitHub Action runs on `pull_request` events
- Runs `git mind suggest --ai --context $PR_SHA_RANGE`
- Posts suggestions as PR comment
- Configurable via `.github/git-mind.yml`

**Acceptance Criteria:**
- [ ] Action published to GitHub Marketplace
- [ ] Runs on PR open and update
- [ ] Posts formatted comment with suggestions
- [ ] Configurable (enable/disable, edge types, confidence threshold)

**Scope:**
- *In:* GitHub Action, PR comments, configuration
- *Out:* Other CI platforms, auto-merge suggestions, PR blocking

**Complexity:** ~200 LoC
**Est. Human Hours:** 6

**Definition of Done:**
- Action runs in test repo
- Comments appear on PRs
- Configuration works

**Blocked By:** ORC-001
**Blocking:** NEX-002

**Test Plan:**
- *Golden Path:* Open PR → Action runs → comment appears with suggestions
- *Failure Modes:* No API key → skip with warning, Action timeout → graceful exit
- *Edge Cases:* PR with no code changes (docs only), PR with 100+ files
- *Fuzz/Stress:* Trigger on 10 PRs simultaneously → verify no race conditions

---

### Task NEX-002: PR Reviewer Edge Display

**User Story:** As a PR reviewer, I want to see suggested knowledge graph edges alongside the code diff so that I can understand the impact of changes.

**Requirements:**
- Format suggestions as a clear table in PR comment
- Group by file changed
- Show: suggested edge, confidence, rationale
- Include "accept" and "reject" buttons (via comment reactions or slash commands)

**Acceptance Criteria:**
- [ ] Suggestions grouped by file
- [ ] Table format with all fields
- [ ] Accept/reject mechanism works
- [ ] Reactions processed by follow-up Action run

**Scope:**
- *In:* PR comment formatting, accept/reject via reactions
- *Out:* Inline diff comments, GitHub Check annotations, web UI

**Complexity:** ~150 LoC
**Est. Human Hours:** 4

**Definition of Done:**
- Formatted comments appearing on PRs
- Accept/reject mechanism working

**Blocked By:** NEX-001
**Blocking:** —

**Test Plan:**
- *Golden Path:* Open PR → suggestions appear → react to accept → edge created
- *Failure Modes:* No suggestions → no comment posted (not empty comment)
- *Edge Cases:* Multiple reviewers react differently, reaction on old comment
- *Fuzz/Stress:* PR with 50 suggestions → verify comment doesn't exceed GitHub limits

---

## Feature NEX-MARKDOWN: Markdown Import

### Task NEX-003: Import from Markdown Frontmatter

**User Story:** As a developer, I want git-mind to extract knowledge from markdown frontmatter (YAML headers) so that existing documentation automatically feeds the graph.

**Requirements:**
- Scan `.md` files for YAML frontmatter
- Extract graph-relevant fields: `id`, `type`, `relates-to`, `implements`, `depends-on`
- Create nodes and edges from extracted data
- `git mind import --from-markdown <glob>` command

**Acceptance Criteria:**
- [ ] Frontmatter parsing works for standard YAML headers
- [ ] Nodes created with `doc:` prefix
- [ ] Edges created from relationship fields
- [ ] Glob pattern for file selection works
- [ ] Idempotent (re-import safe)

**Scope:**
- *In:* Frontmatter parsing, node/edge creation, glob-based file selection
- *Out:* Markdown body parsing, link extraction, heading analysis

**Complexity:** ~200 LoC
**Est. Human Hours:** 5

**Definition of Done:**
- Markdown import works end-to-end
- Idempotent
- Test coverage

**Blocked By:** INT-001, INT-002
**Blocking:** NEX-004

**Test Plan:**
- *Golden Path:* Markdown with frontmatter → import → verify nodes and edges
- *Failure Modes:* No frontmatter → skip file, invalid frontmatter → report and skip
- *Edge Cases:* Multiple documents in one file, frontmatter with non-graph fields (ignored)
- *Fuzz/Stress:* Scan 500 markdown files → verify import <10s

---

## Feature NEX-FEDERATION: Multi-Repo Federation

### Task NEX-004: Cross-Repo Edge Protocol

**User Story:** As an architect working across multiple repos, I want to create edges that span repositories so that the knowledge graph reflects cross-repo dependencies.

**Requirements:**
- Edge source/target can include repo qualifier: `repo:owner/name:prefix:id`
- Remote edges stored locally with remote qualifier
- `git mind link --remote <repo>` flag
- Remote resolution: verify remote node exists (optional, `--no-verify`)

**Acceptance Criteria:**
- [ ] Cross-repo edge syntax works
- [ ] Remote edges stored with full qualifier
- [ ] `--no-verify` skips remote existence check
- [ ] Remote edges visible in views

**Scope:**
- *In:* Edge syntax extension, remote qualifiers, local storage of remote edges
- *Out:* Remote graph queries, cross-repo graph merge, federated views

**Complexity:** ~200 LoC
**Est. Human Hours:** 6

**Definition of Done:**
- Cross-repo edges creatable and queryable
- Views include remote edges

**Blocked By:** BDK-002, NEX-003
**Blocking:** NEX-005

**Test Plan:**
- *Golden Path:* Link local node to remote node → verify edge stored with qualifier
- *Failure Modes:* Invalid remote syntax → helpful error, remote repo not found → error (unless --no-verify)
- *Edge Cases:* Same node ID in different repos, cross-repo circular dependency
- *Fuzz/Stress:* 100 cross-repo edges → verify query performance

---

### Task NEX-005: Multi-Repo Graph Merge

**User Story:** As a team lead, I want to merge knowledge graphs from multiple repos into a unified view so that I can see cross-cutting concerns.

**Requirements:**
- `git mind merge --from <repo-path>` imports another repo's graph
- Nodes are prefixed with repo qualifier to avoid ID collisions
- Edges between repos preserved
- Merge is additive (never deletes)

**Acceptance Criteria:**
- [ ] Merge imports nodes with repo qualifier
- [ ] Cross-repo edges resolved
- [ ] No ID collisions
- [ ] Merged graph queryable with standard commands

**Scope:**
- *In:* Graph merge, ID namespacing, edge resolution
- *Out:* Continuous sync, conflict resolution, merge policies

**Complexity:** ~200 LoC
**Est. Human Hours:** 6

**Definition of Done:**
- Merge works between two repos
- Merged graph is correct and queryable

**Blocked By:** NEX-004
**Blocking:** —

**Test Plan:**
- *Golden Path:* Two repos → merge → verify unified graph
- *Failure Modes:* Missing repo → error, incompatible schema versions → error
- *Edge Cases:* Merge with overlapping node IDs, merge empty repo
- *Fuzz/Stress:* Merge 5 repos with 100 nodes each → verify result integrity

---

## Feature NEX-EXPORT: Round-Trip Export

### Task NEX-006: Implement `git mind export`

**User Story:** As a user, I want to export my graph to YAML or JSON so that I can back it up, share it, or process it with other tools.

**Requirements:**
- `git mind export --format yaml` → export entire graph as YAML
- `git mind export --format json` → export as JSON
- Export format matches import format (round-trip compatible)
- `git mind export --prefix <prefix>` → export subset

**Acceptance Criteria:**
- [ ] YAML export produces valid, re-importable YAML
- [ ] JSON export produces valid JSON
- [ ] Round-trip: export → import → export → compare (identical)
- [ ] Prefix filtering works

**Scope:**
- *In:* YAML and JSON export, prefix filtering, round-trip compatibility
- *Out:* GraphML, DOT, Mermaid export formats (future)

**Complexity:** ~150 LoC
**Est. Human Hours:** 4

**Definition of Done:**
- Export works for both formats
- Round-trip verified
- Prefix filtering works

**Blocked By:** INT-001
**Blocking:** —

**Test Plan:**
- *Golden Path:* Seed graph → export → re-import → verify identical
- *Failure Modes:* Empty graph → valid empty export
- *Edge Cases:* Nodes with special characters in IDs, very large graph export
- *Fuzz/Stress:* Export graph with 1000 nodes, 5000 edges → verify <5s and file size reasonable

---

# Dependency DAG Summary

```
BEDROCK ──────────────────────────────────────────────────┐
  BDK-001 ──→ BDK-002 ──→ BDK-007                       │
  BDK-001 ──→ BDK-003 ──→ BDK-004 ──→ BDK-005 ──→ BDK-006  │
                           BDK-003 ──→ BDK-008           │
                           BDK-004 ──→ BDK-008           │
                                                         │
INTAKE ←── BEDROCK                                        │
  INT-001 ──→ INT-002 ──→ INT-008                        │
  INT-001 ──→ INT-003 ──→ INT-004                        │
  INT-001 ──→ INT-006 ──→ INT-007 ──→ INT-008           │
  INT-005 ──→ INT-008                                    │
                                                         │
PRISM ←── BEDROCK                                        │
  PRI-001 ──→ PRI-002 ──→ PRI-007                       │
  PRI-001 ──→ PRI-003 ──→ PRI-007                       │
  PRI-001 ──→ PRI-004 ──→ PRI-007                       │
  PRI-001 ──→ PRI-005 ──→ PRI-007                       │
  PRI-001 ──→ PRI-006 ──→ PRI-007                       │
                                                         │
WATCHTOWER ←── BEDROCK, PRISM                            │
  WTC-001 ──→ WTC-002                                   │
  WTC-003 ──→ WTC-005                                   │
  WTC-004 ──→ WTC-005                                   │
                                                         │
PROVING GROUND ←── ALL ABOVE                              │
  PRV-001 ──→ PRV-002 ──→ PRV-003 ──→ PRV-004 ──→ PRV-005│
                                                         │
ORACLE ←── PROVING GROUND                                │
  ORC-001 ──→ ORC-002 ──→ ORC-003 ──→ ORC-004 ──→ ORC-005│
  ORC-006 ──→ ORC-007                                   │
                                                         │
NEXUS ←── ORACLE                                         │
  NEX-001 ──→ NEX-002                                   │
  NEX-003 ──→ NEX-004 ──→ NEX-005                       │
  NEX-006 (independent)                                  │
```

---

# Key Design Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Node ID format | `prefix:identifier` | Enables prefix-based filtering and view matching |
| Prefix casing | lowercase prefix, case-preserving identifier | `milestone:BEDROCK` not `Milestone:bedrock` |
| Edge uniqueness | `(source, target, type)` tuple | Re-adding same edge updates props, doesn't duplicate |
| Import semantics | Node props: shallow merge; Edge: update if exists | Idempotent by default |
| Import failure mode | All-or-nothing (default) | Validate everything before writing anything |
| Schema versioning | `version: 1` required, unknown → hard error | Fail closed, not best-effort |
| View definitions | Declarative config objects | Adding a view = config patch, not code change |
| Output format | Human-readable default, `--json` flag | Both humans and machines are first-class consumers |
| AI suggestions | Never auto-commit | Humans curate, machines suggest |
| Cross-repo edges | Repo-qualified node IDs | Namespacing prevents collisions |

---

# Milestone 9: IRONCLAD — GA Hardening

> **"You're done with more features. Now win trust."**

**Mission:** Convert git-mind from "feature complete" to "production undeniable."
**Non-goal:** New features unless they reduce risk or adoption friction.
**Success metric:** New team can install, model, validate, and enforce policy in under 60 minutes.

---

## Phase A — Contract Lockdown (P0)

### A1) CLI JSON Schema Contracts (#205)
- JSON Schema files for every `--json` command output
- `schemaVersion` field in every JSON output
- CI validates all `--json` output against schemas

### A2) API Stability Surface (#206)
- Public API export audit + snapshot
- Automated API diff check in CI
- Deprecation protocol: warning + migration note

### A3) Error Taxonomy + Exit Codes (#207)
- Structured errors: `GMIND_E_*` error codes
- Consistent exit code table
- JSON mode includes `errorCode`, `hint`, `docsRef`

---

## Phase B — Reliability Gauntlet (P0)

### B1) Cross-OS CI Matrix (#208)
- ubuntu-latest, macos-latest, windows-latest
- Node active LTS bands
- smoke / integration / long-run suite split

### B2) Fuzz & Adversarial Inputs (#209)
- Targets: YAML import, frontmatter parser, directive parser, validators
- Zero unhandled exceptions across corpus

### B3) Corruption & Recovery Drills (#210)
- Missing refs, malformed payloads, partial merges, stale locks
- `git mind doctor --strict`
- Deterministic behavior, no silent data mutation

---

## Phase C — Data Safety + Atomicity (P0)

### C1) Transactional Writes (#211)
- Write to temp ref → verify checksum → atomic ref swap
- No partial write states observable

### C2) Backup / Restore (#212)
- `git mind backup [--out file]`
- `git mind restore <file> [--dry-run]`

---

## Phase D — Adoption Engine (P1)

### D1) 30-Minute Zero-to-Value Tutorial (#213)
- Fresh user reaches: imported graph, usable view, doctor clean, CI policy in <60 min

### D2) Opinionated Team Starter + CI Policy Gate (#214)
- `git mind init --preset engineering`
- Strict mode: fail on dangling edges, blocked tasks, low-confidence edges

---

## Phase E — Performance Envelope (P1/P2)

### E1) Benchmark Harness (#215)
- Small/medium/large graph bench packs
- p50/p95 latency, memory ceiling, cold vs warm
- Regressions >10% fail benchmark gate

---

## GA Release Candidate (#216)

**GA Checklist (must all be green):**
- [ ] All `--json` outputs schema-validated in CI
- [ ] Public API snapshot unchanged (or semver-major justified)
- [ ] Typed errors + stable exit codes everywhere
- [ ] Atomic writes proven under injected failures
- [ ] Doctor strict catches all known corruption classes
- [ ] Fresh-user tutorial validated by non-authors
- [ ] CI policy gate docs + working example
- [ ] Performance baseline + regression alarms active

---

## Do Not Cut
- JSON schema contracts
- Typed error codes
- Atomic writes
- Corruption drills

---

# Backlog (Unscheduled)

These items are not assigned to a milestone yet. They'll be scheduled based on user feedback and priorities.

### Extension persistence & ephemeral loading (deferred from M12 polish)

Two issues were filed during the M12 extension polish pass and intentionally deferred:

- **#261 — Ephemeral registration: extension add doesn't persist across invocations.**
  `git mind extension add <path>` registers for the current process only. The fix requires a persistence mechanism — a lockfile (`.git/git-mind/extensions.yaml`), graph-stored config, or git-config entries. Each option has different tradeoffs for portability, discoverability, and merge semantics. This also changes the CLI boot sequence for ALL commands (startup must load user extensions after built-ins), so it needs careful design.

- **#269 — `--extension <path>` flag for single-invocation loading.**
  A workaround for #261: load an extension for one command only (`git mind view my-view --extension ./ext.yaml`). Useful for CI/CD pipelines that inject custom domain logic. Deferred because this is cleaner to design after #261's persistence exists — the flag would be "like `extension add` but ephemeral", which is only meaningful once `add` is actually persistent.

**Recommended slot:** H2 (CONTENT + MATERIALIZATION) planning. Both issues naturally fall into the extension lifecycle story — persistence is a prerequisite for the extension marketplace vision (H4). Design the persistence mechanism during H2 kickoff, implement as the first H2 deliverable so that all subsequent extension work (content system extensions, materializer extensions) benefits from proper registration.

### Content system enhancements (from M13 VESSEL review)

- **`git mind content list`** — Query all nodes that have `_content.sha` properties. Currently there's no way to discover which nodes carry content without inspecting each one individually.
- **Binary content support** — Add base64 encoding for non-text MIME types. Currently the content system is text-only (UTF-8); non-UTF-8 blobs fail the integrity check by design. Requires reintroducing encoding metadata and updating `readContent()` to handle buffer round-trips.
- **`content meta --verify` flag** — Run the SHA integrity check without dumping the full content body. Useful for bulk health checks across all content-bearing nodes.

### Codebase hardening (from M13 VESSEL review)

- **Standardize all git subprocess calls to `execFileSync`** — `src/content.js` now uses `execFileSync` exclusively, but other modules (e.g. `processCommitCmd` in `commands.js`) still use `execSync` with string interpolation. Audit and migrate for consistency and defense-in-depth.

### Other backlog items

- `git mind onboarding` as a guided walkthrough (not just a view)
- Confidence decay over time (edges rot if not refreshed)
- View composition (combine multiple views)
- Wesley-generated typed graph accessors
- Mermaid diagram export (`git mind export --format mermaid`)
- GraphQL API for web frontends
- Real-time file watcher for automatic edge updates
- Git blame integration (who created this edge?)
- Edge provenance visualization

---

> **This roadmap is tracked in git-mind's own graph.** 
> The task DAG above is materialized as nodes and edges in this repo's git-mind graph.
> Run `git mind view roadmap` to see it live.
>
> Maintainers: update this file AND the graph when priorities shift or phases complete.
