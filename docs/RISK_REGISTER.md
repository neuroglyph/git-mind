# Git Mind Risk Register
**Version:** 1.0
**Date:** 2026-02-15  
**Status:** Active  
**Applies to:** M10–M18 roadmap execution

---

## 0) Purpose

This document defines the risk control system for Git Mind as it evolves into a WARP-native, distributed, multi-writer graph platform with file-native editing bridges.

This is not a passive list.  
It is an operational control plane with:

- risk tiers,
- measurable leading indicators,
- stop-ship thresholds,
- named ownership,
- weekly burn-down cadence.

---

## 1) Risk Scoring Model

Each risk has:

- **Impact (1–5)**
- **Likelihood (1–5)**
- **Score = Impact × Likelihood**
- **Trend:** ↑ worsening / → stable / ↓ improving
- **Status:** Green / Yellow / Red

### Status Bands
- **Green:** score 1–7
- **Yellow:** score 8–14
- **Red:** score 15–25

---

## 2) Governance Rules (MUST/SHOULD/COULD/DON’T)

## MUST
- Maintain this register as a living artifact (updated weekly).
- Assign a named DRI for every active risk.
- Define at least one leading indicator and one stop-ship condition per Tier-0/Tier-1 risk.
- Block milestone promotion when any Tier-0 risk is Red.
- Attach evidence links (dashboards, tests, logs) for every mitigation marked “done.”

## SHOULD
- Automate indicator collection in CI/telemetry where possible.
- Re-score risks after every major release or architecture change.
- Pair each mitigation with an owner + due date.
- Maintain “last 4-week trend” snapshots for top 6 risks.

## COULD
- Add quantitative risk burn-down charts.
- Simulate incident drills for top 3 Tier-0 risks monthly.
- Track mitigation cost (engineering hours) to improve planning realism.

## DON’T
- Don’t close risks because “we discussed it.”
- Don’t mark mitigations complete without evidence.
- Don’t merge major architecture changes while Tier-0 stop-ship conditions are active.
- Don’t hide risk status to preserve momentum optics.

---

## 3) Tier Definitions

- **Tier 0 (Existential / Near-term):** can kill adoption or data integrity in <3 months.
- **Tier 1 (Strategic / Mid-term):** can kill credibility or platform viability in <1 year.
- **Tier 2 (Chronic / Long-term):** survivable short-term but harmful compounding debt.

---

## 4) Risk Register

## Tier 0 — Existential

---

### R01 — Git Bloat Death Spiral (Substrate Failure)
- **Owner Team:** Core
- **DRI:** `<name>`
- **Impact:** 5
- **Likelihood:** 4
- **Score:** 20 (Red)
- **Trend:** →
- **Status:** Open

**Description**  
Graph mutations produce high object churn; `.git` growth degrades clone/fetch/status/CI over time.

**Leading Indicators**
- `.git/objects` growth rate > 15% week-over-week for 2+ consecutive weeks
- clone time p95 increases > 25% month-over-month on dogfood repos
- CI checkout/setup time exceeds baseline by > 40%

**Stop-Ship Condition**
- clone time p95 > 8 minutes on reference dogfood repo
- or `git status` p95 > 3 seconds in normal dev workflow

**Mitigations**
- Aggressive checkpoint/compaction strategy in git-warp
- Object deduplication verification
- Scheduled maintenance/GC workflows
- Growth telemetry in CI/nightly jobs

**Evidence**
- `<dashboard link>`
- `<compaction report link>`

---

### R02a — Save Path Latency / Editor Blocking (UX Failure)
- **Owner Team:** Bridge
- **DRI:** `<name>`
- **Impact:** 5
- **Likelihood:** 4
- **Score:** 20 (Red)
- **Trend:** →
- **Status:** Open

**Description**  
Editor save path stalls due to synchronous diff/validate/CAS/writeback/receipt pipeline.

**Leading Indicators**
- save→ack p95 > 200ms for small files (<50KB)
- save→durable p95 > 800ms on normal laptop baseline
- repeated “saving…” spinner incidents in dogfood sessions

**Stop-Ship Condition**
- user-visible editor blocking reproducible in 3+ common scenarios

**Mitigations**
- SyncAdapter buffering mode (disk as decoupling boundary)
- Durable vs Buffered mode semantics
- writeback queue with backpressure/health reporting
- strict performance budgets + regression tests

**Evidence**
- `<latency dashboard>`
- `<perf CI job>`

---

### R02b — Filesystem Semantics Mismatch (UX Correctness Failure)
- **Owner Team:** Bridge
- **DRI:** `<name>`
- **Impact:** 4
- **Likelihood:** 4
- **Score:** 16 (Red)
- **Trend:** →
- **Status:** Open

