---
title: Review Artifacts
description: Rejected suggestion and rationale.
audience: [contributors]
domain: [quality]
tags: [review]
status: archive
last_updated: 2025-09-15
---

Title: Keep `md-verify` convenience alias (non-harmful)

Context
- Commit: 5965e14
- Branch: feat/changelog-and-sweep-4
- PR: #169 (CodeRabbit)
- Suggestion: Remove `md-verify` target as redundant with `md-lint`.
- Original comment: https://github.com/neuroglyph/git-mind/pull/169#discussion_r2348131222

Decision
- Rejected (retain as a convenience alias for now).

Rationale
- Naming ergonomics: `verify` communicates “run all doc checks” and may host additional assertions later without changing contributor muscle memory.
- No maintenance cost: The alias delegates to `md-lint` and does not add complexity. We will fold in more checks here if needed.

Risks
- Minor duplication in Make targets; acceptable for clarity.

Plan
- Keep `md-verify` as an alias. If it remains 1:1 with `md-lint` long-term, we can prune in a janitor pass.

