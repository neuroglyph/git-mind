Title: Defer adding standalone public encode/decode APIs for attributed edges

Context
- Commit: ef52094
- Branch: migration/finish-core-cleanup
- PR: #166 (CodeRabbit)
- Suggestion: In include/gitmind.h, restore or re-implement missing public gm_edge_* and gm_edge_attributed_* APIs (notably encode/decode helpers).
- Original comment: (link not available in local context)

Decision
- Deferred. Journal IO owns CBOR framing; public API already exposes creation/formatting. We will surface pure encode/decode only if there’s a concrete consumer.

Rationale
- We’ve implemented OID-first CBOR in core and added full attributed CBOR read/write in the journal path. Duplicating low-level encode/decode as public API now would expand the surface without a demonstrated need and risks cementing wire details prematurely.

Plan
- If bindings/CLI or external users require direct encode/decode, we’ll add stable, documented helpers with compatibility guarantees in a subsequent PR.

