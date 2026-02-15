# Git Mind Architecture Laws
_Last updated: 2026-02-15_

This document defines non-negotiable engineering laws for Git Mind.

---

## Law 1: Graph Truth Is Canonical
The WARP graph (entities, relations, properties, provenance, receipts) is the source of truth.  
Materialized files are derived artifacts and are non-canonical by default.

---

## Law 2: Worktree Does Not Define System State
Git worktree state is a convenience context, not authoritative state.  
`HEAD` is a default query parameter, never the truth boundary.

---

## Law 3: No Hidden Context Coupling
If output can vary by time/observer/trust/extensions, commands must accept explicit context (or deterministic defaults) and surface it in output metadata.

Required context dimensions:
- `at` / `asOf`
- `observer`
- `trustPolicy`
- `extensionSet` (and lock hash)

---

## Law 4: Materialization Is a Pure Derivation
Materialization must be a deterministic function of:
- projection input/frontier
- context envelope
- renderer/template/version
- options

Same inputs must produce identical output bytes (excluding explicitly non-semantic metadata).

---

## Law 5: Derived Artifacts Must Not Pollute Source Workflows
Generated artifacts are written outside repo root by default.  
If repo-local output is used, it must be policy-controlled (ignore + hook + CI + allowlist).

---

## Law 6: Deterministic Caching Only
Cache keys for projections/materializations must be content-addressed and derived exclusively from semantic inputs and pinned tool/template versions.

No ambient state in cache keys.

---

## Law 7: Side Effects Require Plan/Apply
Any external sync/effectful operation must:
1. produce deterministic plan
2. apply explicitly
3. emit provenance receipt

No implicit mutation from opaque fetch-transform logic.

---

## Law 8: Extensions Cannot Violate Core Invariants
Extensions may add domain semantics, but may not bypass:
- determinism rules
- trust/provenance requirements
- context envelope semantics
- contract versioning requirements

---

## Law 9: Contracts Before Convenience
Machine-facing outputs require versioned schemas.  
Breaking changes require explicit version bumps and migration path.

---

## Law 10: Provenance Is a Product Surface
For any meaningful entity/property, lineage must be queryable and explainable (“why this value?”), including trust-relevant transitions where available.

---

## Law 11: Publish Is Explicit
Tracking generated artifacts in Git is an explicit publication action, never incidental side effect of normal workflows.

---

## Law 12: When in Doubt, Preserve Reproducibility
If a design tradeoff conflicts with reproducibility, reproducibility wins.

---

## PR Gate Checklist (Fast)
A PR must be rejected or redesigned if it introduces any of:

- [ ] Canonical truth split between graph and generated files
- [ ] Hidden dependency on ambient worktree state
- [ ] Non-deterministic output without explicit opt-in and labeling
- [ ] Effectful sync without plan/apply + receipt
- [ ] Extension behavior that bypasses trust/provenance/context contracts
- [ ] Generated artifact tracking without policy allowlist
- [ ] Contract-breaking output changes without schema version update
