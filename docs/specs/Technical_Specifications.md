---
title: Technical Specifications
description: Detailed specifications for semantics, cache integration, and advice.
audience: [developers]
domain: [architecture]
tags: [specs]
status: draft
last_updated: 2025-09-15
---

# Technical Specifications

## Table of Contents

- [Executive Summary](#executive-summary)
- [Implementation Approach](#implementation-approach)
- [API Contracts](#api-contracts)
- [Schemas and Relationships](#schemas-and-relationships)
- [Key Algorithms and Business Logic](#key-algorithms-and-business-logic)

## Executive Summary

Specification for first-class semantics, cache integration, and optional advice. Focused on determinism, safety, and Git-native flows. Related: [PRD: First-Class Semantics](../PRDs/PRD-git-mind-semantics-time-travel-prototype.md), [System Architecture](../architecture/System_Architecture.md).

## Implementation Approach

- Deterministic IDs
  - `fnv1a64("gm_type:" + nfc(type))`, `fnv1a64("gm_lane:" + nfc(lane))`
  - NFC normalization via small utility (ICU not required; stick to composed forms used in CLI)
- Edge CBOR
  - Add fields: `type_name: tstr`, `lane_name: tstr`
  - Backward compatibility: decoders accept absence and default to legacy lane/type
- Cache/Query
  - Accept name filters; resolve to IDs with helpers; keep IDs internal
  - Shard bitmaps by high byte of `type_id` and `lane_id`
- Advice
  - Minimal schema; merge via hybrid CRDT with LWW scalars and OR-Set collections
  - Application limited to `symmetric` and `implies` initially

## API Contracts

Library (C):

```c
// Normalize UTF-8 to NFC; returns length written (0 on error)
size_t gm_utf8_nfc(char* dst, size_t dst_size, const char* src);

// Deterministic 64-bit IDs for semantics
uint64_t gm_sem_type_id(const char* type_name_utf8);
uint64_t gm_sem_lane_id(const char* lane_name_utf8);

// Append attributed edge (names-as-truth)
int gm_edge_append(const gm_edge_attributed_t* edge);

// List edges with filters by names; output iterator-based
int gm_list(const gm_list_filter_t* filter, gm_edge_iter_t* out_iter);
```

CLI:

```bash
git mind link <src> <tgt> --type <str> [--lane <str>] [--confidence <0..1>]
git mind list [--type <str>] [--lane <str>] [--from <path>] [--to <path>]
git mind cohesion-report [--since <rev>] [--until <rev>] [--branch <name>]
```

## Schemas and Relationships

Edge (CBOR array):

```
[ src_sha(20), tgt_sha(20), confidence(u16-half), timestamp(u64),
  src_path(tstr), tgt_path(tstr), ulid(tstr),
  type_name(tstr), lane_name(tstr), attribution(map) ]
```

Advice (CBOR map):

```
{ kind: tstr, subject: tstr, props: map, ulid: tstr, ts: u64, actor: tstr }
```

## Key Algorithms and Business Logic

- FNV-1a-64 over NFC UTF-8 for semantic IDs; constant seed; documented vectors
- Cohesion report: diff advice and edges across two refs
  - Scalars: last by (clock||ts, actor, ulid)
  - Collections: OR-Set union with tombstones
- Cache build
  - Incremental from last processed journal commit
  - Serialize roaring bitmaps under cache ref; per-branch isolation