**Description**  
Atomic save patterns, rename semantics, and watcher behaviors differ across OS/editor stacks.

**Leading Indicators**
- save failures tied to editor temp/rename patterns
- inconsistent behavior across macOS/Linux/Windows in integration suite
- high rate of “file changed on disk” false conflict prompts

**Stop-Ship Condition**
- data-destructive or unresolvable save behavior in any supported primary editor profile

**Mitigations**
- OS/editor compatibility test harness
- explicit handling for atomic rename-save patterns
- loop suppression + debounce in file event layer
- SyncAdapter-first rollout before FUSE mode expansion

**Evidence**
- `<cross-platform integration run>`
- `<editor matrix results>`

---

### R03 — Conflict Hell (Data Integrity + UX Failure)
- **Owner Team:** Bridge
- **DRI:** `<name>`
- **Impact:** 5
- **Likelihood:** 3
- **Score:** 15 (Red)
- **Trend:** →
- **Status:** Open

**Description**  
Multi-writer edits project into file conflicts users cannot interpret or safely resolve.

**Leading Indicators**
- conflict rate > 3% of write attempts in dogfood
- manual cache nukes used as conflict workaround
- unresolved conflicts older than 24h

**Stop-Ship Condition**
- any conflict path that can cause silent overwrite or ambiguous final state

**Mitigations**
- base hash + frontier optimistic concurrency checks
- explicit conflict artifact generation with guided resolver flow
- human-readable conflict UX (not raw merge gibberish)
- optional checkout/lease mode for high-contention entities

**Evidence**
- `<conflict metrics dashboard>`
- `<resolver UX test report>`

---

### R04 — Write-Ack Ambiguity (Silent Data Loss Failure)
- **Owner Team:** Bridge
- **DRI:** `<name>`
- **Impact:** 5
- **Likelihood:** 3
- **Score:** 15 (Red)
- **Trend:** →
- **Status:** Open

**Description**  
OS/editor reports “saved,” but durable graph commit did not happen.

**Leading Indicators**
- buffered writeback retries exceeding 5 minutes
- writeback failures not surfaced to users
- mismatch between local file state and graph commit status

**Stop-Ship Condition**
- one confirmed silent-loss incident in production/dogfood

**Mitigations**
- explicit modes:
  - **Durable:** save acknowledged only after commit+receipt
  - **Buffered:** immediate local save + persistent pending status
- persistent retry queue with durable journal
- mandatory status visibility (`git mind status`, diagnostics)

**Evidence**
- `<writeback pipeline logs>`
- `<loss-prevention test suite>`

---

### R05 — Determinism Drift (Credibility Failure)
- **Owner Team:** Forge
- **DRI:** `<name>`
- **Impact:** 5
- **Likelihood:** 3
- **Score:** 15 (Red)
- **Trend:** →
- **Status:** Open

**Description**  
Equivalent inputs produce different outputs/hashes across environments.

**Leading Indicators**
- flaky golden tests across OS runners
- hash mismatches in materialization fixtures
- cache misses where deterministic hits expected

**Stop-Ship Condition**
- any reproducible cross-environment hash mismatch on canonical fixture set

**Mitigations**
- canonical byte encoders + normalized line endings
- pinned renderer/template/extension versions
- cross-OS determinism CI matrix
- byte-for-byte golden fixtures and regression gating

**Evidence**
- `<determinism CI matrix>`
- `<fixture report>`

---

### R13 — Checkpoint/Compaction Correctness Risk
- **Owner Team:** Core
- **DRI:** `<name>`
- **Impact:** 5
- **Likelihood:** 3
- **Score:** 15 (Red)
- **Trend:** →
- **Status:** Open

**Description**  
Compaction may break replay/proof equivalence across pre/post-checkpoint boundaries.

**Leading Indicators**
- divergence between pre/post-compaction query results
- proof verification drift after compaction
- replay mismatch in audit fixtures

**Stop-Ship Condition**
- any compaction equivalence failure on certified fixture suite

**Mitigations**
- compaction equivalence harness:
  - pre/post query equivalence
  - pre/post materialization hash equivalence
  - pre/post proof trace equivalence
- rollback-capable compaction transaction protocol

**Evidence**
- `<equivalence harness report>`
- `<checkpoint rollback test>`

---

## Tier 1 — Strategic

---

### R06 — Platform Trap (Scope Failure)
- **Owner Team:** Product/Platform
- **DRI:** `<name>`
- **Impact:** 5
- **Likelihood:** 3
- **Score:** 15 (Red)
- **Trend:** →
- **Status:** Open

