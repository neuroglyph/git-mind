---
title: Review Worksheet Flow
description: End-to-end guide for generating, editing, and verifying PR review worksheets.
audience: [contributors, maintainers]
domain: [quality]
tags: [review, automation]
status: draft
last_updated: 2025-09-27
---

# Review Worksheet Flow

This note captures the full lifecycle for the CodeRabbit-backed review worksheets that live under `docs/code-reviews/PR<PR#>/`. Pair it with [Review_Seeding](./Review_Seeding.md) for tooling details.

## 1. Generation

1. **Auto seed (preferred):**
   - Same-repo PRs: `.github/workflows/auto-seed-review.yml` seeds on open/reopen/sync and commits straight to the PR branch.
   - Fork PRs: the workflow tries a bot push; if that fails it attaches the worksheet as an artifact and drops instructions in a comment.
2. **Manual seed:** run `make seed-review PR=<number>` (requires `GITHUB_TOKEN`). Output path: `docs/code-reviews/PR<PR#>/<head_sha>.md`.
3. **Naming:** file name is always the PR head commit SHA. Keep older worksheets around when the PR churns—they form the audit log.

## 2. Editing the worksheet

All seeded docs share the same skeleton:

```markdown
## CODE REVIEW FEEDBACK
### <comment heading>
```text
...original feedback...
```
{response}
```

Replace `{response}` with one of the response templates (Accepted/Rejected) and capture:

- What changed (commits, follow-up PRs, doc updates)
- Why you accepted or declined the feedback
- Preventative steps or “future work” notes

> 📝 **Tip:** When feedback spans multiple commits, list each SHA with a short description, e.g. `- fix: <summary> (abc1234)`.

## 3. Decision markers

The pre-push hook (`hooks/pre-push/worksheet_guard.py`) enforces:

- Every section you touched must state `Decision: Accepted|Rejected` (plain text works).
- No stray `{response}` placeholders.
- Don’t delete sections unless you add a short note explaining the removal (e.g., duplicate of prior comment).

If you really must bypass (e.g., to land a hotfix while docs lag behind):

```bash
HOOKS_BYPASS=1 git push
```

Use sparingly; follow up with a proper decision update before closing the PR.

## 4. What goes where

| Location | Role |
| --- | --- |
| `docs/code-reviews/PR<PR#>/<sha>.md` | Primary worksheet. One file per PR head. |
| `docs/code-reviews/ignored/` | Rationale docs when a CodeRabbit suggestion is intentionally declined without modifying the code. |
| `docs/tools/Review_Seeding.md` | Automation + seeding scripts. |
| `docs/tools/Review_Worksheet_Flow.md` | (This document) human workflow. |

## 5. Merging & archiving

- Keep the worksheet file in the final merge commit; it serves as provenance.
- If new worksheets arrive mid-review (CI re-seeding), keep the previous ones for history and work from the newest SHA.
- When major PRs land, consider adding a short summary to `docs/activity/<date>_hexagonal.md` linking back to the worksheet.

## 6. Frequently asked questions

### “What if the hook still complains?”

Check each heading you modified—many times the missing `Decision:` marker sits immediately above a fenced block. The guard is case-sensitive.

### “Can I delete a worksheet?”

Only if the PR is abandoned or superseded. Otherwise the guard expects the file. If you must remove it, flip `status: archive` to `deprecated`, add a paragraph explaining why, and update the pre-push guard if necessary.

### “How do I reply from the worksheet?”

Run `make apply-feedback FILES="docs/code-reviews/PR<PR#>/*.md"` to post responses back into the PR threads (requires `GITHUB_TOKEN`). The script maps each section to its GitHub comment via metadata captured during seeding.

## 7. Quick checklist before pushing

- [ ] Worksheets exist for every commit you touched.
- [ ] Every `{response}` is replaced.
- [ ] Each section you edited includes `Decision:` and `Response:` (or the template wording).
- [ ] Docs lint passes: `make docs-verify`.
- [ ] CI: `make ci-local` (mirrors gate, keeps clang-tidy happy).

Keeping the worksheet workflow tight means less friction for reviewers—and no more surprise `HOOKS_BYPASS` pushes.
