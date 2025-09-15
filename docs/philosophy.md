---
title: Philosophy
description: The principles guiding git-mind’s design and evolution.
audience: [all]
domain: [project]
tags: [philosophy, principles]
status: stable
last_updated: 2025-09-15
---

# Philosophy

git‑mind starts from a simple premise: the relationships we hold in our heads about code, docs, data, and people are as valuable as the artifacts themselves. Those relationships deserve first‑class treatment, versioning, and review.

Guiding principles:

- Your repo is the database. No servers required; Git is the source of truth.
- Time travel matters. Understanding at commit N shouldn’t be retroactively altered by commit N+1.
- Attribution is essential. Humans and AI can collaborate, but who said what—and when—must remain clear.
- Semantics have names. We store `type_name` and `lane_name` as UTF‑8 strings; IDs are a cache detail.
- Performance enables adoption. Roaring Bitmaps and caches keep queries fast without sacrificing journal correctness.
- CRDTs where appropriate. Advice merges and attribution use OR‑Sets and LWW for predictable outcomes.

This is a long‑term bet: treat understanding as code and you get branches of ideas, reviews of meaning, and merges that preserve history.