**Description**  
Core infrastructure expands while user-facing domain value stalls.

**Leading Indicators**
- milestone output has infra-only deliverables with no dogfooded workflow impact
- roadmap extension usability regresses or stagnates across releases
- increasing “foundation PR” ratio without user task completion gains

**Stop-Ship Condition**
- two consecutive milestones with no measurable user-value uplift in primary domain workflows

**Mitigations**
- roadmap extension as mandatory co-driver for platform features
- “no platform feature without active domain use-case” policy
- product acceptance checks per milestone

**Evidence**
- `<dogfood usage metrics>`
- `<feature-to-workflow mapping doc>`

---

### R07 — Extension Supply-Chain / Capability Abuse
- **Owner Team:** Platform
- **DRI:** `<name>`
- **Impact:** 5
- **Likelihood:** 3
- **Score:** 15 (Red)
- **Trend:** →
- **Status:** Open

**Description**  
Third-party extension scripts execute with excessive privileges.

**Leading Indicators**
- extensions can access fs/net/exec without declared capabilities
- unsigned/unverified extensions used in trusted environments
- policy bypasses increase in dogfood

**Stop-Ship Condition**
- any extension executes privileged effects outside declared capability set

**Mitigations**
- capability manifest (default deny)
- sandboxing + resource quotas
- extension lockfile + signature verification
- enforceable trust policy modes per environment

**Evidence**
- `<capability conformance report>`
- `<sandbox test logs>`

---

### R08 — Trust Theater (Security Semantics Failure)
- **Owner Team:** Citadel
- **DRI:** `<name>`
- **Impact:** 5
- **Likelihood:** 2
- **Score:** 10 (Yellow)
- **Trend:** →
- **Status:** Open

**Description**  
Users misinterpret policy-level checks as cryptographic guarantees.

**Leading Indicators**
- docs/CLI use ambiguous terms (“verified,” “trusted”) without mode disclosure
- support questions reveal repeated trust model confusion
- evidence bundles omit verification provenance details

**Stop-Ship Condition**
- any high-assurance workflow claims crypto verification without enforceable proof path

**Mitigations**
- explicit trust mode surface: `none | policy | cryptographic`
- fail-closed options for regulated/high-assurance pipelines
- strict terminology rules in docs/CLI copy
- signed evidence bundles with key lineage and verification status

**Evidence**
- `<trust mode UX screenshots>`
- `<policy/crypto conformance tests>`

---

### R09 — Observability Debt (Ops Blindness)
- **Owner Team:** Core
- **DRI:** `<name>`
- **Impact:** 4
- **Likelihood:** 3
- **Score:** 12 (Yellow)
- **Trend:** →
- **Status:** Open

**Description**  
Insufficient telemetry prevents root-cause analysis of sync/invalidation/writeback issues.

**Leading Indicators**
- >20% incident reports cannot be diagnosed from logs
- repeated bug class “intermittent desync, no trace”
- missing correlation IDs across pipeline stages

**Stop-Ship Condition**
- inability to reconstruct critical incident timeline within 30 minutes

**Mitigations**
- structured event logs with correlation IDs
- diagnostics bundle export
- `explain` commands for cache/materialization/writeback decisions

**Evidence**
- `<logging schema doc>`
- `<incident reconstruction drill report>`

---

### R14 — Policy Deadlock Risk (Over-Restriction Failure)
- **Owner Team:** Platform/Citadel
- **DRI:** `<name>`
- **Impact:** 4
- **Likelihood:** 3
- **Score:** 12 (Yellow)
- **Trend:** →
- **Status:** Open

**Description**  
Security/capability policies become so strict that users disable protections globally.

**Leading Indicators**
- frequent use of global bypass flags
- policy-related support tickets exceed feature-related tickets
- elevated failure rate in legitimate workflows due to policy denials

**Stop-Ship Condition**
- recurring organization-level instruction to disable policy enforcement to keep teams working

**Mitigations**
- staged policy levels (`audit`, `warn`, `enforce`)
- scoped temporary overrides with expiry
- policy simulator/dry-run mode and remediation hints

**Evidence**
- `<policy bypass metrics>`
- `<override audit report>`

---

## Tier 2 — Chronic

---

### R10 — Migration Drag (Upgrade Friction)
- **Owner Team:** Core
- **DRI:** `<name>`
- **Impact:** 3
- **Likelihood:** 3
- **Score:** 9 (Yellow)
- **Trend:** →
- **Status:** Open

**Description**  
Upgrades require manual repair, damaging trust and adoption.

**Leading Indicators**
- migration failure rate > 2%
- support load spikes after release
- recurring manual remediation docs

