---
title: Risk Register
description: Top risks, mitigations, and contingencies for git-mind.
audience: [contributors, developers]
domain: [project]
tags: [risk]
status: draft
last_updated: 2025-09-15
---
<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->
<!-- © 2025 J. Kirby Ross / Neuroglyph Collective -->

# Risk Register

## Table of Contents

- [Executive Summary](#executive-summary)
- [Top Risks](#top-10-risks)
- [Mitigations and Contingencies](#mitigations-and-contingencies)

## Executive Summary

Risks prioritize correctness, determinism, and solo-maintainer sustainability. Ratings: Probability (Low/Med/High), Impact (1-5). See also: [Release Plans](../planning/Release_Plans.md).

## Top 10 Risks

| ID | Risk | Prob. | Impact | Mitigation | Contingency |
|----|------|-------|--------|------------|-------------|
| R1 | Hash instability across platforms | Med | 4 | FNV-1a-64 + NFC + vectors | Switch to SipHash behind macro and rebuild cache |
| R2 | CBOR b/w compat regressions | Low | 4 | Tolerant readers + golden files | Feature flag legacy decode; rollback commit |
| R3 | Cache invalidation after schema change | Med | 3 | Versioned cache; force rebuild | Auto full rebuild on mismatch |
| R4 | Advice merge performance/cycles | Med | 3 | Cap sizes/depth; detect cycles | Disable advice application via flag |
| R5 | clang-tidy churn stalling features | Med | 3 | Scope tidy gates; batch fixes | Temporarily relax non-critical checks |
| R6 | Toolchain drift in CI | Med | 3 | Pin images; verify versions | Use canaries and mark non-blocking |
| R7 | Single-maintainer bandwidth | High | 3 | Small releases; clear stopping points | Defer interop features |
| R8 | Data corruption via partial writes | Low | 4 | Append-only commits; verify after write | Recreate commit from local state |
| R9 | Memory safety regressions | Med | 4 | Safe wrappers; ASAN/UBSAN builds locally | Bisect via tests; revert change |
| R10 | License/dep conflicts (CRoaring/libgit2) | Low | 3 | Track licenses; vendor fallback | Switch to vendored or alt lib |
| R11 | AI suggestion spam/low precision | Med | 3 | Lane separation; attribution filters; manual review | Disable suggestions; branch isolation |
| R12 | Privacy of local notes/data | Med | 4 | No network in core; MCP local‑only; clear docs | Opt‑in only; sandbox tooling |

## Mitigations and Contingencies

- Keep golden vectors and CBOR fixtures in repo
- Provide flags to disable advice or cache usage for isolation
- Maintain a simple dataset generator to reproduce perf targets
