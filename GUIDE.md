# git-mind Guide

Everything you need to know — from zero to power user.

---

## Table of Contents

1. [What is git-mind?](#what-is-git-mind)
2. [Installation](#installation)
3. [Your first knowledge graph](#your-first-knowledge-graph)
4. [Core concepts](#core-concepts)
5. [CLI reference](#cli-reference)
6. [Views](#views)
7. [Commit directives](#commit-directives)
8. [Using git-mind as a library](#using-git-mind-as-a-library)
9. [Appendix A: How it works under the hood](#appendix-a-how-it-works-under-the-hood)
10. [Appendix B: Edge types reference](#appendix-b-edge-types-reference)

---

## What is git-mind?

git-mind adds a **semantic knowledge graph** to any Git repository. You create **nodes** (files, concepts, tasks, modules — anything) and connect them with **typed edges** (implements, depends-on, documents, etc.). The graph lives inside Git — no external databases, no servers.

**Why?**

Code tells a computer what to do. Comments tell a human what the code does. But neither captures *why* things are connected — which spec does this file implement? What does this module depend on? What task does this commit address?

git-mind captures those relationships explicitly, so you can query them, visualize them, and watch them evolve with your code.

**What makes it different?**

- **Git-native** — Your graph is versioned alongside your code. Check out an old commit, get the old graph.
- **Conflict-free** — Built on CRDTs, so multiple people can add edges simultaneously without conflicts.
- **Branch and merge** — Try experimental connections in a branch, merge what works.
- **No setup** — No database to run. No config files. Just `git mind init`.

---

## Installation

### Prerequisites

- [Node.js](https://nodejs.org/) >= 22.0.0
- [Git](https://git-scm.com/)

### From source

```bash
git clone https://github.com/neuroglyph/git-mind.git
cd git-mind
npm install
```

git-mind depends on `@git-stunts/git-warp`, which is currently installed from a local path. See [CONTRIBUTING.md](CONTRIBUTING.md) for development setup details.

### Verify the installation

```bash
node bin/git-mind.js --help
```

You should see the help text with available commands.

---

## Your first knowledge graph

Let's walk through building a knowledge graph from scratch.

### 1. Initialize

Navigate to any Git repository and initialize git-mind:

```bash
cd /path/to/your/repo
npx git-mind init
# ✔ Initialized git-mind graph
```

This creates a WARP graph stored inside Git's ref system. It's invisible to normal Git operations — your working tree stays clean.

### 2. Create some links

Let's say your repo has a spec document and an implementation:

```bash
# This file implements that spec
npx git-mind link file:src/auth.js spec:auth --type implements

# These two modules are related
npx git-mind link module:cache module:db --type depends-on

# This test documents the expected behavior
npx git-mind link file:test/auth.test.js file:src/auth.js --type documents
```

Each `link` command creates an edge between two nodes. If the nodes don't exist yet, they're created automatically. Node IDs must use the `prefix:identifier` format.

### 3. See what you've built

```bash
npx git-mind list
# ℹ 3 edge(s):
#   file:src/auth.js --[implements]--> spec:auth (100%)
#   module:cache --[depends-on]--> module:db (100%)
#   file:test/auth.test.js --[documents]--> file:src/auth.js (100%)
```

### 4. Use views for focused projections

```bash
npx git-mind view architecture
# Shows only module/crate/pkg nodes and depends-on edges

npx git-mind view roadmap
# Shows only phase/task nodes and their relationships
```

### 5. That's it

Your knowledge graph is stored in Git. It persists across clones (once pushed), branches, and merges. No cleanup needed.

---

## Core concepts

### Nodes

A node ID follows the `prefix:identifier` format. The prefix is always lowercase, and the identifier can contain letters, digits, dots, slashes, `@`, and hyphens. See [GRAPH_SCHEMA.md](GRAPH_SCHEMA.md) for the full grammar.

| Prefix | Meaning | Example |
|--------|---------|---------|
| `file:` | File path | `file:src/auth.js` |
| `module:` | Software module | `module:authentication` |
| `task:` | Work item | `task:implement-oauth` |
| `phase:` | Project phase | `phase:beta` |
| `commit:` | Git commit (system-generated) | `commit:abc123` |
| `concept:` | Abstract idea | `concept:zero-trust` |
| `milestone:` | Major project phase | `milestone:BEDROCK` |
| `feature:` | Feature grouping | `feature:BDK-SCHEMA` |
| `spec:` | Specification document | `spec:graph-schema` |

Unknown prefixes produce a warning but are allowed — this lets the taxonomy grow organically. See the full prefix list in [GRAPH_SCHEMA.md](GRAPH_SCHEMA.md).

Nodes are created implicitly when you create an edge referencing them. You don't need to declare nodes separately.

### Edges

An edge is a directed, typed, scored connection between two nodes.

```
source --[type]--> target
```

Every edge has:
- **type** — One of the 8 defined edge types (see [Appendix B](#appendix-b-edge-types-reference))
- **confidence** — A score from 0.0 to 1.0. Human-created edges default to 1.0. AI suggestions start low.
- **createdAt** — ISO timestamp of when the edge was created
- **rationale** — Optional text explaining why the edge exists

### Confidence scores

Confidence answers the question: *how sure are we about this connection?*

| Score | Meaning | Typical source |
|-------|---------|---------------|
| 1.0 | Verified by a human | Manual `git mind link` |
| 0.8 | High confidence, not reviewed | Commit directive (auto-created) |
| 0.3–0.5 | Suggestion | AI-generated (future feature) |
| 0.0 | Unknown/untrusted | Should be reviewed or removed |

### Views

Views are filtered projections of the graph. Instead of looking at every node and edge, a view shows you only what's relevant to a particular question.

git-mind ships with four built-in views:
- **roadmap** — Phase and task nodes
- **architecture** — Module nodes and dependency edges
- **backlog** — Task nodes and their relationships
- **suggestions** — Low-confidence edges that need review

You can also define custom views programmatically (see [Using git-mind as a library](#using-git-mind-as-a-library)).

---

## CLI reference

### `git mind init`

Initialize a git-mind knowledge graph in the current repository.

```bash
git mind init
```

Safe to run multiple times — initialization is idempotent.

### `git mind link <source> <target>`

Create a semantic edge between two nodes.

```bash
git mind link file:src/auth.js spec:auth --type implements
git mind link module:a module:b --type depends-on --confidence 0.9
```

**Flags:**

| Flag | Default | Description |
|------|---------|-------------|
| `--type <type>` | `relates-to` | Edge type |
| `--confidence <n>` | `1.0` | Confidence score (0.0–1.0) |

### `git mind list`

Show all edges in the graph.

```bash
git mind list
```

### `git mind view [name]`

Render a named view, or list available views.

```bash
git mind view              # list available views
git mind view roadmap      # render the roadmap view
git mind view architecture # render the architecture view
```

### `git mind suggest`

*(Stub — not yet implemented)*

Will use AI to suggest edges based on code analysis.

### `git mind review`

*(Stub — not yet implemented)*

Will present suggested edges for human review and approval.

### `git mind help`

Show usage information.

```bash
git mind --help
```

---

## Views

### Built-in views

#### `roadmap`

Shows nodes with `phase:` or `task:` prefixes and their connecting edges.

```bash
git mind link phase:alpha task:build-cli --type blocks
git mind link task:build-cli task:write-tests --type blocks
git mind view roadmap
```

#### `architecture`

Shows nodes with `module:`, `crate:`, or `pkg:` prefixes and `depends-on` edges between them.

```bash
git mind link module:auth module:db --type depends-on
git mind link module:api module:auth --type depends-on
git mind view architecture
```

#### `backlog`

Shows all `task:` nodes and their relationships.

#### `suggestions`

Shows edges with confidence below 0.5 — connections that need human review.

### Custom views (programmatic)

```javascript
import { defineView, renderView, loadGraph } from '@neuroglyph/git-mind';

defineView('my-view', (nodes, edges) => ({
  nodes: nodes.filter(n => n.startsWith('feature:')),
  edges: edges.filter(e => e.label === 'implements'),
}));

const graph = await loadGraph('.');
const result = await renderView(graph, 'my-view');
console.log(result);
```

---

## Commit directives

git-mind can automatically create edges from commit messages. Include directives in your commit body:

```
feat: add OAuth2 login flow

Implements the social login spec with Google and GitHub providers.

IMPLEMENTS: spec:social-login
AUGMENTS: module:auth-basic
RELATES-TO: concept:zero-trust
```

### Supported directives

| Directive | Edge type created |
|-----------|------------------|
| `IMPLEMENTS:` | `implements` |
| `AUGMENTS:` | `augments` |
| `RELATES-TO:` | `relates-to` |
| `BLOCKS:` | `blocks` |
| `DEPENDS-ON:` | `depends-on` |
| `DOCUMENTS:` | `documents` |

Edges created from directives get a confidence of **0.8** — high, but flagged as not human-reviewed. The commit SHA is recorded as provenance.

### Processing commits programmatically

```javascript
import { processCommit, loadGraph } from '@neuroglyph/git-mind';

const graph = await loadGraph('.');
await processCommit(graph, {
  sha: 'abc123def456',
  message: 'feat: add login\n\nIMPLEMENTS: spec:auth',
});
```

---

## Using git-mind as a library

git-mind exports its core modules for use in scripts and integrations.

```javascript
import {
  initGraph,
  loadGraph,
  saveGraph,
  createEdge,
  queryEdges,
  removeEdge,
  EDGE_TYPES,
  validateNodeId,
  validateEdgeType,
  validateConfidence,
  validateEdge,
  extractPrefix,
  classifyPrefix,
  NODE_ID_REGEX,
  NODE_ID_MAX_LENGTH,
  CANONICAL_PREFIXES,
  SYSTEM_PREFIXES,
  defineView,
  renderView,
  listViews,
  parseDirectives,
  processCommit,
} from '@neuroglyph/git-mind';
```

### Initialize and load

```javascript
// Initialize in a repo (idempotent)
const graph = await initGraph('/path/to/repo');

// Load an existing graph
const graph = await loadGraph('/path/to/repo');

// Checkpoint the graph state
const sha = await saveGraph(graph);
```

### Edge operations

```javascript
// Create — node IDs must use prefix:identifier format
await createEdge(graph, {
  source: 'file:src/auth.js',
  target: 'spec:auth',
  type: 'implements',
  confidence: 1.0,
  rationale: 'Direct implementation of the spec',
});

// Query
const allEdges = await queryEdges(graph);
const authEdges = await queryEdges(graph, { source: 'file:src/auth.js' });
const implEdges = await queryEdges(graph, { type: 'implements' });

// Remove
await removeEdge(graph, 'file:src/auth.js', 'spec:auth', 'implements');
```

### Validation

Validators return result objects — they don't throw. Callers decide how to handle errors.

```javascript
// Validate a node ID
const r = validateNodeId('task:BDK-001'); // { valid: true }
const bad = validateNodeId('bad id');     // { valid: false, error: '...' }

// Validate a full edge (composite — checks everything)
const result = validateEdge('task:X', 'feature:Y', 'implements', 0.8);
// { valid: true, errors: [], warnings: [] }

// Classify a prefix
classifyPrefix('milestone'); // 'canonical'
classifyPrefix('commit');    // 'system'
classifyPrefix('banana');    // 'unknown'
```

### Views

```javascript
// List available views
const views = listViews(); // ['roadmap', 'architecture', 'backlog', 'suggestions']

// Render a view
const result = await renderView(graph, 'architecture');
// { nodes: ['module:auth', 'module:db'], edges: [...] }

// Define a custom view
defineView('unreviewed', (nodes, edges) => ({
  nodes: [],
  edges: edges.filter(e => e.props?.confidence < 1.0),
}));
```

### Directives

```javascript
// Parse directives from a message
const directives = parseDirectives('IMPLEMENTS: spec:auth\nBLOCKS: task:deploy');
// [{ type: 'implements', target: 'spec:auth' }, { type: 'blocks', target: 'task:deploy' }]

// Process a full commit (parse + create edges)
const processed = await processCommit(graph, {
  sha: 'abc123',
  message: 'feat: login\n\nIMPLEMENTS: spec:auth',
});
```

---

## Appendix A: How it works under the hood

### The storage model

git-mind stores its graph using [git-warp](https://github.com/nicktomlin/git-warp), a CRDT graph database that lives entirely inside Git's object store. Here's how:

**Git commits as an append-only log.** Every graph mutation (add node, add edge, set property) is a "patch" — a small operation recorded as a Git commit. These commits point to Git's empty tree object (`4b825dc...`) so they don't create any files in your working directory. They're invisible to `git status`, `git diff`, and normal Git operations.

**Refs as namespaces.** The graph is stored under `refs/warp/gitmind/writers/<writerId>`. Each writer gets its own ref — there's no contention. Multiple people (or bots) can write to the graph simultaneously without conflicts.

**Materialization.** To read the graph, git-warp "materializes" it by replaying all patches in causal order. The result is an in-memory graph of nodes, edges, and properties. git-mind sets `autoMaterialize: true`, so this happens transparently on every query.

### CRDTs — conflict-free merging

The graph uses two CRDT types:

- **OR-Set (Observed-Remove Set)** for nodes and edges. If writer A adds a node and writer B removes it concurrently, the add wins (add-wins semantics). Only explicit removals after the latest add take effect.

- **LWW (Last-Writer-Wins) registers** for properties. If two writers set the same property simultaneously, the one with the later timestamp wins. This is deterministic — both sides converge to the same value.

This means you can `git push` and `git pull` your knowledge graph freely. Merges always converge. No manual conflict resolution needed.

### What a patch looks like

Internally, a patch is a JSON blob committed to Git:

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

Each patch carries a **version vector** (`vv`) for causal ordering and a **tick** (logical clock) for property conflict resolution.

### Performance characteristics

| Operation | Complexity | Notes |
|-----------|-----------|-------|
| Add node/edge | O(1) | Append-only |
| Materialize | O(P) | P = total patches across all writers |
| Query (after materialize) | O(N) | N = nodes matching the query |
| Checkpoint | O(S) | S = state size; creates a snapshot for faster future loads |

For large graphs, use `saveGraph()` to create checkpoints. Future materializations replay only patches since the last checkpoint.

### Directory structure in Git

```
refs/
  warp/
    gitmind/
      writers/
        local          ← your patches (a chain of Git commits)
        alice          ← another writer's patches
        bot            ← a CI bot's patches
      checkpoints/
        latest         ← most recent snapshot
```

All of this is under `refs/`, not in your working tree. Your `.gitmind/` directory (in `.gitignore`) is for future local caching — it's not used yet.

---

## Appendix B: Edge types reference

| Type | Direction | Meaning | Example |
|------|-----------|---------|---------|
| `implements` | source implements target | Code fulfills a spec | `file:src/auth.js` implements `spec:auth` |
| `augments` | source extends target | Enhancement or extension | `module:auth-oauth` augments `module:auth` |
| `relates-to` | source relates to target | General association | `doc:README` relates-to `concept:philosophy` |
| `blocks` | source blocks target | Dependency/ordering (no self-edges) | `task:migrate-db` blocks `task:deploy` |
| `belongs-to` | source is part of target | Membership/containment | `file:src/auth.js` belongs-to `module:security` |
| `consumed-by` | source is consumed by target | Usage relationship | `pkg:chalk` consumed-by `module:format` |
| `depends-on` | source depends on target | Dependency (no self-edges) | `module:api` depends-on `module:auth` |
| `documents` | source documents target | Documentation | `doc:api` documents `module:api` |
