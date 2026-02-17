# Git Mind North Star

## Verifiable Knowledge Operations on WARP Graphs

---

## 1) North Star Statement

Git Mind is a **WARP-native knowledge operating system** for teams and autonomous agents.
It models work as cryptographically causal graph entities (nodes/edges/properties), supports domain-specific workflows via extensions, and materializes human-readable artifacts (Markdown, HTML, PDF, etc.) on demand from graph state — at any point in time, for any observer/trust policy.

**Source of truth is the graph, not checked-in files.**

---

## 2) Identity

**Git Mind is:**
- A domain-agnostic knowledge graph engine
- A WARP-powered causal/provenance runtime
- An extension platform for domain workflows
- A materialization engine for file-like outputs without file-centric truth
- A trust-aware audit substrate (authorship, approvals, policy)

**Git Mind is not:**
- A generic note app
- A static markdown repo
- A thin wrapper over GitHub issues/docs
- A plugin free-for-all without deterministic contracts

---

## 3) Core Product Thesis

Traditional tools separate planning, docs, tickets, architecture, and audit trails into disconnected silos.
Git Mind unifies them into one causal graph where:

1. Every fact has provenance.
2. Every state is replayable.
3. Every domain is an extension.
4. Every document is materialized from graph truth.
5. Every query can be observer- and trust-scoped.

---

## 4) Platform Model

### 4.1 Substrate (WARP / git-warp)
- Cryptographic causal history
- Multi-writer/fork/merge semantics
- Time-travel and deterministic replay
- Observer-relative projections
- Trust-aware verification (approved authorship, delegation, revocation)

### 4.2 Semantic Runtime (Git Mind Core)
- Generic node/edge/property model
- Query engine
- View/lens composition
- Rule execution
- Materialization pipeline

### 4.3 Extension Layer

Extensions declare domain semantics:
- Schemas (node/edge types)
- Views
- Lenses
- Rules
- Sync adapters
- Workflow actions
- Materializers/templates

---

## 5) Terminology Decision

Use **Extensions** as user-facing term (clear and product-friendly).
Optionally keep "packs" as internal implementation name during migration.

- CLI: `git mind extension ...`
- Manifest: `extension.yaml`
- Registry: extension registry
- SDK: extension SDK

---

## 6) Key Product Capabilities

### A) Graph-native domain workflows

Examples:
- **Roadmap workflow**: task/feature/milestone graphs
- **ADR workflow**: decision nodes + rationale + consequences
- **PR workflow**: contributors/reviews/approvals/dependencies
- **Incident workflow**: timeline, blast radius, corrective actions

### B) Content-on-node (not file-first)

Any rich content (ADR body, spec text, policy memo, postmortem narrative) is stored as graph-linked content, not as canonical checked-in docs.

Content becomes:
- versioned
- provenance-backed
- observer-aware
- trust-filterable
- materializable on demand

### C) Materialization engine

Render graph state into:
- Markdown
- HTML
- PDF
- JSON
- static site outputs

at:
- current head
- historical head (`--as-of`)
- specific observer/trust policy

### D) Provenance slicing + explainability

For any node/edge/property:
- full mutation lineage
- causal ancestry
- author/trust chain
- policy/rule receipts
- "why this value?" explanation

### E) Trust-aware operations

With git-warp trust features:
- approved-authorship modes
- delegated trust
- revocation impact analysis
- trust-scoped views and rules
- signed evidence/certification outputs

---

## 7) The ADR Vision (concrete example)

Instead of writing ADRs in external docs:

**1. Create ADR node:**
```bash
git mind node add adr:0007 --type adr --title "Adopt extension runtime"
```

**2. Attach content directly to node (graph-backed CAS payload):**
```bash
git mind content set adr:0007 --from ./adr-0007.md
# or interactive editor
git mind content edit adr:0007
```

**3. Link to impacted architecture/tasks:**
```bash
git mind edge add adr:0007 decides module:runtime
git mind edge add adr:0007 affects task:M11
```

**4. Materialize when needed:**
```bash
git mind materialize adr:0007 --format md --out ./dist/adr-0007.md
git mind materialize adr:0007 --format html --out ./dist/adr-0007.html
git mind materialize adr:0007 --format pdf --out ./dist/adr-0007.pdf
```

**5. Time travel:**
```bash
git mind materialize adr:0007 --as-of main~30 --format md
```

Same node, many renderings, full provenance.

---

## 8) Content Storage Architecture

### Option A: Git Notes-backed content

**Pros:**
- Native git primitive
- Easy inspection
- Familiar ecosystem

**Cons:**
- Scaling/format constraints
- Harder to enforce richer contracts
- Less control for advanced CAS policies

### Option B: git-cas-backed content (recommended default)

**Pros:**
- Content-addressed durability
- Natural dedupe/chunking potential
- Strong integrity linkage to WARP receipts
- Cleaner path for encryption/policy/capability controls
- Better for large/structured content evolution

**Cons:**
- More initial implementation work
- Needs good tooling for inspectability

