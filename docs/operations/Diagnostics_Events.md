---
title: Diagnostics Events (Dev/Test)
description: Structured, low-volume debug breadcrumbs via an optional diagnostics port.
audience: [developers]
domain: [operations]
tags: [diagnostics, debugging]
status: draft
last_updated: 2025-10-08
---

# Diagnostics Events (Dev/Test)

Diagnostics events are optional, structured breadcrumbs for debugging in development and tests. They are separate from normal logs to avoid polluting dashboards and cardinality budgets.

## How it works

- Port: `gm_diagnostics_port` with a single `emit(component, event, kvs)` entry point.
- Default: If the port is unset, `gm_diag_emit()` is a no-op.
- Adapters:
  - `stderr_diagnostics_adapter` prints `component event k=v…` to stderr.
  - Test fake captures events in-memory for assertions.

## Wiring

```c
#include "gitmind/ports/diagnostic_port.h"
#include "core/src/adapters/diagnostics/stderr_diagnostics_adapter.h"

gm_context_t ctx = {0};
gm_stderr_diagnostics_port_init(&ctx.diag_port);
```

To disable, leave `ctx.diag_port.vtbl` unset or replace with a null adapter.

## When we emit

- Cache:
  - `rebuild_prep_failed{branch,code}`
  - `rebuild_edge_map_failed{branch,code}`
  - `rebuild_collect_write_failed{branch,code}`
  - `rebuild_meta_failed{branch,code}`
  - `rebuild_failed{branch,code}`

- Journal:
  - `journal_commit_create_failed{code}`
  - `journal_nff_retry{ref}` (non-fast-forward retry)
  - `journal_ref_update_failed{ref,code}`
  - `journal_read_message_failed{code}`
  - `journal_cbor_invalid{offset,remaining}`
  - `journal_walk_failed{code}`

## Guidance

- Keep events low-volume and specific to failure edges or rare transitions.
- Do not emit high-rate events in inner loops.
- Use diagnostics for tests and local debugging; rely on the normal logger/metrics for production observability.
