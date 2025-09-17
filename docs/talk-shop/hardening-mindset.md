---
title: The Hardening Mindset: Safety as a Feature
description: Explorations and design discussions.
audience: [contributors]
domain: [project]
tags: [discussion]
status: archive
last_updated: 2025-09-15
---

# The Hardening Mindset: Safety as a Feature

Great semantics deserve great engineering. We treat safety work—bounds‑checked formatting, consistent error codes, and growing tests—as product features. `gm_snprintf` replaces risky `snprintf`; GM_ERR_* codes show up everywhere; and tests cover the paths users actually hit (journal read/write, cache fan‑in/out, equality semantics). The goal isn’t “no crashes,” it’s graceful failure with debuggable signals.

This mindset compounds. Once the guardrails are in place, iteration gets faster: refactors are less scary, features ship with confidence, and CI tells the truth. It’s not glamorous, but it’s how software grows up without losing its soul.

```mermaid
flowchart TD
  I[Inputs] --> B[Bounds Checks
  (gm_snprintf,
  gm_memcpy_safe)]
  B --> E[Explicit Errors
  (GM_ERR_*)]
  E --> T[Tests
  (unit + integration)]
  T --> CI[CI Signals]
  CI --> F[Fast Iteration]
  F --> I
```

Safety is a feature the whole stack can rely on.
