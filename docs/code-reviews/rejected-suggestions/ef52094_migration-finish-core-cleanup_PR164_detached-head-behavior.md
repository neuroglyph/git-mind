Title: Defer explicit detached/unborn HEAD handling in journal branch discovery

Context
- Commit: ef52094
- Branch: migration/finish-core-cleanup
- PR: #164 (CodeRabbit)
- Suggestion: In core/src/journal/reader.c, verify behavior when HEAD is detached or unborn and adjust branch copy accordingly.
- Original comment: (link not available in local context)

Decision
- Deferred for a focused follow-up.

Rationale
- Current code fails fast with an error if HEAD is unavailable, which is acceptable for CI and typical flows.
- Adding alternate resolution (e.g., using refs/heads/<default> or falling back to a provided branch) is useful but orthogonal to the OID-first/CI-critical changes in this PR.

Plan
- Add explicit handling and tests in a follow-up: detect unborn HEAD and use repository default branch or require an explicit branch arg; handle detached HEAD by using the current commit’s short ref where resolvable.

