Summary
- Suggestion: Centralize path validation in core for reuse.

Original comment
- https://github.com/neuroglyph/git-mind/pull/164#discussion_r2346005404

Rationale for rejection

- This PR is scoped to CLI error messaging for invalid paths. Moving validation into core is desirable but touches public API and multiple call sites; we prefer to land the CLI fix first and do the core refactor in a focused follow‑up.

What you did instead

- Kept a small helper in the CLI (`validate_path_is_regular_file`) and reduced duplication. Behavior preserved; clearer errors.

Tradeoffs considered

- Pros: Smaller, safer diff; unblocks this PR.
- Cons: Validation is not yet reusable by other commands.

What would make you change your mind

- A follow‑up PR to add `gm_path_validate_*` APIs in core with tests and CLI adoption.

Future Plans

- Track a core refactor task to introduce standardized path helpers under `core/include/gitmind/path.h`.

Confidence

- 7/10. A small, localized change that improves clarity without expanding scope.
