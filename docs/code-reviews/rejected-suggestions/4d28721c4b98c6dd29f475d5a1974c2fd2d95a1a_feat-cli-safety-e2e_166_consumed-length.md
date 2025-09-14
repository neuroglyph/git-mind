Summary
- Suggestion: _🛠️ Refactor suggestion_

Rationale for rejection
- - Journal currently stores one CBOR edge per commit; a robust measure helper (gm_cbor_measure_edge) is not yet available. The wrapper will be replaced when the attributed decoder lands, at which point proper consumption will be implemented in the CBOR module.

What you did instead
- Maintained current approach consistent with repository conventions and near-term goals.

Tradeoffs considered
- Pros: Minimal diffs; aligns with existing code; reduces risk.
- Cons: Leaves some polish/robustness opportunities unaddressed for now.

What would make you change your mind
- Clear repository-wide policy shift or a follow-up PR enabling this safely.

Future Plans
- Revisit after core features stabilize; track in PR #166 follow-ups.

Confidence
- 7/10. Based on current CI and style norms in this repo.
