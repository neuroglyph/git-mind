---
title: Deprecation Notice: Legacy SHA-1 Byte Arrays in Edges
description: Plan to remove SHA-1 specific arrays in favor of OID-only APIs and storage.
audience: [developers]
domain: [architecture]
tags: [deprecation, oid]
status: planned
last_updated: 2025-09-15
---

# Deprecation Notice: Legacy SHA-1 Byte Arrays in Edges

Table of Contents

- [Summary](#summary)
- [Rationale](#rationale)
- [Impacted Areas](#impacted-areas)
- [Migration Plan](#migration-plan)
- [Action Items](#action-items)
- [Compatibility](#compatibility)

Date: 2025-09-14
Status: Planned (grace period)

## Summary

Edge structs (`gm_edge_t`, `gm_edge_attributed_t`) currently carry:

- Legacy SHA-1 byte arrays: `src_sha[20]`, `tgt_sha[20]`
- Preferred OIDs: `src_oid`, `tgt_oid` (type `gm_oid_t`/`git_oid`)

We have migrated all encode/decode, cache, hooks, and equality semantics to be OID‑first. Legacy SHA arrays remain only for read‑compatibility with older data.

## Rationale

`git_oid` is hash‑agnostic (SHA‑1, SHA‑256, etc.), while raw `20`‑byte arrays are SHA‑1 specific. Keeping both doubles memory footprint and risks ambiguity. Removing legacy fields simplifies the API and aligns with OID‑first design.

## Impacted Areas

- `core/include/gitmind/edge.h`
- `core/include/gitmind/edge_attributed.h`
- Any code reading/writing `src_sha`/`tgt_sha` directly

## Migration Plan

1. Grace period: continue to write OID fields and (for backward compatibility) read legacy fields in decoders when OIDs are absent.
2. Consumers should use `gm_oid_t` (`git_oid`) exclusively and compare via `git_oid_cmp`.
3. After one minor release, remove `src_sha`/`tgt_sha` from public structs and drop legacy CBOR keys from writers (readers may retain legacy acceptance for another cycle if needed).

## Action Items

- [ ] Mark legacy fields as deprecated in headers with comments and TODOs.
- [ ] Sweep modules to remove direct SHA array access in favor of OID.
- [ ] Announce in release notes and update examples.

## Compatibility

Journal and cache already use OID‑first encoding and compare semantics. Decoders backfill OIDs from legacy bytes, so older commits remain readable throughout the grace period.
