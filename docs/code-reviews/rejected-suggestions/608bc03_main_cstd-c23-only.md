---
title: Review Artifacts
description: Preserved review artifacts and rationale.
audience: [contributors]
domain: [quality]
tags: [review]
status: archive
last_updated: 2025-09-15
---

Title: Keep C23 target in Meson (no down‑level support)

Context
- Commit: 608bc03 (main)
- Suggestion: Support older C standards to prevent build failures on non‑C23 compilers.

Decision
- Rejected for now; project standard remains C23 with warnings‑as‑errors.

Note
- Implementation detail: Meson uses `c_std=c2x` to achieve C23 semantics across compilers. CI gating still requires real C23 compilers (gcc-14/clang-20). This is not a policy change.

Rationale
- The codebase purposefully uses C23 features and safety posture (e.g., strict prototypes, aliasing, bounds helpers), and the supported environment is the containerized CI toolchain.
- Down‑leveling would dilute guarantees and broaden the test matrix without clear user benefit.
- Developers can override locally via `GITMIND_ALLOW_HOST_BUILD=1`, but CI enforcement remains on C23.

Follow‑ups
- Keep DEV_SETUP clear on the required toolchain and container workflows.
- Revisit only if a compelling downstream use case emerges.