**Stop-Ship Condition**
- no rollback path for schema/format migrations

**Mitigations**
- versioned adapters
- automated migration with dry-run and rollback
- `doctor fix` remediation scripts

**Evidence**
- `<migration test matrix>`
- `<post-release support metrics>`

---

### R11 — Performance Death by 1000 Cuts
- **Owner Team:** Core
- **DRI:** `<name>`
- **Impact:** 3
- **Likelihood:** 4
- **Score:** 12 (Yellow)
- **Trend:** →
- **Status:** Open

**Description**  
Small regressions accumulate, degrading perceived responsiveness.

**Leading Indicators**
- p95 latency drift upward for 3 consecutive releases
- perf benchmark variance widening
- increasing user-reported sluggishness without single root cause

**Stop-Ship Condition**
- breach of agreed p95 budgets on core commands in release candidate

**Mitigations**
- hard SLOs and regression gates in CI
- benchmark suite over representative graph sizes
- hot-path profiling discipline per milestone

**Evidence**
- `<perf dashboard>`
- `<benchmark history>`

---

### R12 — Bus Factor / Knowledge Concentration
- **Owner Team:** Team
- **DRI:** `<name>`
- **Impact:** 4
- **Likelihood:** 2
- **Score:** 8 (Yellow)
- **Trend:** →
- **Status:** Open

**Description**  
Critical system knowledge concentrated in one maintainer.

**Leading Indicators**
- single point of review for critical subsystems
- missing runbooks for bridge/writeback/compaction
- incident response blocked by maintainer availability

**Stop-Ship Condition**
- critical incident cannot be mitigated by secondary maintainer within SLA window

**Mitigations**
- mandatory runbooks for top 5 critical subsystems
- rotation-based code ownership and incident drills
- ADR hygiene and architecture walkthrough recordings

**Evidence**
- `<runbook index>`
- `<ownership matrix>`

---

## 5) Cross-Risk Operational SLOs

These budgets become hard release gates once baseline is established.

- **Save→Ack (small entity file):** p95 ≤ 200ms
- **Save→Durable Commit:** p95 ≤ 800ms (normal workstation baseline)
- **View Query (common):** p95 ≤ 300ms
- **Materialize (small doc):** p95 ≤ 500ms
- **Conflict Resolution Success:** ≥ 95% without cache reset
- **Cross-OS Determinism Fixtures:** 100% pass

---

## 6) 30-Day Anti-Death Protocol

## MUST
1. Instrument `.git` growth, save pipeline latency, writeback outcomes, conflict stats.
2. Ship SyncAdapter MVP before FUSE work.
3. Land compaction/checkpoint strategy + equivalence harness.
4. Enable cross-OS determinism gate on fixture suite.
5. Dogfood roadmap extension as primary product driver.

## SHOULD
- Publish weekly risk burn-down report.
- Run one incident reconstruction drill/week for top Tier-0 risks.
- Keep milestone scope tied to at least one user-visible workflow delta.

## COULD
- Add synthetic load test repo for 10k+ nodes.
- Run policy deadlock simulation for extension restrictions.
- Add trust-mode UX comprehension tests.

## DON’T
- Don’t expand platform scope while Tier-0 stop-ship is active.
- Don’t defer instrumentation “until after MVP.”
- Don’t introduce new extension power without capability policy.

---

## 7) Weekly Risk Review Template

**Date:**  
**Attendees:**  
**Tier-0 Status:** Green / Yellow / Red  
**Tier-1 Status:** Green / Yellow / Red

### A) Score Changes
| Risk ID | Previous | Current | Trend | Reason |
|---|---:|---:|---|---|
| R01 |  |  |  |  |

### B) Mitigation Progress
| Risk ID | Mitigation | Owner | Due | Status | Evidence |
|---|---|---|---|---|---|
| R04 | Durable/Buffered mode status channel |  |  |  |  |

### C) Stop-Ship Check
- Any Tier-0 stop-ship condition triggered?  
  - [ ] No  
  - [ ] Yes (list IDs + action plan)

### D) Decisions
- Promote milestone?  
  - [ ] Yes  
  - [ ] No  
- Required corrective actions:

---

## 8) Appendix: Risk Ownership Map

- **Core:** R01, R09, R10, R11, R13
- **Bridge:** R02a, R02b, R03, R04
- **Forge:** R05
- **Platform:** R06, R07, R14
- **Citadel:** R08, R14
- **Team/Program:** R12

---

## 9) Changelog

### v1.0 (2026-02-15)
- Initial consolidated doomsday register (14 risks)
- Added indicators, stop-ship thresholds, SLOs, and review protocol
- Added MUST/SHOULD/COULD/DON’T governance model
