# Vision Alignment / Drift Audit

Table of Contents

- Executive Summary
- Scope and Method
- Findings by Document
- Summary of Drift
- Remediation Plan

## Executive Summary

The vision is: version your thoughts — a serverless, distributed, forkable thought‑graph for humans and AI built on Git. The core path is journal (edges‑as‑commits), cache (fast queries), names‑as‑truth, attribution/lanes, AUGMENTS, and optional advice; co‑thought (MCP + review lanes) is optional.

Overall, top‑level docs now mostly align. Two documents showed notable drift (ARCHITECTURE.md, MIGRATION_STATUS.md). Minor terminology and status misalignments remain in a few places.

## Scope and Method

- Reviewed: README.md, ARCHITECTURE.md, docs/architecture/_, PRD/ADR, planning/_, requirements, risk, deployment, AGENTS.md, MIGRATION_STATUS.md.
- Compared to current direction as captured in README + Product Roadmap.

## Findings by Document

1) README.md — Aligned

- Vision captured (“version your thoughts”, co‑thought, serverless DB). Quickstart includes code+notes examples. Added stub outputs.
- Action: None.

2) ARCHITECTURE.md — Was drifted, UPDATED

- Outdated progress metrics and dependency claims (said “only libgit2”); now corrected to reflect CRoaring and current focus. Status table modernized.
- Action: Keep as index; avoid duplicating status numbers.

3) docs/architecture/* — Partially aligned

- Journal/Cache docs align; attribution docs match filters/lanes direction.
- Action: Add a short note on “names‑as‑truth” to attribution docs in a future pass.

4) PRD (Semantics) and ADR 0001 — Aligned

- Names‑as‑truth, advice via hybrid CRDT, cache IDs derived at runtime.
- Action: Specify final hash choice (FNV‑1a‑64 vs SipHash) and add test vectors (later).

5) Planning (Product_Roadmap, Release_Plans, Milestones, Sprint_Plans) — UPDATED

- Now includes optional Co‑Thought MCP and review lanes as post‑core.
- Action: Track MCP tasks behind a feature flag in issues.

6) Requirements — UPDATED

- Added optional co‑thought requirements (attribution filters, lanes, MCP, promote/demote).
- Action: None.

7) Risk Register — UPDATED

- Added AI precision/privacy risks; mitigations noted (lanes, local‑only MCP).
- Action: None.

8) Deployment — Aligned

- CI gates, packaging, rollback, monitoring; no server dependencies.
- Action: Add MCP runbook when implemented.

9) AGENTS.md — UPDATED

- Added vision snapshot and docs linter guidance; removed internal Neo4j guidance.
- Action: Keep co‑thought/MCP marked as optional.

10) MIGRATION_STATUS.md — Drifted

- Contradictory statuses (Attribution both “Complete” and “Next Action”).
- Action: Replace ad‑hoc metrics with links to planning milestones; simplify to “what’s next”.

## Summary of Drift

- Primary drift: stale status/progress numbers; dependency claims (libgit2‑only) ignoring cache dependency; unclear optionality of MCP.
- Secondary drift: terminology duplication and minor inconsistencies across architecture subpages.

## Remediation Plan

- Short term (this PR):
  - ARCHITECTURE.md modernized (done).
  - Planning/requirements/risk updated with co‑thought optional track (done).
  - AGENTS.md vision snapshot and docs linter guidance (done).
- Next steps:
  - Simplify MIGRATION_STATUS.md to point at Milestones and remove conflicting numbers.
  - Add hash algorithm decision + vectors to PRD/specs.
  - Add an “Attribution filters & lanes” note in architecture docs.
  - Add a runbook for MCP when implemented.

Last reviewed: <set-on-commit>
