---
title: Centralized CBOR Keys and Public Encoders
description: Explorations and design discussions.
audience: [contributors]
domain: [project]
tags: [discussion]
status: archive
last_updated: 2025-09-15
---

# Centralized CBOR Keys and Public Encoders

Wire formats drift when every module invents its own constants. We centralized CBOR field keys in a single header and exposed public encode/decode helpers. That small move pays huge dividends: writers are consistent, readers are simpler, and schema evolution becomes a surgical change—not a scavenger hunt. It also makes fuzzing and differential testing easier—one place to feed, many places to validate.

Encoders return typed Results, and decoders fail loud and early with precise errors. Combined with OID‑first fields, it’s very hard to misuse the API accidentally. The journal, cache, and CLI all speak “git‑mind over CBOR” through these helpers, so the on‑wire language stays crisp.

```mermaid
flowchart LR
  K[CBOR Keys
  (one header)] --> W[Writer
  (public helper)]
  K --> R[Reader
  (public helper)]
  W --> J[Journal
  append]
  R --> C[CLI / Tools]
  R --> S[Services]
```

One vocabulary. Many speakers. Fewer bugs.
