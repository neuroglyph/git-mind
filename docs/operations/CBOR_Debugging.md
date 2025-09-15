---
title: CBOR Debugging
description: How to enable CBOR decode tracing when reading journal commits.
audience: [developers]
domain: [operations]
tags: [cbor, debugging]
status: stable
last_updated: 2025-09-15
---

# CBOR Debugging

Enable verbose CBOR decode logging in the journal reader to troubleshoot malformed entries or golden vector issues.

## Enable

```bash
export GITMIND_CBOR_DEBUG=1
```

The reader logs to `stderr`:

- Offsets and remaining length when a decode fails
- Bytes consumed per decoded edge

## Notes

- Only affects CBOR decode paths in `core/src/journal/reader.c`.
- Use with Docker/CI only; don’t run core binaries against real repos on host.
- Disable by unsetting or setting to `0` / `false` / `no`.

