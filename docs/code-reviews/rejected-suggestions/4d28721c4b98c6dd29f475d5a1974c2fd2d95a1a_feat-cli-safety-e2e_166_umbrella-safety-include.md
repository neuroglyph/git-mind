Summary

- Suggestion: Consider exporting safety utilities via the umbrella (if part of public API)

Rationale for rejection

- - Safety utilities are CLI‑only and not part of the core public API we want consumers to depend on. Keeping them out of the umbrella avoids accidental external coupling.

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
