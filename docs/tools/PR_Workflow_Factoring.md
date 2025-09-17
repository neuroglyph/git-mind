---
title: Factoring PR Workflow to a Standalone Repo
description: Step‑by‑step guide to extract our PR seeding + worksheet + auto‑reply workflow into its own reusable repository.
audience: [maintainers, contributors]
domain: [tooling]
tags: [review, github-actions, automation]
status: draft
last_updated: 2025-09-16
---

# Factoring PR Workflow to a Standalone Repo

## Overview

This guide captures how to factor our “PR feedback → worksheet → auto-reply” workflow into a separate, reusable repository that any project can consume with minimal setup.

The goal is a portable, easy‑install review pipeline that works great for both AI agents and humans: seed a worksheet from PR comments, fill in Accepted/Rejected with rationale, and let the bot reply and (optionally) resolve threads.

## Components to Extract

Core (to externalize):
- Scripts
  - `tools/review/seed_feedback_from_github.py`
  - `tools/review/check_worksheets.py`
  - `tools/review/apply_feedback_to_github.py`
- Workflows
  - `.github/workflows/auto-seed-review.yml` — seed worksheet on PR events
  - `.github/workflows/apply-feedback.yml` — reply on push of updated worksheets
- Hooks & config (optional but helpful)
  - Pre‑push gate: `.githooks/pre-push` changes (template)
  - Pre‑commit hook: worksheet checker and curation (templates)
  - Review config: `.coderabbit.yml` suggestions
  - PR template: `.github/pull_request_template.md` guidance
  - Docs: `docs/tools/Review_Seeding.md` user guide

Nice‑to‑have extras:
- GH Labeler: `.github/labeler.yml` + docs‑only workflow
- Curator (AGENTS‑style): `tools/agents/curate_recent.py` (not required for the PR flow)

## Target Repo Layout (Example)

```
review-buddy/
  README.md
  LICENSE
  scripts/
    seed_feedback.py
    apply_feedback.py
    check_worksheets.py
  .github/
    workflows/
      seed-review.yml              # reusable workflow
      apply-feedback.yml           # reusable workflow
  action/
    apply-feedback/action.yml      # composite Action for replies
    seed-review/action.yml         # composite Action for seeding
  templates/
    pre-push                       # sample pre-push gate
    pre-commit-config.yaml         # sample pre-commit additions
    pull_request_template.md
    coderabbit.yml
  docs/
    GETTING_STARTED.md
    ADVANCED.md
```

Notes:
- Prefer both “reusable workflows” (callable with `workflow_call`) and “composite actions” for flexibility.
- Scripts live in `scripts/` and are referenced by the composite actions.

## Extraction Steps (from this repo)

1) Create the new repo, e.g., `neuroglyph/review-buddy` (MIT/Apache‑2.0 preferred).
2) Copy scripts into `scripts/` and update imports and paths to be repo‑relative.
3) Convert current workflows to reusable workflows:
   - `on: workflow_call`
   - Inputs: `pr_number`, `seed_to_branch`, toggles
4) Create composite actions (`action/*/action.yml`) that shell out to the Python scripts for easy inclusion in non‑reusable workflows.
5) Add docs (`GETTING_STARTED.md`, `ADVANCED.md`) with install steps.
6) Add an example consuming project in `examples/` (optional).

## Consuming Repo Setup

Option A — Reusable workflows:
```
# .github/workflows/seed-review.yml
name: Seed Review Doc
on:
  pull_request_target:
    types: [opened, reopened, synchronize]

jobs:
  seed:
    uses: neuroglyph/review-buddy/.github/workflows/seed-review.yml@vX.Y.Z
    secrets: inherit
```

```
# .github/workflows/apply-feedback.yml
name: Apply Feedback Replies
on:
  push:
    branches-ignore: [ main ]
    paths: [ 'docs/code-reviews/**.md' ]

jobs:
  apply:
    uses: neuroglyph/review-buddy/.github/workflows/apply-feedback.yml@vX.Y.Z
    secrets: inherit
```

Option B — Composite actions (per‑job):
```
jobs:
  seed:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - uses: neuroglyph/review-buddy/action/seed-review@vX.Y.Z
        with:
          pr: ${{ github.event.number }}
```

