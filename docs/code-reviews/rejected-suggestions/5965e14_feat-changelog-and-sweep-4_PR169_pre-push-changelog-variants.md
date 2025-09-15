---
title: Review Artifacts
description: Rejected suggestion and rationale.
audience: [contributors]
domain: [quality]
tags: [review]
status: archive
last_updated: 2025-09-15
---

Title: Keep canonical CHANGELOG filename (reject variants in hook)

Context
- Commit: 5965e14
- Branch: feat/changelog-and-sweep-4
- PR: #169 (CodeRabbit)
- Suggestion: Update `.githooks/pre-push` to allow case-insensitive matches and alternate filenames like `CHANGES.md`, `CHANGELOG`, etc.
- Original comment: https://github.com/neuroglyph/git-mind/pull/169#discussion_r2348131083

Decision
- Rejected (enforce a single canonical filename).

Rationale
- Tooling consistency: `tools/changelog/add_entry.py` and docs standardize on a single root `CHANGELOG.md`. Allowing variants increases ambiguity and complicates automation.
- Signal, not heuristics: The pre-push guard is meant to enforce habit on `main`, not to discover files across paths or casings.
- Fixtures do not apply: Any `CHANGES.md` under tests/ or docs/ are fixtures or historical notes and should not satisfy the guard for repository CHANGELOG policy.

Risks
- Contributors used to alternate names need to align on `CHANGELOG.md`.

Plan
- Keep hook strict on `CHANGELOG.md` at repo root. If we ever broaden policy, update both docs and tooling together.

