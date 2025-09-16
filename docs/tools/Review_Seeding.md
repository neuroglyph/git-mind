---
title: Review Seeding and Auto‑Worksheet
description: How PR review comments are harvested into a structured worksheet and committed back to the PR branch.
audience: [contributors, maintainers]
domain: [quality]
tags: [review, automation]
status: stable
last_updated: 2025-09-16
---

# Review Seeding and Auto‑Worksheet

> This documents the PR feedback workflow: scraping GitHub PR comments (reviews + discussion), pre‑filling a structured worksheet under `docs/code-reviews/PR<PR#>/<sha>.md`, and committing it back to the PR branch.

## Overview

- Script: `tools/review/seed_feedback_from_github.py`
- Make target: `make seed-review PR=<number>`
- Auto workflows:
  - Same‑repo PRs: auto‑commit seeded worksheet to the PR branch
  - Fork PRs: attempt push with bot PAT; else upload artifact + comment
- Review config:
  - `.coderabbit.yml` tunes CodeRabbit (summary‑first, caps, doc filters)
  - `.github/pull_request_template.md` guides reviewers
  - Docs‑only PRs auto‑labeled via `.github/labeler.yml`

## Local Usage

1. Create a GitHub token and export it:
   - Fine‑grained token, minimal scopes: Pull Requests: Read; Issues: Read; Metadata: Read
   - Classic token alternative: `repo` (read) is sufficient
   - macOS/Linux: `export GITHUB_TOKEN=ghp_xxx`
2. Seed a worksheet:
   - `make seed-review PR=169`
   - Output: `docs/code-reviews/PR169/<head_sha>.md`
3. Fill out `{response}` sections using Accepted/Rejected templates and commit.

## CI Automation

### Same‑repo PRs
- Workflow: `.github/workflows/auto-seed-review.yml`
- Trigger: PR opened/reopened/synchronized
- Action: seeds doc and commits directly to the PR branch using `GITHUB_TOKEN`.

### Fork PRs
- If “Allow edits from maintainers” is enabled and a bot PAT is configured:
  - Attempts to commit the seeded doc to the contributor’s branch
- Otherwise:
  - Uploads the seeded file as a build artifact
  - Adds a PR comment with instructions to copy the file into the branch

## Bot PAT Configuration (for fork PR pushes)

Use a Fine‑grained Personal Access Token:
- Repository access: Only select repositories → select this repo
- Repository permissions (minimum):
  - Contents: Read and write
  - Pull requests: Read and write
  - Issues: Read and write
  - Metadata: Read
- Add the token to repo secrets as: `GITMIND_PR_WORKSHEET_BOT`
- Ensure contributors keep “Allow edits from maintainers” enabled on PRs

The workflow will use `GITMIND_PR_WORKSHEET_BOT` to push the seeded doc to fork PR branches when allowed. If push fails (edits disabled), the artifact+comment path is used.

## Manual Workflow

- Workflow: `.github/workflows/seed-review.yml`
- Dispatch from Actions with input `pr=<number>`
- Produces an artifact (folder `review-seed`) with the generated worksheet

## Review Document Structure

- Location: `docs/code-reviews/PR<PR#>/<sha>.md`
- Contains one heading per feedback item with a fenced text block
- Replace `{response}` with:
  - Accepted template (with lesson + prevention), or
  - Rejected template (with rationale + tradeoffs)

## Best Practices

- Keep fixes in small, targeted commits; link commit SHAs into the worksheet
- Prefer rule/pattern references over fragile line numbers (e.g., MD041)
- For docs‑only PRs, rely on summary review; minimize inline nit comments
- Use `make docs-verify` before pushing to keep docs clean

## Related

- `.coderabbit.yml` — CodeRabbit tuning
- `.github/pull_request_template.md` — Reviewer guidance
- `.github/workflows/auto-seed-review.yml` — Auto‑seeding logic
- `.github/workflows/label-docs-only.yml` — Auto labeler
- `make docs-verify` — Front matter, link, TOC, title/H1 checks

## Future Work

- GitHub App for Worksheet Bot
  - Create a GitHub App that contributors can grant access to their fork. The app’s installation token would allow writing the seeded doc directly to fork branches without broad classic PAT scopes.
  - Benefits: principle‑of‑least‑privilege, per‑repo install, clean audit trail.
  - Work: app manifest, permissions (Contents:RW, PR:RW, Issues:RW, Metadata:Read), and token exchange in the workflow.
- Classic PAT for pushing to forks (if needed)
  - Replace fine‑grained PAT with a classic PAT (scope: `public_repo` for public, or `repo` for private) in the secret `GITMIND_PR_WORKSHEET_BOT` to enable pushes to user‑owned forks.
  - Trade‑off: broader scope than fine‑grained tokens.
- Auto‑open helper PR to contributor fork
  - If push to fork fails, create a helper PR in their fork with the seeded doc. This typically requires permission to fork/cross‑fork and may be restricted. Consider only if contributor experience warrants it.

