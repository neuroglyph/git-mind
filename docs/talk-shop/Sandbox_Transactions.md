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
  edges/<branch>                          # canonical journal
  pending/<actor>/<txn-id>/edges/<branch> # sandbox transactions (namespaced)
  pending/<actor>/<txn-id>/meta           # json metadata for the txn
  applied/<txn-id>                        # optional archival pointer
```

- Transactions are ULID-keyed and namespaced by the author (CLI derives `<actor>` from git config or `GITMIND_TXN_NAMESPACE`). This prevents cross-claiming someone else’s sandbox.
- Metadata (JSON) records author, branch target, base journal OID, start time, edge-count hash, and applied-by provenance. Payload edges mirror the canonical layout so replay logic can reuse existing helpers.

### CLI Flow

1. `git mind txn start [--branch main]` → returns txn ID, records base journal OID.
2. Subsequent `git mind link/list/...` commands operate inside the transaction by default (reads merge pending + canonical, writes hit pending).
3. `git mind txn show [--format table|json|diff]` renders edges staged in the transaction.
4. Users choose `git mind txn apply [--branch main]` (alias: `txn commit`) or `git mind txn abort`.

`git mind txn show --format diff` is the default preview; `--dry-run` performs the apply simulation without mutating refs (useful for CI or pre-review).

### Apply Logic

- Collect pending commits for `<txn-id>`.
- Confirm the canonical journal tip still equals the recorded base OID (fast-forward); else replay on top (append new commits) and surface schema/ULID conflicts loudly.
- Stamp the apply commit with trailers (`Gm-Txn:` and `Gm-Apply:`) for provenance.
- Update `refs/gitmind/edges/<branch>` and move the transaction ref under `refs/gitmind/applied/<txn-id>` (retention policy below) or prune it if configured.
- Trigger cache rebuild hooks or mark the cache dirty if the branch matches current context.

### Abort Logic

- Drop the pending ref; the CLI should refuse to abort if the transaction was already applied to avoid footguns.

### Concurrency / Conflicts

- If the canonical ref moved while the transaction was active, we replay the edges as fresh commits: read CBOR payloads, re-run append logic, and create new journal commits.
- If replay encounters conflicts (duplicate ULIDs, incompatible schema), we surface the errors before applying.

### Hooks & Tests

- Local hooks (pre-push) should block accidental pushes of `refs/gitmind/pending/*` unless an override flag is set; server-side hooks enforce trailers on canonical refs and optionally auto-archive transactions (see worked example in `Sandbox_Transactions_Feedback.md`).
- Unit tests for `gm_journal` should ensure reads can accept a transaction ref override (`gm_journal_read_txn(ctx, txn_id, ...)`).
- E2E: script that starts a transaction, stages edges, aborts, verifies canonical journal unchanged; then repeat with apply and validate.

## Implementation Phases

1. **Scaffold** – add `gm_txn_*` helpers (start/apply/abort) that manipulate Git refs only; no CLI surface yet.
2. **CLI Preview** – wire `git mind txn` commands, default writes to a transaction when `GITMIND_TXN_ID` is set (useful for `make ci-local` harnesses).
3. **Opt-in Auto-Txn** – add `--txn` flag, environment variable, and docs. CI remains “apply-only”; developers manually apply before running pipelines.
4. **Retention + Enforcement** – implement configurable retention (`git config gitmind.txn.retention=keep|days=<n>|delete`) and disallow direct journal writes outside of a transaction unless `--apply` is passed explicitly (with a hook guard).

Default retention proposal:

- Pending refs are pruned automatically on abort.
- Applied refs are archived under `refs/gitmind/applied/` for 14 days by default, overridden via config or `git mind txn apply --no-archive`.
- A periodic `git mind txn gc` command prunes expired archives.

## Decisions & Remaining Questions

- ✅ **Retention** — archive applied transactions for 14 days by default with `gitmind.txn.retention` override; pending refs auto-pruned on abort/apply.
- ✅ **CI strategy** — CI remains “apply-only”; pipelines fail fast if pending refs are pushed. Developers must apply (or use dry-run apply locally) before running `make ci-local`.
- ✅ **Namespaces & metadata** — each transaction stores author, base OID, branch, start time, edge digest, and apply provenance; refs are namespaced by actor.
- ✅ **Replay semantics** — schema violations or duplicate ULIDs abort apply. No silent skipping; users fix the transaction then re-apply.
- ❓ **Review tooling** — explore integrating transaction IDs into worksheet/review seeding so bots can comment before apply.
- ❓ **Rebase support** — evaluate need for `git mind txn rebase` once transactions are in heavy use.

## Related Work

- `docs/features/Features_Ledger.md` (new tasks around cache/journal confidence)
- `docs/talk-shop/Graph-Lvl-Up-Tank.md` (journey node + transaction storytelling)
- Existing cache conformance tests and the recently added branch/tree coverage (`core/tests/unit/test_cache_branch_limits.c`, `test_cache_tree_size.c`)

---

Feedback welcome—especially on the remaining review/rebase questions. A worked end-to-end example (including hook snippets) lives in `docs/talk-shop/Sandbox_Transactions_Feedback.md` for quick reference.
