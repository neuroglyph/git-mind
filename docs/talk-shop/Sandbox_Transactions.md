---
title: Sandbox Transaction Blocks
description: RFC to stage git-mind graph mutations inside inspectable transaction branches before committing to the journal.
audience: [contributors]
domain: [architecture]
tags: [journal, workflow, sandbox]
status: draft
last_updated: 2025-09-17
---

# Sandbox Transaction Blocks

## Why

As we migrate fully into `core/` and tighten our conformance tests, the next friction point is confidence in graph mutations. Contributors (humans, plugins, or AI helpers) often want to queue a series of edges, inspect the result, then decide whether to land it. Today the only options are:

- append directly to `refs/gitmind/edges/<branch>` and hope CI catches bad edges later
- hand-roll temporary refs and clean them up manually
- bypass the hooks with `HOOKS_BYPASS=1`

We can do better by giving every mutation a sandboxed transaction.

## Goals

- **Preview first** – let devs and tools see exactly what a batch of edges will do before touching the canonical refs.
- **Make journaling safe** – journal entries authored in a transaction never appear in the main DAG until applied.
- **Shareable drafts** – a transaction should be easy to hand off (txn ID/ref), so reviewers can inspect deltas without bespoke tooling.
- **Integrate with cache rebuilds** – transaction refs must be invisible to cache/journal consumers unless explicitly targeted.
- **Zero surprise for CI** – transactions that were never applied should not affect builds/tests.

Non-goals for the first iteration: multi-user locking, conflict resolution UI, or automatic rebase of transactions during apply.

## Proposed Design

### Ref Layout

```
refs/gitmind/
  edges/<branch>          # canonical journal
  pending/<txn-id>        # sandbox transaction refs
  applied/<txn-id>        # (optional) archive for auditing
```

- Transaction IDs are ULIDs; metadata stored under `refs/gitmind/pending/<txn-id>/meta`.
- Payload edges live under `refs/gitmind/pending/<txn-id>/edges/<branch>` mirroring the canonical layout so reuse is easy.

### CLI Flow

1. `git mind txn start [--branch main]` → returns txn ID, records base journal OID.
2. Subsequent `git mind link/list/...` commands operate inside the transaction by default (reads merge pending + canonical, writes hit pending).
3. `git mind txn show [--format table|json|diff]` renders edges staged in the transaction.
4. Users choose `git mind txn apply [--branch main]` or `git mind txn abort`.

### Apply Logic

- Collect pending commits for `<txn-id>`.
- Confirm the canonical journal tip still equals the recorded base OID (fast-forward); else replay on top (append new commits).
- Update `refs/gitmind/edges/<branch>` and move the transaction ref under `refs/gitmind/applied/<txn-id>` (or delete if retention not needed).
- Trigger cache rebuild hooks or mark the cache dirty if the branch matches current context.

### Abort Logic

- Drop the pending ref; the CLI should refuse to abort if the transaction was already applied to avoid footguns.

### Concurrency / Conflicts

- If the canonical ref moved while the transaction was active, we replay the edges as fresh commits: read CBOR payloads, re-run append logic, and create new journal commits.
- If replay encounters conflicts (duplicate ULIDs, incompatible schema), we surface the errors before applying.

### Hooks & Tests

- Hooks (pre-push) should whitelist pending refs or ignore them entirely.
- Unit tests for `gm_journal` should ensure reads can accept a transaction ref override (`gm_journal_read_txn(ctx, txn_id, ...)`).
- E2E: script that starts a transaction, stages edges, aborts, verifies canonical journal unchanged; then repeat with apply and validate.

## Implementation Phases

1. **Scaffold** – add `gm_txn_*` helpers (start/apply/abort) that manipulate Git refs only; no CLI surface yet.
2. **CLI Preview** – wire `git mind txn` commands, default writes to a transaction when `GITMIND_TXN_ID` is set (useful for `make ci-local` test harnesses).
3. **Opt-in Auto-Txn** – add `--txn` flag to commands, environment variable for automation, and docs for contributors.
4. **Enforce Default** – eventually, disallow direct journal writes outside of a transaction unless `--apply` is passed explicitly (with a hook guard).

## Open Questions / Feedback Needed

- **Transaction retention** – do we keep applied transactions under `refs/gitmind/applied/` for audit trails, or rely on Git history only?
- **CI strategy** – should CI automatically apply transactions (and roll back) to run tests, or do we rely on developers applying manually first?
- **Integration with review seeding** – can CodeRabbit or the worksheet flow respond to transaction comments when edges are applied?
- **Merge tool** – do we want a `gm mind txn rebase` to restack the transaction on a new base before applying?
- **Security** – do we need ACLs to prevent transactions from referencing other users’ pending refs (e.g., namespace by user)?

## Related Work

- `docs/features/Features_Ledger.md` (new tasks around cache/journal confidence)
- `docs/talk-shop/Graph-Lvl-Up-Tank.md` (journey node + transaction storytelling)
- Existing cache conformance tests and the recently added branch/tree coverage (`core/tests/unit/test_cache_branch_limits.c`, `test_cache_tree_size.c`)

---

Feedback welcome, especially on apply semantics and how strict we want to be before promoting transactions to canonical refs.
