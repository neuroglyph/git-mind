---
title: Clang-Tidy Backlog Snapshot — 2025-09-29
description: File-by-file warning counts captured after GM.MVP.T001 completion.
audience: [contributors]
domain: [quality]
tags: [clang-tidy, backlog, hex-migration]
status: draft
last_updated: 2025-09-29
---

# Clang-Tidy Backlog Snapshot — 2025-09-29

After landing GM.MVP.T001 we reran `tools/docker-clang-tidy.sh` inside the CI image to refresh the warning baseline. Diff-guard reported **318 new warnings**, which expand to **766 diagnostics** across **17 files** in `clang-tidy-report-full.txt`. The table below buckets each file by migration status so we know where to focus the next cleanup passes.

Status legend:
- **Ported to hex** — Module already lives in the new hexagonal layout; warnings are follow-up hygiene tasks.
- **Not Ported Yet** — Legacy implementation still needs a full hex migration; tidy cleanup will happen alongside the port.
- **Dead code / Not needed** — _Not present in this snapshot._

| Filepath | Warning Count | Status | Remarks | Roast |
|---|---:|---|---|---|
| `core/src/adapters/fs/posix_temp_adapter.c` | 173 | Ported to hex | Reserved macro abuse, missing includes, needs naming cleanup. | _GNU_SOURCE screaming like it's 2005—maybe give it a calmer indoor voice? |
| `core/src/edge/attributed.c` | 142 | Ported to hex | Gigantic CBOR encode/decode still monolithic; tidy flags cognitive complexity. | This file is the Kaiju we keep feeding; time to slice it into domain-sized sashimi. |
| `core/src/adapters/git/libgit2_repository_port.c` | 106 | Ported to hex | Still depends on POSIX feature macros + include chaos. | POSIX macros everywhere—libgit2 called, it wants a chaperone. |
| `core/src/edge/edge.c` | 61 | Ported to hex | Legacy equality helpers still chunky; tidy hates redundant casts and branches. | If complexity were ULIDs, this file would need sharding. |
| `core/src/cache/query.c` | 60 | Not Ported Yet | Pre-port cache adapter still directly touching constants + include debt. | Still clinging to pre-port habits like a Git hook without DI. |
| `core/src/fs/path_utils.c` | 60 | Ported to hex | Analyzer catching undefined cursor + SIZE_MAX include gaps. | Apparently our cursor likes Schrödinger states—tidy disagrees. |
| `core/include/gitmind/util/memory.h` | 36 | Ported to hex | Analyzer noise from memcpy helpers lacking contracts. | `gm_memcpy_span` meanwhile: trust me bro isn't an analyzer contract. |
| `core/include/gitmind/result.h` | 30 | Ported to hex | Macro-based flow control trips tidy diff-guard (GM_TRY). | Macros are fun until clang-tidy files a restraining order. |
| `core/src/domain/attribution/env_loader.c` | 25 | Ported to hex | Needs smaller helpers; include-cleaner wants direct headers. | Twenty-branch switchboard; let's give it a service layer already. |
| `core/src/domain/cache/edge_map.c` | 18 | Ported to hex | Bitmap types not forward-declared; include list stale. | Roaring bitmaps roaring at missing header hygiene—irony noted. |
| `core/src/cache/bitmap.c` | 14 | Not Ported Yet | Old implementation ignores return codes; still needs port. | Return values ignored like an CI bot DM—it's asking for attention. |
| `core/src/edge/internal/blob_identity.c` | 13 | Ported to hex | Helper missing direct includes for context/result types. | If you want NODISCARD respect, maybe invite the header to the party. |
| `core/src/domain/attribution/defaults.c` | 12 | Ported to hex | Enum constants not directly declared; include shuffle required. | Source defaults still living on borrowed includes; time for eviction. |
| `core/src/app/cache/cache_rebuild_service.c` | 9 | Ported to hex | Uses constants without headers; minor include cleanup. | Knows about GM_PATH_MAX by rumor only—cite your sources. |
| `core/src/adapters/config/env_adapter.c` | 5 | Ported to hex | Adapter missing gm_result headers; easy include fix. | Adapter forgot its own result headers—did we move out without forwarding address? |
| `core/src/attribution/attribution.c` | 1 | Not Ported Yet | Legacy module untouched—include debts + old patterns. | Legacy monolith still waiting for the hex bus to arrive. |
| `core/include/gitmind/security/memory.h` | 1 | Ported to hex | Analyzer leak traces due to no inline docs; mostly false positive. | Analyzer yelling at memset like it's a crime scene—document the alibi. |

## Observations
- The tsunami is concentrated: three files account for 421 of the 766 diagnostics. Targeted include cleanup + feature-macro rewrites in the fs/libgit2 adapters will buy the biggest win.
- Legacy cache modules (`core/src/cache/*.c`) still need a formal port; when we move them into the domain layer we should eliminate the include debt rather than patching in place.
- Analyzer complaints in the shared headers stem from lack of explicit contracts. Adding `GM_ASSERT_NONNULL`/documentation while splitting helpers into smaller translation units should quiet those.

## Next Steps
1. Fold the POSIX adapter cleanups into the upcoming ref-utils sprint (remove feature macros, add wrapper headers, note `gm_posix_` prefixes).
2. Schedule a focused PR to slice `core/src/edge/attributed.c` into smaller domain operations, addressing both complexity and include-tidy chatter.
3. When we tackle `core/src/cache/query.c` for the hex port, bake in tidy fixes rather than band-aids.

_Recorded automatically after `./tools/docker-clang-tidy.sh` (Docker) on September 29, 2025._
