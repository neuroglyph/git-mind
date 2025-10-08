---
title: docs/ (Index and Structure)
description: Index of documentation with stable links and key entry points.
audience: [all]
domain: [project]
tags: [index, docs]
status: stable
last_updated: 2025-09-15
---

# docs/ (Index and Structure)

Table of Contents

- [Overview](#overview)
- [Planned Structure](#planned-structure)
- [Quick Map of Existing Docs](#quick-map-of-existing-docs)
- [Getting Started](#getting-started)
 - [Docs Conventions](#docs-conventions)

## Overview

This index organizes documentation with stable links so the project can be paused and resumed without confusion.

- Start here if you’re new: [Journal Architecture Pivot](architecture/journal-architecture-pivot.md)
- Semantics prototype PRD: [PRD: First-Class Semantics](PRDs/PRD-git-mind-semantics-time-travel-prototype.md)
- ADR 0001 (Semantics): [ADR 0001](adr/0001-first-class-semantics.md)
- Planning hub: [Planning docs](planning/)
- Developer setup: [DEV_SETUP](DEV_SETUP.md)
- Static analysis: [Static Analysis Guide](quality/static-analysis-guide.md), [Tidy Crusade](quality/TIDY_CLANG_CRUSADE.md)
- Local CI parity (Docker): [Local CI Parity](quality/local-ci-parity.md)

## Planned Structure

- Architecture: [docs/architecture](architecture/)
- Planning: [Product Roadmap](planning/Product_Roadmap.md), [Release Plans](planning/Release_Plans.md), [Milestones](planning/Milestones.md), [Sprint Plans](planning/Sprint_Plans.md), [Legacy](../.legacy/)
- Specs: [Technical Specifications](specs/Technical_Specifications.md)
- Testing: [Test Plan](testing/Test_Plan.md)
- Deployment: [Deployment](deployment/Deployment.md)
- Requirements: [Requirements](requirements/Requirements.md)
- Risk: [Risk Register](risk/Risk_Register.md)
- Charter: [Project Charter](charter/Project_Charter.md)

## Quick Map of Existing Docs

- Architecture and design
  - [Journal Architecture](architecture/journal-architecture.md)
  - [Cache Architecture](architecture/cache-architecture.md)
  - [Modular Restructure Plan](architecture/MODULAR_RESTRUCTURE_PLAN.md)
- CLI
  - [CLI Overview](cli/gitmind.md)
  - [git mind link](cli/gitmind-link.md)
  - [git mind list](cli/gitmind-list.md)
  - [git mind cache-rebuild](cli/gitmind-cache-rebuild.md)
  - [git mind install-hooks](cli/gitmind-install-hooks.md)
- PRDs and ADRs
  - [PRD: First-Class Semantics](PRDs/PRD-git-mind-semantics-time-travel-prototype.md)
  - [PRD: Co‑Thought MCP Service](PRDs/PRD-co-thought-mcp-service.md)
  - [ADR 0001: First-Class Semantics](adr/0001-first-class-semantics.md)
- Ops and quality
  - [Developer Setup](DEV_SETUP.md)
  - [Environment Variables](operations/Environment_Variables.md)
  - [Static Analysis Guide](quality/static-analysis-guide.md)
  - [Tidy Crusade](quality/TIDY_CLANG_CRUSADE.md)
  - [Tools vs Custom Checks](quality/tools-vs-custom-checks.md)
- Status and sitreps
  - [SITREPs](sitrep/)
  - [Wish list features](wish-list-features/)
  - [Legacy mapping](Legacy_Mapping.md)

If you add a new doc, link it here under the right section.

## Getting Started

- Install/build: [install.md](install.md)
- Tutorial: [tutorial.md](tutorial.md)
- Philosophy: [philosophy.md](philosophy.md)
- Technical overview: [TECHNICAL.md](TECHNICAL.md)
- Roadmap: [roadmap.md](roadmap.md)

## Docs Conventions

- Frontmatter required: Every curated doc begins with a YAML block containing
  - `title`, `description`, `audience` (e.g., `[users, developers]`), `domain` (e.g., `[architecture]`), `tags`, `status`, and optional `last_updated`.
  - Example:
    ---
    title: Page Title
    description: One-line summary
    audience: [developers]
    domain: [architecture]
    tags: [journal, cache]
    status: stable
    last_updated: 2025-09-15
    ---
- Table of Contents: Major docs include a simple TOC section; `tools/docs/check_docs.py --mode toc` enforces presence for designated pages.
- Link hygiene: Use relative links; `tools/docs/check_docs.py --mode link` validates links in CI and pre-commit.
- Style: Follow `docs/quality/Markdown_Style_Guide.md`. Use consistent headings, bullet markers, and code fences.
