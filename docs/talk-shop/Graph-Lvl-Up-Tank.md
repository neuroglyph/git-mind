---
title: Graph LvL-Up Tank
description: Crash-course to power-level new contributors into graph-native thinking for git-mind.
audience: [contributors]
domain: [learning]
tags: [graphs, onboarding, best-practices]
status: draft
last_updated: 2025-09-16
---

# Graph LvL-Up Tank 🛡️🧠

Welcome to the tank queue. Strap in; by the time you exit, you should be dangerous enough to model git-mind semantics without summoning undefined behavior—or a disappointed reviewer.

## 0. Mindset Buffs (Pre-pull Ritual)
- **Everything is an edge first, artifact second.** Start by naming the relationship (`implements`, `touches`, `reflects`), then pick endpoints. If you can’t articulate the edge, you’re not ready to store the nodes.
- **Paths are narratives.** A commit-to-retro chain is a story. Store the raw events, then decide which shortcuts (journey nodes, convenience edges) the UX needs.
- **Filters > Firehoses.** Plan for edge-type and node-type filters in every traversal. Hidden subgraphs are the secret sauce; surface them deliberately.
- **Rebuild is cheap, provenance isn’t.** Derived edges are disposable. Canonical edges (journal entries) must be traceable and time-travel safe.

## 1. Core Knowledge Slots (Allocate 5 Skill Points)
1. **Graph Math Basics**
   - Directed acyclic vs. cyclic; strongly connected components.
   - Path length (degree/separation) and why we cap N in CLI output.
2. **Query Languages**
   - Cypher pattern: `MATCH (a:Issue)-[:RESOLVED_IN]->(f:File)` → mental template for git-mind queries.
   - Gremlin grok: `g.V().hasLabel('file').out('implements')` — stream mindset for CLI pipelines.
3. **Indexing**
   - Node key indexes (file path + OID), composite indexes for `(:Edge {type, lane})`.
   - Materialized projections (journey nodes, globs) to accelerate user-facing flows.
4. **Temporal Semantics**
   - Effective-from / effective-to windows, commit ULIDs, cache snapshots.
   - How time-aware queries keep “what was true at C2?” deterministic.
5. **Conflict Handling**
   - Merge strategies: union vs. CRDT for advice edges, last-writer-wins metadata.
   - Review worksheets as capture devices, not just documentation.

## 2. git-mind Native Patterns
- **Journey Nodes (a.k.a. path bundles)**
  - Use when the same path is replayed often (Issue → PR → Review → Merge → Retro).
  - Store ordered edge IDs + timestamps; connect journey ⇄ participants for hop-1 access.
- **Convenience Edges**
  - Materialize on workflows closing: `issue --touched--> file`, `retro --reflects--> file`.
  - Regenerate on cache rebuild; mark `derived=true` for hygiene.
- **Edge Taxonomy**
  - `primary`: canonical journal entry (AUGMENTS edge).
  - `secondary`: manual curation (`implements`, `verified_by`).
  - `advisory`: AI suggestions, dashed edges in the SVG; require verification state.
- **OID-first Everything**
  - Node keys: `(type, repo_id, oid)`.
  - Edge keys: `(src_oid, dst_oid, type, ulid)` to guarantee time-travel diffing.

## 3. Query Recipes (Tank Macros)
| What you want | Pattern | Notes |
| --- | --- | --- |
| “All artifacts tied to this file” | `MATCH (f:File {oid:$oid})-[:TOUCHES|:REFLECTS|:JOURNEY_HAS*1..2]->(x) RETURN x` | Include journey bundles for 1-hop hits. |
| “Which commits closed issue #42?” | `MATCH (i:Issue {number:42})-[:RESOLVED_IN]->(j:Journey)-[:MERGED_IN]->(c:Commit) RETURN c` | Journey keeps metadata; fallback to merge edges. |
| “Retro notes within 2 hops of auth.c” | `MATCH (f:File {path:'core/src/auth.c'})-[:REFLECTS\|:JOURNEY_HAS*1..2]->(r:Retro) RETURN DISTINCT r` | Filters keep CLI output sane. |
| “What changed between C2 and C5?” | Use timeline diff: `gm timeline diff --from C2 --to C5 --edge-types implements,verified_by` | Rely on cache deltas; avoid brute-force BFS. |

## 4. Modeling Checklist (Before You Commit Your Thoughts)
- [ ] Edge types named verb-first (`implements`, `reflects`, `verified_by`).
- [ ] Node labels single-responsibility (no `DocOrIssue` mashups).
- [ ] Every derived edge cites its source journal ULID.
- [ ] Path bundle recorded if more than three steps are replayed in UX.
- [ ] Filters documented: which queries surface this edge? Which layers hide it?

## 5. Toolchain & Playground
- **Local Sandbox**: Use `cypher-shell` or `memgraph` for synthetic modeling before touching git refs.
- **Visualization**: Keep the animated SVG (assets/images/git-mind-dag.svg) as the mental map; update it when semantics evolve.
- **Performance Drills**: `EXPLAIN` queries (even conceptually) to spot where indexes matter. If a traversal requires >3 hops routinely, consider a projection.
- **Docs Backfill**: Update `docs/architecture/*` whenever you invent a new edge type or path strategy; ledger tasks depend on that clarity.

## 6. Level-Up Quests (XP Farming)
1. Run `git mind list --from core/src/auth.c --with-attribution` and sketch the path you *expected* vs. what exists. Gap? Add edges or journeys.
2. Model Issue → Review → Merge as a journey node in a scratch graph; verify you can reach every participant in ≤2 hops.
3. Draft a Cypher query that respects time-travel (`WHERE c.ulid BETWEEN $from AND $to`). Translate it into git-mind CLI verbs.
4. Teach someone else the difference between canonical vs. derived edges. If you can’t explain it in one whiteboard, repeat this tank session.

## 7. Raid Boss Tips
- **Graph drift shows up as surprise hops.** When a traversal suddenly needs N+2 steps, inspect recent merges; you probably missed a convenience edge.
- **Don’t promote AI advice to canonical without human sign-off.** Keep `verified_by` state explicit; blank verification is villain fuel.
- **Measure before refactoring.** Snapshot traversal counts/latency before “optimizing” edge types. Graph debt hides in assumptions.
- **Time-travel kills shortcuts.** If an edge isn’t reproducible at C1…Cn, it doesn’t belong in the journal. Derived edges can time-shift; canonical can’t.

## 8. TL;DR Buff Sheet
- Model narratives, not just nodes.
- Use journey nodes + convenience edges to balance fidelity and ergonomics.
- Keep queries filter-rich and depth-limited.
- Treat OIDs and ULIDs as sacred; everything else is rebuildable.
- Document every new edge type the moment it spawns.

Queue is clear. Mount awarded. Go forth and mind the graph.
