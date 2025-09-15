# Centralized CBOR Keys and Public Encoders

Wire formats drift when every module invents its own constants. We centralized CBOR field keys in one header and exposed public encode/decode helpers. That small move pays huge dividends: writers are consistent, readers are simpler, and evolutions to the schema become surgical changes rather than scavenger hunts. It also makes fuzzing and differential tests much easier—one place to feed, many places to validate.

By combining typed Result returns with OID‑first fields, encoders are harder to misuse and decoders fail loud and early. As a bonus, the journal and CLI reuse the same helpers, so there’s only one way to speak “git‑mind over CBOR,” and it’s the correct one.

