Title: Keep `c_std=c23` in Meson (no down‑level support)

Context
- Commit: 608bc03 (main)
- Suggestion: Support older C standards to prevent build failures on non‑C23 compilers.

Decision
- Rejected for now; project standard remains C23 with warnings‑as‑errors.

Rationale
- The codebase purposefully uses C23 features and safety posture (e.g., strict prototypes, aliasing, bounds helpers), and the supported environment is the containerized CI toolchain.
- Down‑leveling would dilute guarantees and broaden the test matrix without clear user benefit.
- Developers can override locally via `GITMIND_ALLOW_HOST_BUILD=1`, but CI enforcement remains on C23.

Follow‑ups
- Keep DEV_SETUP clear on the required toolchain and container workflows.
- Revisit only if a compelling downstream use case emerges.

