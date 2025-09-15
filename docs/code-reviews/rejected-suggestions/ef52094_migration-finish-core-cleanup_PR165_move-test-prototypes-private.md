---
title: Review Artifacts
description: Preserved review artifacts and rationale.
audience: [contributors]
domain: [quality]
tags: [review]
status: archive
last_updated: 2025-09-15
---

Title: Defer moving internal test prototypes to a private header

Context
- Commit: ef52094
- Branch: migration/finish-core-cleanup
- PR: #165 (CodeRabbit)
- Suggestion: In core/include/gitmind/cache.h, move prototypes for internal testing functions to a private header.
- Original comment: (link not available in local context)

Decision
- Deferred (not rejected on principle).

Rationale
- Current exposure is used by unit tests via Meson targets; a quick split would expand changes beyond the current OID-first + CI scope.
- To keep diffs minimal and land critical fixes, we will stage a focused follow-up to relocate test-only symbols to a private header within core/src/cache/ or an internal include path.

Risks
- Limited public surface area remains temporarily larger than ideal.

Plan
- Track as follow-up: introduce core/include-internal/ for non-public headers; update Meson to avoid surfacing these to consumers; shrink public API accordingly.

