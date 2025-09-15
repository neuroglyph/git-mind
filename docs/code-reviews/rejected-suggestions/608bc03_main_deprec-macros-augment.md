Title: Keep deprecated augment macros during transition

Context
- Commit: 608bc03 (main)
- Suggestion: Remove deprecated macros in `core/include/gitmind/hooks/augment.h` to avoid postponed cleanup.

Decision
- Deferred (not removed immediately).

Rationale
- We intentionally keep compatibility macros (e.g., `get_blob_sha` → `gm_hook_get_blob_sha`) for one grace release to avoid breaking downstreams that include public headers directly.
- The header now follows repo‑wide guard naming (`GITMIND_*`) and provides new `gm_augment_*` aliases alongside the `gm_hook_*` APIs, making migration explicit.
- We will remove deprecated aliases in the next minor release; deprecation is marked in code and tracked in the deprecation notes.

Follow‑ups
- Document removal date in CHANGELOG when cutting the next release.
- Add a `#warning` gate behind a build option to help downstreams surface usage early.