### Recommended stance
- **Canonical content store**: git-cas-backed
- **Optional import/export bridges** for git notes interop
- Treat notes as compatibility channel, not primary substrate

---

## 9) Materialization Model

Materialization should be explicit and reproducible.

### 9.1 Inputs
- entityId or view
- asOf (rev/receipt/tick)
- observerContext
- trustPolicy
- extensionSet + lock
- template/profile

### 9.2 Output envelope

Every materialized artifact includes provenance header:
- graph head / frontier hash
- observer hash
- trust policy hash
- extension lock hash
- render hash
- timestamp of render (non-semantic metadata)

### 9.3 Rules
- Rendering must be deterministic given same semantic inputs
- Generated files are artifacts, not canonical truth
- Artifacts may be cached, always reproducible

---

## 10) Feature Catalog

### 10.1 Core Engine
- Generic graph operations
- Deterministic serialization and query execution
- Time-travel/as-of query semantics
- Fork/merge aware reasoning

### 10.2 Extension Runtime
- Extension manifest v1
- Registry/activation/deactivation
- Namespacing and collision policy
- Capability-scoped scripts (pure/effect)
- Extension compatibility checks

### 10.3 View/Lens System
- Named views per extension
- Composable lenses (`view:a:b:c`)
- Cross-extension projections
- Causal/observer-aware filtering

### 10.4 Rules/Policy
- Rule execution with severities
- Doctor policy gates
- CI-ready contracts
- Trust policy integration

### 10.5 Sync/Automation
- Plan/apply split
- Idempotent sync receipts
- Source snapshot hashing
- Replayable ingest pipelines

### 10.6 Content System
- Entity-attached content bodies
- Rich metadata (mime, language, template hints)
- Multi-revision content lineage
- Content linking + embedding primitives

### 10.7 Materialization
- Markdown/HTML/PDF/JSON renderers
- Template engine per extension
- Static site/export packs
- As-of materialization and comparison

### 10.8 Provenance & Audit
- Holographic slice explorer
- trace/prove/certify commands
- Evidence bundles
- Chain-of-custody certificates
- Trust drift and revocation impact

### 10.9 Observer Intelligence
- Observer profiles
- Observer diff / disagreement maps
- Rulial-distance metrics (operational v1)
- Policy simulation across historical windows

### 10.10 Distribution & Governance
- Extension lockfile
- Signing/integrity policy
- Conformance test suite
- Maturity levels (experimental/stable/trusted)

---

## 11) Long-term Roadmap

### Horizon 1: Deterministic Extension Foundation
- Extension spec v1
- Registry + namespaces
- Lens ABI
- Built-in domains migrated to extensions
- Contracts/schema lock

### Horizon 2: Graph-native Content + Materialization
- Content subsystem (CAS-backed)
- ADR/doc workflows on nodes
- Materialize MD/HTML/PDF
- As-of rendering + reproducible headers

### Horizon 3: Trust + Audit Intelligence
- Approved authorship integration
- Trust-aware views/rules
- Certify/evidence bundle workflows
- Revocation and policy simulation

### Horizon 4: Ecosystem + Knowledge OS
- Extension marketplace/distribution
- Signed trusted extensions
- Federated multi-domain workflows
- Enterprise governance/audit suites

---

## 12) Design Invariants (hard gates)

1. **Determinism > convenience**
2. **Graph truth > file truth**
3. **Extensions cannot bypass trust/provenance contracts**
4. **Side effects always plan/apply**
5. **Observer/trust context is explicit** for materialization and views
6. **Every machine interface is schema-versioned**
7. **Reproducibility requires extension lock + provenance envelope**

---

## 13) Proposed CLI Surface (v1 direction)

```bash
# Extension lifecycle
git mind extension list
git mind extension add roadmap
git mind extension remove docs
git mind extension validate ./my-extension

# Content-on-node
git mind content set adr:0007 --from ./adr.md
git mind content edit adr:0007
git mind content show adr:0007 --as-of main~12

# Materialization
git mind materialize adr:0007 --format md --out ./dist/adr.md
git mind materialize view:roadmap --format html --out ./site/roadmap.html
git mind materialize adr:0007 --format pdf --as-of release/1.0

# Provenance/trust
git mind trace adr:0007 --property status
git mind prove adr:0007 --trust approved-only
git mind certify adr:0007 --bundle audit --out ./evidence/
```

---

## 14) Immediate Next Decisions

1. **Choose canonical content store**
   Recommend: git-cas-backed, with optional git-notes bridge.

2. **Freeze extension manifest v1**
   Don't build runtime before spec discipline.

3. **Define materialization contract v1**
   Input/output envelope + provenance headers.

4. **Define content entity model**
   Node-linked content object schema (revisioning, mime, origin, trust tags).

5. **Decide observer/trust defaults**
   Prevent hidden ambiguity in CLI behavior.

---

## 15) The Narrative

> "Git Mind turns organizational knowledge into a verifiable causal graph.
> You work in domain workflows through extensions.
> You write decisions and specs directly to graph entities.
> Files are generated views, not source of truth.
> Everything is replayable, auditable, and trust-aware."

That's a category statement, not a feature list.
