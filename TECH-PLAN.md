# Technical Deep Dive: git-mind

> **"A knowledge graph that thinks alongside you."**

A technical analysis of git-mind's architecture, data model, runtime behavior, invariants, and roadmap trajectory. This is a reference artifact — not a changelog, not a tutorial.

---

## 1. The Big Idea

git-mind turns any Git repository into a **semantic knowledge graph**. Instead of only tracking *what* files exist and *when* they changed (which Git already does), git-mind captures *why* things are connected — which spec a file implements, what a module depends on, which task a commit addresses.

The key insight: **Git is already a graph database.** It has an immutable object store, a reflog, branches, merges, and distributed replication. git-mind exploits this by storing its knowledge graph *inside* Git's ref system using CRDTs, making the graph invisible to normal workflows but always present, versioned, and mergeable.

---

## 2. Storage Layer: git-warp

git-mind delegates persistence to **@git-stunts/git-warp** (v10.3.2), a CRDT graph database that uses Git's object store as its backend.

### How data gets stored

```
refs/warp/gitmind/writers/local     ← your patch chain (Git commits)
refs/warp/gitmind/writers/alice     ← another writer's patches
refs/warp/gitmind/checkpoints/latest ← snapshot for fast reload
```

Each graph mutation is a **patch** — a JSON blob committed to Git's object store. These commits point to Git's empty tree (`4b825dc...`), creating **zero files** in the working directory. They're invisible to `git status`, `git diff`, and all normal Git operations.

A patch looks like:

```json
{
  "ops": [
    { "op": "addNode", "id": "file:src/auth.js" },
    { "op": "addEdge", "from": "file:src/auth.js", "to": "spec:auth", "label": "implements" },
    { "op": "setProp", "id": "file:src/auth.js", "key": "type", "value": "file" }
  ],
  "writerId": "local",
  "tick": 42,
  "vv": { "local": 42 }
}
```

Each patch carries a **version vector** (`vv`) for causal ordering and a **tick** (Lamport logical clock) for property conflict resolution.

### CRDT Semantics

Two CRDT types make concurrent writes conflict-free:

| CRDT | Used for | Conflict resolution |
|------|----------|-------------------|
| **OR-Set** (Observed-Remove) | Nodes and edges | Add wins over concurrent remove |
| **LWW Register** (Last-Writer-Wins) | Properties | See tie-break rules below |

### LWW Tie-Break Hierarchy

When two writers concurrently set the same property, the following deterministic tie-break applies:

