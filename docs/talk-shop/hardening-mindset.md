# The Hardening Mindset: Safety as a Feature

Great semantics deserve great engineering. We treat safety work—bounds‑checked formatting, consistent error codes, and growing tests—as product features. gm_snprintf replaces risky snprintf; GM_ERR_* codes show up everywhere; and tests cover the paths users actually hit (journal read/write, cache fan‑in/out, equality semantics). The goal isn’t just “no crashes,” it’s graceful failure with debuggable signals.

This mindset compounds: once the guardrails are in place, iteration gets faster. Refactors are less scary, features ship with confidence, and CI tells the truth. It’s not glamorous, but it’s how software grows up without losing its soul.

