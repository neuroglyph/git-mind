# Deterministic Merges with CRDT Advice

When knowledge lives in Git, merges need to be predictable. We model edges as commits (append‑only) and layer “advice” using a small set of CRDT rules (LWW for scalars, OR‑Set for collections). That means two people (or a person and an AI) can explore semantics on branches and then converge without surprises. No hidden conflict resolvers, no opaque heuristics—just deterministic replay on top of Git’s own merge model.

This design turns collaboration into a first‑class workflow: suggest edges in a lane, filter by attribution, and promote when ready. Even the disagreements are data you can reason about, because the merge outcome is a function of history, not timing.

