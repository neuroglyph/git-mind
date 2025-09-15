---
title: Ref Name Validation
description: Building refs safely with Git-style branch shorthands.
audience: [contributors, developers]
domain: [architecture]
tags: [git, refs]
status: stable
last_updated: 2025-09-15
---

# Ref Name Validation

`git-mind` stores semantic edges under namespaced refs such as `refs/gitmind/edges/<branch>` and caches under `refs/gitmind/cache/<branch>`.

## Table of Contents

- Building Refs
- Policy
- Testing

## Building Refs

- API: `gm_build_ref(out, out_sz, prefix, branch)`
  - `prefix`: ref prefix (e.g., `refs/gitmind/edges/`)
  - `branch`: Git-style shorthand name. Slashes are allowed (e.g., `feature/foo`). Must NOT start with `refs/` to avoid double-prefixing.
  - Validation: uses libgit2 `git_reference_normalize_name` on the combined ref. Invalid refnames return an error.
  - Safety: bounds-checked via `gm_snprintf` using internal `REF_NAME_BUFFER_SIZE` for intermediate buffers; returns `GM_ERR_BUFFER_TOO_SMALL` when truncated.

## Policy

- Accept standard Git ref segment rules, including `/`.
- Disallow passing a full `refs/...` as the `branch` shorthand; callers should supply only the leaf path relative to the chosen prefix.
- Prefer constants for ref namespaces:
  - Journal: `GITMIND_EDGES_REF_PREFIX` → `refs/gitmind/edges/`
  - Cache: `GM_CACHE_REF_PREFIX` → `refs/gitmind/cache/`

## Testing

Unit tests cover:
- Valid shorthand: `main`, `feature/x` → OK
- Rejected input: `refs/heads/x` → error (double prefix prevention)

See: `core/tests/unit/test_ref_utils.c`.
