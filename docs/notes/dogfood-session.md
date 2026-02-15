# PROVING GROUND — Dogfood Session Transcript

> **Date**: 2026-02-11
> **Seed**: `test/fixtures/echo-seed.yaml` — Echo ecosystem (55 nodes, 70 edges)
> **Runtime**: Node.js 20, vitest 3.2.4

## Setup

```bash
$ git mind init
Initialized empty git-mind graph

$ git mind import test/fixtures/echo-seed.yaml
Imported 55 nodes, 70 edges (144ms)
```

---

## Q1: What blocks milestone M2?

**View**: `milestone`

```bash
$ git mind view milestone --json | jq '.meta.milestoneStats["milestone:M2"]'
{
  "total": 2,
  "done": 0,
  "pct": 0,
  "blockers": ["issue:E-003", "issue:E-004"]
}
```

**Answer**: Two issues block M2 — `issue:E-003` (REST schema breaks on nested objects) blocks `task:E-007`, and `issue:E-004` (validation rejects valid payloads) blocks `task:E-008`. Both M2 tasks are incomplete (0%).

**Timing**: <1ms

---

## Q2: Which ADRs lack implementation?

**View**: `traceability`

```bash
$ git mind view traceability --json | jq '.meta.gaps | map(select(startswith("adr:")))'
[
  "adr:003-encryption-at-rest",
  "adr:004-rest-vs-grpc"
]
```

**Answer**: 2 of 5 ADRs have no `implements` edge pointing at them. ADR-003 (encryption at rest for PII) and ADR-004 (REST over gRPC) are decisions with no code backing. The other 3 ADRs are covered: `echo-core` implements ADR-001, `echo-db` implements ADR-002, `echo-api` implements ADR-005.

**Timing**: <1ms

---

## Q3: Which crates are unlinked to specs?

**View**: `coverage` (new in PROVING GROUND)

```bash
$ git mind view coverage --json | jq '.meta'
{
  "linked": [
    "crate:echo-core",
    "crate:echo-api",
    "crate:echo-db",
    "crate:echo-auth",
    "crate:echo-queue",
    "crate:echo-web"
  ],
  "unlinked": [
    "crate:echo-crypto",
    "crate:echo-cli",
    "crate:echo-config",
    "crate:echo-log",
    "crate:echo-test-utils",
    "crate:echo-bench",
    "crate:echo-migrate",
    "crate:echo-plugin",
    "crate:echo-sdk"
  ],
  "coveragePct": 40
}
```

**Answer**: 9 of 15 crates have no `implements` edge to any spec or ADR. Coverage is 40%. The unlinked crates are utility/tooling crates (crypto, cli, config, log, test-utils, bench, migrate, plugin, sdk) — they may not need formal specs, but the gap is now visible.

**Timing**: <1ms

---

## Q4: What should a new engineer read first?

**View**: `onboarding`

```bash
$ git mind view onboarding --json | jq '.meta.readingOrder | map(select(startswith("doc:")))'
[
  "doc:getting-started",
  "doc:architecture-overview",
  "doc:api-reference",
  "doc:contributing",
  "doc:deployment-guide"
]
```

**Answer**: Start with `doc:getting-started` (the root — no dependencies). Then `doc:architecture-overview` (depends on getting-started). After that, `doc:api-reference` and `doc:contributing` (both depend on architecture-overview). Finally `doc:deployment-guide` (depends on getting-started, so it could be read earlier, but topological sort with alphabetical tie-breaking places it last).

**Timing**: <1ms

---

## Q5: What's low-confidence?

**View**: `suggestions` + `computeStatus()`

```bash
$ git mind view suggestions --json | jq '.edges[] | {from, to, label, confidence: .props.confidence}'
{ "from": "issue:E-016", "to": "issue:E-017",          "label": "relates-to", "confidence": 0.2 }
{ "from": "issue:E-018", "to": "crate:echo-crypto",    "label": "relates-to", "confidence": 0.3 }
{ "from": "issue:E-019", "to": "spec:003-web-sockets",  "label": "relates-to", "confidence": 0.4 }
{ "from": "issue:E-020", "to": "doc:contributing",       "label": "relates-to", "confidence": 0.3 }

$ git mind status --json | jq '.health.lowConfidence'
4
```

**Answer**: 4 edges have confidence below 0.5 (the low-confidence threshold). All are `relates-to` edges — likely AI-suggested links that haven't been reviewed. Confidence values range from 0.2 to 0.4.

**Timing**: <1ms

---

## Summary

| # | Question | View | Answer | Time |
|---|----------|------|--------|------|
| 1 | What blocks M2? | `milestone` | `issue:E-003`, `issue:E-004` | <1ms |
| 2 | ADRs lacking impl? | `traceability` | `adr:003-encryption-at-rest`, `adr:004-rest-vs-grpc` | <1ms |
| 3 | Unlinked crates? | `coverage` | 9 of 15 (40% coverage) | <1ms |
| 4 | Read first? | `onboarding` | `doc:getting-started` → `doc:architecture-overview` | <1ms |
| 5 | Low-confidence? | `suggestions` | 4 edges (0.2–0.4) | <1ms |

**Total query time**: ~1ms for all 5 questions against a 55-node, 70-edge graph.

All 5 questions answered correctly from the graph alone — no manual inspection, no external tools, just views.
