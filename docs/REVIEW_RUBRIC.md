# Git Mind Review Rubric
_Last updated: 2026-02-15_

Use this rubric for PR reviews, design reviews, and milestone gate checks.

---

## Rating Scale

- **PASS** = Meets requirement fully
- **CONDITIONAL** = Mostly correct; minor fixes required before merge
- **FAIL** = Violates architecture law or introduces significant risk

---

## Verdict Rules

- **APPROVE**:  
- No FAIL in MUST section  
- >= 80% PASS across SHOULD section

- **APPROVE WITH CHANGES**:  
- No FAIL in MUST section  
- Any CONDITIONAL items have explicit fix list

- **REJECT**:  
- One or more FAIL in MUST section

---

## MUST Criteria (Hard Gates)

## M1. Canonical Truth Boundary
- Graph remains canonical source of truth.
- Generated artifacts are non-canonical by default.

**FAIL if:** PR introduces dual truth (graph + checked-in generated output as authoritative).

---

## M2. Worktree Independence
- Behavior does not silently depend on current checkout beyond explicit defaults.
- Context-sensitive operations expose `--at/--as-of` semantics.

**FAIL if:** Output changes due to hidden worktree coupling.

---

## M3. Determinism
- Same semantic inputs produce same outputs.
- No hidden nondeterministic sources (time/random/env/network) in pure paths.

**FAIL if:** Pure query/materialize path is nondeterministic.

---

## M4. Context Envelope Integrity
For variant outputs, context is explicit (or deterministically defaulted) and traceable:
- `asOf`
- `observer`
- `trustPolicy`
- `extensionSet`/lock

**FAIL if:** Any context dimension is implicit and unreported.

---

## M5. Materialization Discipline
- Materialization treated as derived projection output.
- Repo pollution controls exist (outside-repo default or strict in-repo guardrails).

**FAIL if:** Generated outputs can be accidentally committed without policy enforcement.

---

## M6. Contract Governance
- Machine outputs are schema-versioned.
- Breaking changes include version bump and migration notes.

**FAIL if:** Output contract changes silently.

---

## M7. Effects Safety
- External sync/effectful workflows use plan/apply split.
- Mutations emit receipts/provenance.

**FAIL if:** Side effects happen without explicit apply or traceable receipt.

---

## M8. Extension Safety
- Extension behavior cannot bypass core invariants.
- Capability boundaries are respected.

**FAIL if:** Extension code can violate determinism/trust/provenance constraints unchecked.

---

## SHOULD Criteria (Quality + Operability)

## S1. Test Coverage Quality
- Golden tests for stable outputs
- Failure-path tests
- Edge-case tests
- (Where appropriate) fuzz/property tests

---

## S2. CI Policy Enforcement
- CI blocks forbidden generated artifacts
- CI validates schemas/contracts
- CI checks determinism where feasible

---

## S3. Observability/Debuggability
- Output includes useful provenance metadata
- Errors are actionable
- Traces are human-parseable and machine-parseable

---

## S4. Backward Compatibility Strategy
- Preserves expected behavior where promised
- Clear migration path when behavior changes

---

## S5. Performance Envelope
- No obvious pathological regressions
- Materialization/projection caches keyed deterministically
- Large graph operations are bounded and profiled

---

## S6. Security/Trust Readiness
- Clear trust mode behavior
- No trust-context leakage into default workflows
- Future trust hooks are preserved (not blocked by design)

---

## S7. DX Clarity
- CLI semantics are intuitive (`--at`, `--observer`, `--trust`)
- Docs are updated with concrete examples
- Error messages suggest next command/action

---

## S8. Scope Hygiene
- PR is focused and cohesive
- No unrelated refactors bundled in

---

## Review Output Template

```text
VERDICT: APPROVE | APPROVE WITH CHANGES | REJECT

MUST:
- M1 Canonical Truth Boundary: PASS/CONDITIONAL/FAIL
- M2 Worktree Independence: PASS/CONDITIONAL/FAIL
- M3 Determinism: PASS/CONDITIONAL/FAIL
- M4 Context Envelope Integrity: PASS/CONDITIONAL/FAIL
- M5 Materialization Discipline: PASS/CONDITIONAL/FAIL
- M6 Contract Governance: PASS/CONDITIONAL/FAIL
- M7 Effects Safety: PASS/CONDITIONAL/FAIL
- M8 Extension Safety: PASS/CONDITIONAL/FAIL

SHOULD (summary):
- PASS count: X/8
- CONDITIONAL count: Y/8
- FAIL count: Z/8

Top Risks:
1)
2)
3)

Required Changes (if any):
1)
2)
3)

SENSEI WISDOM:
<1-3 blunt lines on architectural direction and future risk>

---

Red-Flag Auto-Reject Triggers

Immediate REJECT if PR includes any of:
1.	Generated artifacts committed as canonical outputs without explicit publish policy
2.	Hidden nondeterminism in pure query/materialization path
3.	Output contract changes without schema/version update
4.	Effectful sync without plan/apply separation
5.	Unbounded extension execution that bypasses capability controls

If you want, next I can produce a **`scripts/review-checklist.sh`** scaffold that enforces the mechanical bits (artifact path checks, schema presence checks, etc.) so reviewers spend time on architecture, not clerical policing.
