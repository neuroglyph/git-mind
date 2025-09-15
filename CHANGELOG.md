# Changelog

This project follows a changelog so humans (and future us) can reason about what changed and why.

Format options:
- We aim to keep a conventional “Keep a Changelog” style for releases.
- We also keep a light “daily dev log” with timestamped entries for high‑velocity work.

## [Unreleased]

- OID‑first encode/decode across edges and attributed edges
- Journal safety: base64 CBOR, deterministic read path, public decoders
- Stable cache refs with legacy fallback and OID‑based hashing
- Header hygiene (extern "C"), private cache internals, safety sweeps (gm_snprintf)
- Integration tests: mixed CBOR journal, cache fanout/fanin, equality semantics

---

## Daily Dev Log

Use the following snippet for quick, timestamped notes as work lands. This complements the release‑oriented sections above.

```
## <YYYY‑MM‑DD>

- @ <HH:MM> – `<short_sha>` One‑liner description

> [!note]
> Optional context about the day/decisions
```