1. **Compare Lamport tick** (logical clock). Higher tick wins.
2. **If equal, compare writerId lexicographically.** `"bob"` > `"alice"` — deterministic across all replicas.
3. **If equal (same writer, same tick — shouldn't happen), compare patch hash.** The patch with the lexicographically greater hash wins.

This ensures **all replicas converge to identical state** regardless of delivery order. No hidden nondeterminism. No "it depends on who syncs first."

### Convergence vs. Correctness

**State convergence is guaranteed** — all replicas always reach the same materialized graph. The CRDTs ensure this mechanically.

**Domain correctness is not guaranteed by convergence alone.** If writer A sets `confidence: 0.9` and writer B concurrently sets `confidence: 0.2` on the same edge, LWW picks one deterministically — but neither writer's *intent* is preserved. Similarly, if two writers independently create `spec:auth` and `specs:auth`, the graph converges to having both nodes — even though that's semantic noise.

Domain correctness is governed by:
- The **validation layer** (prefix taxonomy, edge type constraints, self-edge prohibition)
- The **review workflow** (humans curate, machines suggest)
- **Prefix governance** (see §3)

Do not confuse "no merge conflicts" with "no data quality problems."

### Materialization

Reading the graph requires **materialization** — replaying all patches in causal order to build an in-memory graph. git-mind sets `autoMaterialize: true`, so this happens transparently.

| Operation | Complexity | Notes |
|-----------|-----------|-------|
| Add node/edge | O(1) | Append-only |
| Materialize | O(P) | P = total patches across all writers |
| Query (post-materialize) | O(N) | N = matching nodes |
| Checkpoint | O(S) | S = state size; creates snapshot |

### Scalability Escape Hatches

Materialization is O(P) where P is total patches. At small scale this is fine. At large scale:

- **Checkpoints** (`saveGraph()`) — Creates a snapshot. Future materializations replay only patches since the last checkpoint. This is the primary mitigation and is available today.
- **Checkpoint policy** — Not yet automated. Currently manual via `saveGraph()`. Planned: automatic checkpoint after N patches or on `git mind status`.
- **Patch compaction** — Not yet implemented. Planned: merge sequential patches from the same writer into a single consolidated patch.
- **Incremental materialization** — Not yet implemented. Planned: cache frontier hash + tick, only replay new patches since last materialization.
- **State cache keying** — Planned: key cache by `(writer frontier vector hash)` so identical state can skip re-materialization entirely.

Until these are implemented, the practical limit is ~10K patches before materialization latency becomes noticeable. Checkpoints should be created after bulk operations.

### Initialization Flow (`src/graph.js`)

```
initGraph(repoPath)
  → ShellRunnerFactory.create()        // from @git-stunts/plumbing
  → GitPlumbing({ runner })            // low-level Git operations
  → GitGraphAdapter({ plumbing })      // persistence adapter
  → WarpGraph.open({
      graphName: 'gitmind',
      persistence: adapter,
      writerId: 'local',
      autoMaterialize: true
    })
```

`WarpGraph.open()` is idempotent — init and load are the same call. `@git-stunts/plumbing` must be installed as a **direct** dependency (not hoisted from git-warp).

---

## 3. Data Model

### Node ID Grammar

Node IDs follow the `prefix:identifier` format. The prefix is always lowercase; the identifier is case-preserving.

**Regex:** `/^[a-z][a-z0-9-]*:[A-Za-z0-9._\/@-]+$/`
**Max length:** 256 characters

Nodes are **implicitly created** when an edge references them. There's no separate "create node" operation — if you link `file:a.js` to `spec:auth`, both nodes spring into existence.

### Prefix Taxonomy

**18 canonical prefixes:**

| Category | Prefixes |
|----------|----------|
| Project mgmt | `milestone`, `feature`, `task`, `issue`, `phase` |
| Documentation | `spec`, `adr`, `doc` |
| Architecture | `crate`, `module`, `pkg`, `file` |
| Abstract | `concept`, `decision` |
| People/tools | `person`, `tool` |
| Observability | `event`, `metric` |

**1 system prefix:** `commit` (auto-generated from commit hooks, reserved)

### Prefix Governance

Unknown prefixes produce a **warning** but are allowed. This lets the taxonomy grow organically. However, unchecked growth creates semantic noise (`spec:auth` vs `specs:auth` vs `requirement:auth` vs `req:auth`).

**Two-level governance model:**

1. **Runtime: soft allowlist (warn-on-unknown)**
   Current behavior in `validators.js`. Unknown prefixes pass validation but emit warnings. This prevents rejecting legitimate new prefixes while signaling drift.

2. **Project: canonical prefix registry (lint-in-CI)**
   Each project should declare its authorized prefixes. Planned: `.gitmind.yml` config with `allowedPrefixes` field. CI can treat unknown-prefix warnings as errors, preventing organic sprawl in team projects while allowing solo users full freedom.

This is "freedom with rails" — the runtime never blocks, but the CI can.

### Node ID Grammar vs. Federation Format

**Current grammar:** `prefix:identifier`
**Planned federation grammar (NEXUS milestone):** `repo:owner/name:prefix:identifier`

**Collision risk:** The current regex allows `/` in identifiers, so `repo:owner/name` would parse as prefix=`repo`, identifier=`owner/name` — a valid node ID under today's grammar. When federation arrives, `repo:owner/name:prefix:identifier` would be ambiguous: is the prefix `repo` or `repo:owner/name`?

**Resolution strategy:** Federation IDs use a **different syntax boundary** — not bare `:` but a qualified escape. Options under consideration:

1. **Double-colon delimiter:** `repo::owner/name::prefix:identifier` — federation layer uses `::`, node IDs use single `:`
2. **Bracket syntax:** `[owner/name]prefix:identifier` — repo qualifier is syntactically distinct
3. **URI scheme:** `gitmind://owner/name/prefix:identifier` — full URI for remote, bare `prefix:id` for local

The final choice will be made during NEXUS implementation. **The key invariant is:** local node IDs (`prefix:identifier`) are never syntactically ambiguous with federation-qualified IDs. The parser must be able to distinguish them without context.

Until federation ships, `repo:` is not in the canonical prefix list and will trigger an unknown-prefix warning if used manually.

### Edge Types

An edge is a directed, typed, scored connection: `source --[type]--> target`

**Edge uniqueness:** `(source, target, type)` tuple. Re-adding the same edge updates its properties rather than creating a duplicate.

**8 edge types:**

| Type | Direction | Meaning | Self-edge? |
|------|-----------|---------|-----------|
| `implements` | source implements target | Code fulfills a spec | Allowed |
| `augments` | source extends target | Enhancement or extension | Allowed |
| `relates-to` | source relates to target | General association | Allowed |
| `blocks` | source blocks target | Dependency ordering | **Forbidden** |
| `belongs-to` | source is part of target | Membership/containment | Allowed |
| `consumed-by` | source is consumed by target | Usage relationship | Allowed |
| `depends-on` | source depends on target | Dependency | **Forbidden** |
| `documents` | source documents target | Documentation link | Allowed |

### Edge Properties

| Property | Type | Default | Mutable? | Owner |
|----------|------|---------|----------|-------|
| `confidence` | float [0.0, 1.0] | 1.0 | Yes (user) | User/system |
| `createdAt` | ISO 8601 string | `new Date().toISOString()` | **No** (set once) | System |
| `rationale` | string or undefined | undefined | Yes (user) | User |

**Confidence semantics:**

| Score | Source | Meaning |
|-------|--------|---------|
| 1.0 | `git mind link` | Human-verified |
| 0.8 | Commit directive | Auto-created, high confidence |
| 0.3–0.5 | AI suggestion (future) | Needs review |
| 0.0 | Unknown | Should be reviewed or removed |

**Note:** `createdAt` is set on first creation and should be treated as immutable. LWW means a concurrent write *could* overwrite it, but this is considered a bug in the writer, not a feature of the system. Future: enforce immutability of `createdAt` at the validation layer.

---

## 4. System Invariants

These are the rules that must never be violated. If any of these break, the system is in a corrupt state.

| Invariant | Enforced by | Consequence of violation |
|-----------|------------|------------------------|
| Node IDs match `/^[a-z][a-z0-9-]*:[A-Za-z0-9._\/@-]+$/` | `validateNodeId()` | Edge creation rejected |
| Node IDs ≤ 256 characters | `validateNodeId()` | Edge creation rejected |
| Edge types are one of the 8 defined types | `validateEdgeType()` | Edge creation rejected |
| Confidence is a finite number in [0.0, 1.0] | `validateConfidence()` | Edge creation rejected |
| Self-edges forbidden for `blocks` and `depends-on` | `validateEdge()` | Edge creation rejected |
| System prefix `commit:` is reserved for hooks | Convention (not enforced) | Semantic confusion |
| Edge uniqueness by `(source, target, type)` | git-warp addEdge semantics | Update, not duplicate |
| Patches are append-only | git-warp CRDT | History is immutable |

---

## 5. Runtime Architecture

```
┌──────────────────────────────────────────────────────┐
│                   CLI Layer                           │
│  bin/git-mind.js → src/cli/commands.js               │
│                    src/cli/format.js                  │
├──────────────────────────────────────────────────────┤
│                  Domain Layer                         │
│  src/edges.js      — Edge CRUD with validation        │
│  src/views.js      — Observer views (filtered reads)  │
│  src/hooks.js      — Commit directive parser           │
│  src/validators.js — Runtime schema enforcement       │
├──────────────────────────────────────────────────────┤
│               Infrastructure Layer                    │
│  src/graph.js      — WarpGraph wrapper                │
├──────────────────────────────────────────────────────┤
│                 External Layer                        │
│  @git-stunts/git-warp    — CRDT graph on Git          │
│  @git-stunts/plumbing    — Low-level Git ops          │
├──────────────────────────────────────────────────────┤
│              Public API                               │
│  src/index.js      — Re-exports everything            │
└──────────────────────────────────────────────────────┘
```

### CLI Entry Point (`bin/git-mind.js`)

Manual argv parsing — no CLI framework. Parses command name and flags (`--type`, `--confidence`, `--source`, `--target`), then delegates to command functions in `src/cli/commands.js`.

**Current commands:** `init`, `link`, `list`, `remove`, `view`, `install-hooks`, `process-commit`, `suggest` (stub), `review` (stub)

### Edge CRUD (`src/edges.js`)

`createEdge(graph, { source, target, type, confidence, rationale })`
1. Validates all inputs via `validators.js`
2. Emits warnings for unknown prefixes (to stderr)
3. Auto-creates source/target nodes if they don't exist (idempotent `addNode`)
4. Creates edge via `graph.createPatch()` → `.addEdge()` → `.commit()`
5. Sets edge properties: confidence, createdAt, rationale

`queryEdges(graph, { source?, target?, type? })`
- Fetches all edges via `graph.getEdges()`, then filters client-side
- Returns edge objects with full properties (`from`, `to`, `label`, `props`)

`removeEdge(graph, source, target, type)`
- Removes matching edge via patch commit

### Views (`src/views.js`)

Registry pattern: `Map<string, ViewDefinition>`. A view is a filter function `(nodes, edges) → { nodes, edges }`.

**4 built-in views:**

| View | Node filter | Edge filter |
|------|------------|-------------|
| `roadmap` | `phase:*`, `task:*` | All related edges |
| `architecture` | `crate:*`, `module:*`, `pkg:*` | `depends-on` only |
| `backlog` | `task:*` | All related edges |
| `suggestions` | Nodes in low-conf edges | Confidence < 0.5 |

Custom views can be registered via `defineView(name, filterFn)`.

### Commit Directives (`src/hooks.js`)

Parses commit messages for directives that auto-create edges.

**Directive regex:** `/^(IMPLEMENTS|AUGMENTS|RELATES-TO|BLOCKS|DEPENDS-ON|DOCUMENTS):\s*(\S.*)$/gmi`

**Current directive grammar:**

```
directive      := KEYWORD ":" WHITESPACE+ target
KEYWORD        := "IMPLEMENTS" | "AUGMENTS" | "RELATES-TO" | "BLOCKS"
                | "DEPENDS-ON" | "DOCUMENTS"
target         := non-whitespace-run trailing-trimmed
```

**Behavior:**
- Case-insensitive for keyword (`implements:` = `IMPLEMENTS:`)
- One directive per line — the regex is anchored to `^...$` with multiline flag
- Target is everything after the colon and whitespace, trimmed
- Source node is always `commit:<sha>`
- Confidence: 0.8 (high but not human-reviewed)
- Rationale: `"Auto-created from commit <sha8>"`

**Defined edge cases:**

| Input | Behavior |
|-------|----------|
| `IMPLEMENTS: spec:auth` | Creates one edge |
| `implements: spec:auth` | Creates one edge (case-insensitive) |
| `IMPLEMENTS: spec:auth, spec:session` | Creates one edge to target `"spec:auth, spec:session"` (comma is **not** a delimiter) |
| `IMPLEMENTS: spec:auth spec:session` | Creates one edge to target `"spec:auth spec:session"` (space is **not** a delimiter within target) |
| Two `IMPLEMENTS:` lines in one message | Creates two edges (each line matched independently) |
| `AUGMENTS:` with no target | No match (`\S` requires at least one non-space char after colon) |
| Directive in commit subject line | Matches (regex is multiline, not body-only) |

**Current limitation:** The regex captures `(\S.*)$` as the target, which means multi-word targets like `spec:auth, spec:session` are treated as a single target string. This will fail node ID validation (commas and spaces are not in the allowed charset), causing `createEdge` to throw. The directive effectively requires exactly one valid node ID per line.

**Planned improvements (ORACLE milestone):**
- Explicit multi-target delimiter (comma-separated, validated individually)
- Body-only directive matching (skip subject line)
- Duplicate directive deduplication within a single commit

### Validation (`src/validators.js`)

Pure functions that return result objects — they never throw. Callers decide error handling.

- `validateNodeId(id)` — format, length, regex
- `validateEdgeType(type)` — against EDGE_TYPES enum
- `validateConfidence(value)` — [0.0, 1.0], rejects NaN/Infinity/non-number
- `validateEdge(source, target, type, confidence)` — composite; collects all errors and warnings

Self-edges are forbidden for `blocks` and `depends-on` (would create semantic paradoxes).

### Formatting (`src/cli/format.js`)

Terminal output using chalk + figures:
```
✔ Created edge
ℹ 3 edge(s):
  file:src/auth.js --[implements]--> spec:auth (100%)
```

Views render with header, stats, edge list, and orphan nodes section.

---

## 6. Data Flow: End-to-End Scenarios

### Scenario: `git mind link file:a.js spec:auth --type implements`

```
CLI parses argv
  → commands.link(graph, 'file:a.js', 'spec:auth', { type: 'implements', confidence: 1.0 })
    → edges.createEdge(graph, { source, target, type, confidence })
      → validators.validateEdge(source, target, type, confidence)
        → validateNodeId('file:a.js')        → { valid: true }
        → validateNodeId('spec:auth')        → { valid: true }
        → validateEdgeType('implements')     → { valid: true }
        → validateConfidence(1.0)            → { valid: true }
        → self-edge check                    → pass (different nodes)
        → prefix check                      → both canonical, 0 warnings
      → graph.createPatch()
        → patch.addNode('file:a.js')         // idempotent if exists
        → patch.addNode('spec:auth')         // idempotent if exists
        → patch.addEdge('file:a.js', 'spec:auth', 'implements')
        → patch.setEdgeProperty(..., 'confidence', 1.0)
        → patch.setEdgeProperty(..., 'createdAt', ISO timestamp)
        → patch.commit()                      // writes to Git ref
    → format.success('Created edge...')
```

### Scenario: Post-commit hook fires

```
.git/hooks/post-commit runs:
  npx git-mind process-commit "$SHA"

CLI parses argv
  → commands.processCommitCmd(graph, sha)
    → git log -1 --format=%B $SHA          // get commit message
    → hooks.processCommit(graph, { sha, message })
      → hooks.parseDirectives(message)
        → regex matches: IMPLEMENTS: spec:auth
        → returns [{ type: 'implements', target: 'spec:auth' }]
      → for each directive:
        → edges.createEdge(graph, {
            source: 'commit:<sha>',
            target: 'spec:auth',
            type: 'implements',
            confidence: 0.8,
            rationale: 'Auto-created from commit <sha8>'
          })
```

### Scenario: `git mind view architecture`

```
CLI parses argv
  → commands.view(graph, 'architecture')
    → views.renderView(graph, 'architecture')
      → get all nodes from graph
      → filter: keep only crate:*, module:*, pkg:* nodes
      → get all edges from graph
      → filter: keep only 'depends-on' edges between matching nodes
      → return { nodes, edges }
    → format.formatView('architecture', result)
      → print header, node/edge counts
      → print each edge: source --[depends-on]--> target (confidence%)
      → print orphan nodes (nodes with 0 edges in this view)
```

---

## 7. Test Coverage

**74 tests across 5 files** (as of v2.0.0-alpha.0, verified by `vitest run`).

| File | Tests | Focus |
|------|-------|-------|
| `test/graph.test.js` | 3 | Init, load (idempotent), round-trip persistence |
| `test/edges.test.js` | 14 | CRUD, 8 edge types, confidence, validation rejection, self-edge |
| `test/hooks.test.js` | 8 | Directive parsing (6 types), case insensitivity, processCommit integration |
| `test/validators.test.js` | 42 | Exhaustive boundary testing for all validators + constants |
| `test/views.test.js` | 7 | 4 built-in views, custom view registration, filtering |

**Note:** The README and ROADMAP reference "25 tests" — this is stale. The actual count is 74.

**Testing pattern:** Every test creates a temp Git repo (`mkdtemp` + `git init`), initializes the graph, runs the test, then `rm -rf`s the temp dir. No mocking — real Git operations. Full isolation.

**Tested:**
- Graph lifecycle (init, load, checkpoint round-trip)
- Edge CRUD with all 8 relationship types
- Confidence scoring boundaries (0.0, 0.5, 1.0, -0.1, 1.1, NaN, Infinity, null)
- Node ID validation (regex, length, prefix classification)
- Self-edge prohibition for `blocks` and `depends-on`
- Commit directive parsing (all 6 directive types, case insensitivity)
- View filtering (all 4 built-in views + custom)

**Not tested (known gaps):**
- CLI commands directly (`bin/git-mind.js` argv parsing)
- Terminal formatting (`src/cli/format.js`)
- Concurrent CRDT merge behavior (multiple writers)
- Large graph performance (>1K nodes)
- Error recovery (corrupt refs, partial writes)
- Hook installation and execution (`install-hooks` command)

---

## 8. Failure Modes

| Failure | Current behavior | Severity |
|---------|-----------------|----------|
| Corrupt Git ref | git-warp throws on materialization | Fatal — manual ref repair |
| Missing writer chain | Materialization skips that writer's patches | Silent data loss |
| Partial hook execution | Some directives create edges, others fail | Partial state — no rollback |
| Invalid node ID in directive target | `createEdge` throws, remaining directives skipped | Partial — first N succeed |
| Disk full during patch commit | Git object write fails, patch lost | Fatal — retry after space freed |
| Concurrent checkpoint + write | git-warp handles via ref CAS | Safe — retry on conflict |
| `plumbing` not installed as direct dep | `ShellRunnerFactory.create()` throws | Fatal — npm install fix |

---

## 9. Roadmap: The Seven Milestones

The roadmap is a 7-milestone, 46-task, ~202 hour plan organized as a dependency DAG. The roadmap is itself tracked as a DAG inside git-mind's own graph.

### Milestone 1: BEDROCK (28h) — Schema & Node Foundations

**Goal:** Establish the schema contract, runtime validators, and node query layer.

**Status:** Partially complete. `validators.js` (BDK-002) and its tests (BDK-007) are done. What remains:
- `GRAPH_SCHEMA.md` spec doc (BDK-001)
- Node query/inspection API: `getNodes()`, `getNode()`, `hasNode()`, `getNodesByPrefix()` (BDK-003/004)
- `git mind nodes` command with `--prefix`, `--id`, `--json` flags (BDK-005/006)
- Node query test suite (BDK-008)

### Milestone 2: INTAKE (34h) — Data Ingestion Pipeline

**Depends on:** BEDROCK

A YAML import pipeline with:
- Schema-validated ingestion (`version: 1` field required, unknown version = hard error)
- Idempotent merge semantics (re-import = safe; nodes merge props, edges update confidence)
- Reference validation (no dangling edges — all source/target must exist)
- Atomic writes (all-or-nothing via `createPatch()`)
- CLI: `git mind import graph.yaml --dry-run --validate --json`

### Milestone 3: PRISM (30h) — Views & Value Surfaces

**Depends on:** BEDROCK

Refactors hardcoded views into a **declarative view engine** (config objects instead of filter functions). Then adds 4 new views:
- **milestone** — progress tracking (completion %, blockers)
- **traceability** — spec-to-implementation gap analysis
- **blockers** — transitive blocking chain resolution with cycle detection
- **onboarding** — topologically-sorted reading order for new engineers

### Milestone 4: WATCHTOWER (18h) — Dashboard & Observability

**Depends on:** BEDROCK, PRISM

`git mind status` — a single command showing graph health:
- Total nodes and edges
- Nodes by prefix (count + percentage)
- Edges by type (count)
- Blocked items count
- Low-confidence edges (< 0.5) count
- Orphan nodes (0 edges) count
- `--json` output for CI pipelines

### Milestone 5: PROVING GROUND (16h) — Dogfood Validation

**Depends on:** All above

Seeds the [Echo](https://github.com/neuroglyph/echo) ecosystem into git-mind and validates 5 specific project management questions can be answered via CLI in <60 seconds total:

1. What blocks milestone M2?
2. Which ADRs lack implementation?
3. Which crates are unlinked to specs?
4. What should a new engineer read first?
5. What's low-confidence and needs review?

### Milestone 6: ORACLE (40h) — AI Intelligence & Curation

**Depends on:** PROVING GROUND

- `git mind suggest --ai` — LLM-powered edge suggestions from code context
- `git mind review` — interactive accept/reject/adjust/skip workflow
- Review provenance stored in git-warp (decisions improve future suggestions)
- `git mind doctor` — integrity checks (dangling edges, orphan milestones, duplicates)
- Design principle: **humans curate, machines suggest.** AI never auto-commits.

### Milestone 7: NEXUS (36h) — Integration & Federation

**Depends on:** ORACLE

- GitHub Action that posts edge suggestions on PRs
- Markdown frontmatter import (`git mind import --from-markdown`)
- Cross-repo edges with federation-qualified node IDs
- Multi-repo graph merge (additive, namespaced)
- Round-trip YAML/JSON export (`git mind export --format yaml|json`)

### Dependency DAG

```
BEDROCK → INTAKE  ──→ PROVING GROUND → ORACLE → NEXUS
       ↘ PRISM  ──↗
       ↘ WATCHTOWER ↗
```

INTAKE, PRISM, and WATCHTOWER can proceed in parallel once BEDROCK is done. Everything converges at PROVING GROUND.

### Backlog (Unscheduled)

- Confidence decay over time (edges rot if not refreshed)
- View composition (combine multiple views)
- Graph diff between commits (`git mind diff HEAD~5..HEAD`)
- Mermaid diagram export
- GraphQL API for web frontends
- Real-time file watcher for automatic edge updates
- Git blame integration (who created this edge?)
- Edge provenance visualization

---

## 10. Key Design Decisions

| Decision | Choice | Why |
|----------|--------|-----|
| Storage backend | Git refs via git-warp CRDTs | No external DB, versioned with code, conflict-free |
| Node ID format | `prefix:identifier` | Enables prefix-based filtering and view matching |
| Prefix casing | Lowercase prefix, case-preserving identifier | `milestone:BEDROCK` not `Milestone:bedrock` |
| Edge uniqueness | `(source, target, type)` tuple | Re-adding updates, doesn't duplicate |
| Implicit nodes | Nodes created by edge references | Reduces ceremony; edges are the primary data |
| Import failure | All-or-nothing (atomic via `createPatch`) | No partial writes, validates everything first |
| AI suggestions | Never auto-commit | Humans maintain authority over the graph |
| Schema version | Required `version: 1`, unknown = hard error | Fail closed, not best-effort |
| CLI framework | None (manual argv) | Zero deps, full control |
| Validation style | Pure functions returning result objects | No throws; callers decide error handling |
| Unknown prefixes | Warning, not rejection | Allows organic growth; CI can enforce strictness |
| Convergence model | CRDTs guarantee state convergence; validation governs correctness | Explicit separation of concerns |

---

## 11. What's Built vs. What's Planned

**Built (v2.0.0-alpha.0):**
- Graph lifecycle (init, load, checkpoint)
- Edge CRUD with 8 types + confidence scoring
- 4 hardcoded observer views
- Commit directive parser (6 directive types)
- CLI with 7 working commands + 2 stubs
- Comprehensive validation layer
- 74 tests, CI green (GitHub Actions, Node 22 + 24)

**Not yet built:**
- Node query layer (nodes are still implicit-only)
- YAML import pipeline
- Declarative view engine
- Status/dashboard command
- AI suggestions + interactive review
- GitHub Action integration
- Cross-repo federation
- Export command
- Prefix governance config (`.gitmind.yml`)
- Directive payload grammar (multi-target support)
- Checkpoint automation / patch compaction
