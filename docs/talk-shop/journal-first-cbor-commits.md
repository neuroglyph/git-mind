# Journal‑First CBOR Commits: Time‑Travel You Can Trust

Most systems treat metadata as an afterthought; we made it the source of truth. In git‑mind, every semantic edge is an append‑only CBOR payload committed under a dedicated ref. That journal design buys us three things at once: immutability (no in‑place mutation, ever), auditability (every change has authorship, time, and diff), and actual time‑travel (check out an old commit and you get the edges from that moment). It’s just Git, which means the transport, history, and security model are battle‑tested and familiar.

Encoding the journal in compact CBOR lets us keep commits small while remaining precise and typed. We base64 the payload in the commit message for safety, and keep the write path append‑only so merges behave just like code. The result is a semantic layer that’s not a bolt‑on database but a first‑class citizen of your repository. You can branch your ideas, review them, and merge them with confidence—exactly like you manage code.