Secrets required (at minimum):
- `GITHUB_TOKEN` (provided by Actions)
- Optional fork push PAT: `GITMIND_PR_WORKSHEET_BOT` (or rename for the external repo)

## Pre‑push / Pre‑commit Templates

Pre‑push gate (blocks if worksheets contain `{response}` or undecided sections):
- Provide a `templates/pre-push` script and document symlink/install.

Pre‑commit templates:
- Provide a stanza to insert into `.pre-commit-config.yaml` to run the worksheet checker on docs changes.

## Configuration Surface

Top‑level `review-buddy.yml` (in consuming repo):
```
worksheet:
  dir: docs/code-reviews
  accepted_marker: "[!note]- **Accepted**"
  rejected_marker: "[!CAUTION]- **Rejected**"
  require_decision_for_all: true

seeding:
  seed_to_branch: auto      # auto | artifact_only
  bot_secret: REVIEW_BOT_PAT
  maintainers_edit_required: true

apply:
  reply_accepted: true
  reply_rejected: true
  resolve_accepted: false   # GraphQL; can be enabled later
```

The composite actions / reusable workflows should respect these conventions via environment variables and default values.

## Versioning + Release

- Tag releases `vX.Y.Z`.
- Changelog: list breaking changes for inputs/outputs.
- Provide a matrix of supported GitHub Runners and Python versions.

## “HOLY SHIT THIS IS AMAZING” Upgrades

Make it delightful for agents and humans:

1) GraphQL Auto‑Resolve
   - After replying on accepted items, resolve review threads via GraphQL mutation.
   - Map worksheet sections to original thread IDs; dry‑run mode for safety.

2) PR Dashboard Comment
   - Post a summary comment on PR with counts: total, accepted, rejected, undecided; link to worksheet.
   - Update the comment on each push or worksheet change.

3) ChatOps Commands
   - `/seed` to regenerate worksheet; `/apply` to apply replies; `/resolve-accepted` to resolve threads.
   - Gate commands by role (maintainer only) and label.

4) Web‑Preview of Worksheet
   - Serve a GitHub Pages or Action artifact as a styled HTML view of the worksheet (TOC, filters, search).
   - Provide a “focus mode” for reviewers to skim Accepted/Rejected quickly.

5) Multi‑Repo Templates
   - A single “org” repo stores default templates (.coderabbit.yml, PR template, hooks). Repos opt‑in or override.

6) Deep CodeRabbit Integration
   - Parse CR’s review metadata (if accessible) to group comments by rule/pattern.
   - Suggest fix‑it snippets and patch stacks; apply suggestions in a fresh branch via a bot.

7) Quality Signals + Badges
   - Expose a JSON “review‑health” summary: % accepted, time‑to‑address, undecided count.
   - CI gate: fail if undecided > threshold on “ready to merge” label.

8) Agent‑First Ergonomics
   - Provide a tiny API: `seed(pr)`, `apply(pr)`, `resolve(pr)` that agents can call with one line.
   - Emit machine‑readable logs with stable keys (decision=accepted/rejected, url, title, section).

9) Installer Script
   - `npx review-buddy init` to add workflows, templates, and secrets scaffolding to a repo with prompts.

10) Secure Bot Options
   - Ship a GitHub App (least privilege) as an alternative to PAT.
   - Document org‑wide vs repo‑specific secret strategies.

## Migration Checklist (from this repo)

- [ ] Create `neuroglyph/review-buddy` with scripts, workflows, actions, and docs.
- [ ] Replace in‑repo scripts with references to the external Action/workflows.
- [ ] Update docs: point to `GETTING_STARTED.md` in the new repo.
- [ ] Keep a compatibility shim for a few releases (graceful deprecation).

## Getting to “Meh → Amazing”

Deliver smooth on‑ramp + high trust:
- One‑command install; sensible defaults; crisp errors.
- Great docs with copy‑paste snippets and screenshots.
- Safety: dry‑run modes, explicit labels to enable writing.
- Telemetry: text summaries + JSON for automation.
- Extensibility: template markers for block parsing, configurable decision markers, pluggable resolvers.
