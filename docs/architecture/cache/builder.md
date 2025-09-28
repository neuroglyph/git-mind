---
title: Legacy Cache Builder Adapter
description: Thin compatibility layer that exposes the historic gm_cache_rebuild entry point.
audience: [contributors]
status: draft
domain: cache
tags: [cache, adapter]
---

# Legacy Cache Builder Adapter

## Table of Contents

- [Role](#role)
- [Call Flow](#call-flow)
- [Why It Exists](#why-it-exists)
- [Implementation Notes](#implementation-notes)
- [Deprecation Plan](#deprecation-plan)

## Role

`core/src/cache/builder.c` now exists solely to preserve the historic `gm_cache_rebuild` symbol exported by `gitmind/cache.h`. It performs basic argument validation and forwards execution to the cache rebuild service.

## Call Flow

```
app/CLI → gm_cache_rebuild(ctx, branch, force_full)
                   │
                   ▼
         gm_cache_rebuild_execute(ctx, …)
```

- Validates that `ctx`, `ctx->git_repo_port`, and `branch` are non-null.
- Returns `GM_ERR_INVALID_ARGUMENT` early when validation fails.
- Delegates all other work to the service (`core/src/app/cache/cache_rebuild_service.c`).

## Why It Exists

- **API stability**: external callers still include `gitmind/cache.h` and expect `gm_cache_rebuild` to exist.
- **Migration safety**: keeping the shim separate allowed us to rewrite the rebuild workflow without touching consumers during the first pass.

## Implementation Notes

- The file intentionally contains ~10 lines of code and has no additional dependencies beyond the public header and service contract.
- Any new logic belongs in the service (or future inbound port implementation), not here.

## Deprecation Plan

Once all first-party consumers migrate to the inbound cache command port, we can:

1. Introduce the port-backed API as the public entry point.
2. Mark `gm_cache_rebuild` as deprecated but still forwarding to the service.
3. Remove the shim only after the public API versioning story allows it.
