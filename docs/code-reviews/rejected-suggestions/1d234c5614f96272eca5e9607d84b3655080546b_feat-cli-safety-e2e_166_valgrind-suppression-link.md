---
title: Review Artifacts
description: Preserved review artifacts and rationale.
audience: [contributors]
domain: [quality]
tags: [review]
status: archive
last_updated: 2025-09-15
---

Summary
- Suggestion: Document sanitizer builds and link Valgrind suppression file.

Original comment
- https://github.com/neuroglyph/git-mind/pull/166#discussion_r2347196032

Rationale for rejection
- CI already documents and runs ASan/UBSan in `.github/workflows/core-quality.yml`. We avoid Valgrind false positives by using dynamically linked test binaries and do not maintain a global suppression file at this time.

What you did instead
- Updated the legacy doc to explicitly call out ASan/UBSan coverage and our dynamic-linking approach; noted that no repo-wide Valgrind suppression file is required.

Tradeoffs considered
- Pros: Clearer, simpler practice; fewer stale suppressions.
- Cons: Some environments may still prefer suppressions; can add later if needed.

What would make you change your mind
- Recurring unavoidable false positives in actively used test configurations that cannot be eliminated by dynamic linking.

Future Plans
- If needed, introduce a `tools/valgrind.supp` with ownership and update docs.

Confidence
- 7/10. Sanitizer CI exists; current approach suffices without suppression maintenance overhead.
