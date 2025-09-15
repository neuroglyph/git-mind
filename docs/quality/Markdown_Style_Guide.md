# Markdown Style Guide

Table of Contents

- [Core Rules](#core-rules)
- [Practical Exceptions](#practical-exceptions)
- [Tooling](#tooling)
- [Usage](#usage)

This repository standardizes Markdown formatting to reduce noise in reviews and keep docs consistent. Pre-commit enforces these rules automatically.

## Core Rules

- Emphasis markers: use underscores
  - Italic: `_text_` (not `*text*`)
  - Bold: `__text__` (not `**text**`)
- Unordered lists: use `-` for bullets
- Ordered lists: always start with `1.` and let Markdown auto-number
- List indentation: 2 spaces
- Headings: ATX style (`#`, `##`, …)
- Code fences: use triple backticks with a language when practical

## Practical Exceptions

- Line length is not enforced for readability and diffs (`MD013` disabled)
- Limited inline HTML is allowed (e.g., `<img>`, `<br>`, `<details>`, `<summary>`, superscripts/subscripts)

## Tooling

- Linter/formatter: `markdownlint-cli2`
  - Config: `.markdownlint.jsonc`
  - Pre-commit hooks: auto-fix first, then lint

## Usage

- One-time: `pre-commit install`
- Run on all files: `pre-commit run --all-files`
- Run just markdown fixes/lint:
  - `pre-commit run markdownlint-cli2-fix --all-files`
  - `pre-commit run markdownlint-cli2 --all-files`

If a rule fights readability, propose a tweak to `.markdownlint.jsonc` rather than inlining exceptions.
