Title: Reject reserving an extra byte for CBOR batch buffer

Context
- Commit: ef52094
- Branch: migration/finish-core-cleanup
- PR: #165 (CodeRabbit)
- Suggestion: In core/src/journal/writer.c, consider reserving one extra byte in the batch buffer to avoid an additional copy.
- Original comment: (link not available in local context)

Decision
- Rejected for now.

Rationale
- The batch buffer uses a conservative CBOR encoding safety margin (CBOR_OVERFLOW_MARGIN) and commits chunks well before any risk of overrun.
- CBOR payloads are base64-encoded into the commit message with a known encoded length (sodium_base64_ENCODED_LEN), and we now explicitly NUL-terminate the message string defensively.
- Adding a perpetual +1 over-allocation for every batch does not change runtime copies and provides negligible additional safety, while increasing memory use and diverging from established sizing logic.

Risks
- Minimal. Existing checks and margins cover the identified concern.

Alternatives/Mitigations
- Keep the current overflow margin and explicit NUL-termination.
- If we later profile a hotspot that shows a real extra copy in practice, we can revisit with evidence and adjust the buffer policy in a targeted PR.

