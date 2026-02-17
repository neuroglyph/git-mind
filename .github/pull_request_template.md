# PR Title

<!-- Use imperative style, e.g. "feat(materialization): add provenance envelope hashing" -->

## Summary

<!-- What changed, in 3-7 bullets max. -->
- 
- 
- 

## Problem Statement

<!-- What concrete problem does this solve? Why now? -->
- 

## ADR Compliance (Required)

### Relevant ADR(s)

<!-- List all applicable ADR IDs. If none, explicitly state "None". -->
- [ ] ADR-00XX (Worktree Independence and Materialization Architecture)
- [ ] ADR-00XY (Graph-Native Content, Deterministic Materialization, and Workspace Bridge)
- [ ] None

### Compliance Declaration

<!-- Choose one -->
- [ ] This PR is fully compliant with all checked ADRs.
- [ ] This PR intentionally deviates from one or more checked ADRs (complete Exception Request below).

### Exception Request (Required if deviating)

<!-- If you deviated, this is mandatory. -->
- **ADR clause(s) deviated from:**
- **Why deviation is necessary now:**
- **Risk introduced by deviation:**
- **Mitigation and rollback plan:**
- **Follow-up ADR/issue to reconcile architecture debt:**

---

## Architecture Laws Checklist (Hard Gates)
<!-- Any unchecked required item is a review blocker. -->

### Canonical Truth & Context

- [ ] Graph remains canonical truth (no dual truth with generated files).
- [ ] No hidden worktree coupling introduced in core/domain/materialization paths.
- [ ] Context-sensitive behavior is explicit (`--at`, `--observer`, `--trust`) or deterministically defaulted.
- [ ] Resolved context is surfaced in output metadata where applicable.

### Determinism & Provenance

- [ ] Pure query/materialization paths remain deterministic for identical inputs.
- [ ] Mutations/materializations include provenance receipts/envelopes where required.
- [ ] Cache keys (if used) are derived only from semantic inputs + pinned versions.

### Artifact Hygiene

- [ ] No forbidden generated artifact paths are tracked.
- [ ] Any generated artifacts intentionally tracked are in allowlisted publish paths only.
- [ ] Pre-commit/CI policy checks updated or confirmed valid.

### Contracts & Compatibility

- [ ] Machine-facing outputs are schema-versioned.
- [ ] Breaking contract changes include version bump + migration notes.
- [ ] Backward compatibility impact is documented below.

### Extension/Effects Safety (if applicable)

- [ ] Extension behavior does not bypass capability restrictions.
- [ ] Effectful operations use explicit plan/apply semantics and emit receipts.
- [ ] Timeouts/resource bounds are defined for new script/effect paths.

---

## Scope Control

- [ ] PR is single-purpose/cohesive (no unrelated refactors).
- [ ] Any non-essential refactor is split into separate PR(s) or explicitly justified.

## Backward Compatibility

<!-- Be precise; "none" is acceptable if true. -->
- **CLI/API contract changes:**
- **Data model/storage changes:**
- **Migration required?:**
- **User-facing behavior changes:**

## Test Plan (Required)
<!-- Include exact commands and expected outcomes. -->

### Unit

- [ ] Added/updated tests for changed logic
- Commands:

```bash
# e.g.
npm test -- test/materialization/*.test.js
```

### Integration

- [ ] Added/updated integration tests
- [ ] Commands:

```bash
# e.g.
npm run test:integration
```

### Determinism

- [ ] Determinism assertions included for relevant paths
- [ ] Method:
- [ ] Commands:

```bash
# e.g.
npm run test:determinism
```

### Contract/Schema

- [ ] Schema validation updated/passing
- [ ] Commands:

```bash
# e.g.
npm run validate:schemas
```

### Policy Gates

- [ ] Mechanical architecture gates pass
- [ ] Commands:

```bash
# e.g.
scripts/review-checklist.sh
```

---

## Security / Trust Impact

<!-- Required for trust/materialization/extension/writeback changes -->

- [ ] Threat surface changed?:
- [ ] Trust policy impact:
- [ ] Provenance/audit impact:
- [ ] New failure modes introduced:

### Performance Impact

- [ ] Hot path affected?:
- [ ] Expected impact (latency/memory/io):
- [ ] Benchmarks or profiling evidence:

### Observability / Debuggability

- [ ] Errors are actionable and include context.
- [ ] Logs/diagnostics added or updated where needed.
- [ ] git mind status / diagnostics updated if writeback/eventing behavior changed.

## Operational Notes

<!-- Runbooks, rollout, feature flags, fallback behavior -->

- [ ] Feature flag (if any):
- [ ] Rollback strategy:
- [ ] Operational caveats:

## Linked Issues / Milestones

- [ ] Closes #
- [ ] Milestone: M

---

## Reviewer Quick Verdict Block (for maintainers)

### MUST (Hard Gates)

	•	PASS
	•	CONDITIONAL
	•	FAIL

### SHOULD (Quality)

	•	PASS
	•	CONDITIONAL
	•	FAIL

### Verdict

	•	APPROVE
	•	APPROVE WITH CHANGES
	•	REJECT
