---
title: 17ddad1aa3747f40ecacc0bd202e7b83edbf1d1d.md
description: Preserved review artifacts and rationale.
audience: [contributors]
domain: [quality]
tags: [review]
status: archive
---

# Code Review Feedback

| Date | Agent | SHA | Branch | PR |
|------|-------|-----|--------|----|
| 2025-09-29 | CodeRabbit (and reviewers) | `17ddad1aa3747f40ecacc0bd202e7b83edbf1d1d` | [feat/edge-port-followup](https://github.com/neuroglyph/git-mind/tree/feat/edge-port-followup "neuroglyph/git-mind:feat/edge-port-followup") | [PR#173](https://github.com/neuroglyph/git-mind/pull/173) |

## CODE REVIEW FEEDBACK

### core/include/gitmind/util/oid.h:7 — coderabbitai[bot]

```text
_⚠️ Potential issue_ | _🔴 Critical_

<details>
<summary>🧩 Analysis chain</summary>

**Verify umbrella exposure in include/gitmind.h.**

Public headers must be umbrella‑safe and reachable via the umbrella. Please confirm this header is re‑exported.

---

🏁 Script executed:

```shell
#!/bin/bash
# Expect: include/gitmind.h exists and references util/oid.h
fd -a gitmind.h include | xargs -I{} rg -n 'gitmind/util/oid\.h' {}
# Also check for duplicates/different paths:
rg -nP --glob 'include/**' '\butil/oid\.h\b'
```

Length of output: 0

---

🏁 Script executed:

```shell
#!/usr/bin/env bash
# Locate all gitmind.h files and check for util/oid.h references
find . -type f -name gitmind.h | while read file; do
  echo "---- $file ----"
  grep -Hn "util/oid\.h" "$file" || echo "no reference"
done
```

Length of output: 42

---



</details>

**Include util/oid.h in the umbrella header**  
Add the following to `include/gitmind.h` so that `core/include/gitmind/util/oid.h` is re-exported by the umbrella API:  
```diff
--- a/include/gitmind.h
+++ b/include/gitmind.h
@@ /* somewhere among other includes */
+#include <gitmind/util/oid.h>
```

<!-- suggestion_start -->

<details>
<summary>📝 Committable suggestion</summary>

> ‼️ **IMPORTANT**
> Carefully review the code before committing. Ensure that it accurately replaces the highlighted code, contains no missing lines, and has no issues with indentation. Thoroughly test & benchmark the code to ensure it meets the requirements.

```suggestion
++ b/include/gitmind.h
@@ /* somewhere among other umbrella-header includes */
 #include <gitmind/other_header.h>
#include <gitmind/util/oid.h>
 #include <gitmind/yet_another_header.h>
```

</details>

<!-- suggestion_end -->

<details>
<summary>🤖 Prompt for AI Agents</summary>

```
In core/include/gitmind/util/oid.h at line 7, the util/oid header is not being
re-exported by the umbrella API; open include/gitmind.h and add an #include
"gitmind/util/oid.h" (placed with the other util/includes or alphabetical group)
so that core/include/gitmind/util/oid.h is exposed via the umbrella header;
update any include guards or ordering if necessary to preserve compile order.
```

</details>

<!-- fingerprinting:phantom:medusa:chinchilla -->

<!-- This is an auto-generated comment by CodeRabbit -->
```

_Meta_: https://github.com/neuroglyph/git-mind/pull/173#discussion_r2387021216

{response}

### core/src/edge/attributed.c:285 — coderabbitai[bot]

```text
_⚠️ Potential issue_ | _🔴 Critical_

**Fix OID backfill length mismatch (potential OOB read)**

You pass GM_OID_RAWSZ while sourcing from out.src_sha/out.tgt_sha (GM_SHA1_SIZE). That can read past the SHA buffer or fail. Zero‑pad into a GM_OID_RAWSZ scratch first.

```diff
-    if (gm_oid_is_zero(&out.src_oid)) {
-        (void)gm_oid_from_raw(&out.src_oid, out.src_sha, GM_OID_RAWSZ);
-    }
-    if (gm_oid_is_zero(&out.tgt_oid)) {
-        (void)gm_oid_from_raw(&out.tgt_oid, out.tgt_sha, GM_OID_RAWSZ);
-    }
+    if (gm_oid_is_zero(&out.src_oid)) {
+        uint8_t tmp[GM_OID_RAWSZ];
+        gm_memset_safe(tmp, sizeof tmp, 0, sizeof tmp);
+        gm_memcpy_span(tmp, sizeof tmp, out.src_sha, GM_SHA1_SIZE);
+        (void)gm_oid_from_raw(&out.src_oid, tmp, GM_OID_RAWSZ);
+    }
+    if (gm_oid_is_zero(&out.tgt_oid)) {
+        uint8_t tmp[GM_OID_RAWSZ];
+        gm_memset_safe(tmp, sizeof tmp, 0, sizeof tmp);
+        gm_memcpy_span(tmp, sizeof tmp, out.tgt_sha, GM_SHA1_SIZE);
+        (void)gm_oid_from_raw(&out.tgt_oid, tmp, GM_OID_RAWSZ);
+    }
```
As per coding guidelines (zero outputs on error paths; use gm_memcpy_span). Based on learnings.

<!-- suggestion_start -->

<details>
<summary>📝 Committable suggestion</summary>

> ‼️ **IMPORTANT**
> Carefully review the code before committing. Ensure that it accurately replaces the highlighted code, contains no missing lines, and has no issues with indentation. Thoroughly test & benchmark the code to ensure it meets the requirements.

```suggestion
    if (gm_oid_is_zero(&out.src_oid)) {
        uint8_t tmp[GM_OID_RAWSZ];
        gm_memset_safe(tmp, sizeof tmp, 0, sizeof tmp);
        gm_memcpy_span(tmp, sizeof tmp, out.src_sha, GM_SHA1_SIZE);
        (void)gm_oid_from_raw(&out.src_oid, tmp, GM_OID_RAWSZ);
    }
    if (gm_oid_is_zero(&out.tgt_oid)) {
        uint8_t tmp[GM_OID_RAWSZ];
        gm_memset_safe(tmp, sizeof tmp, 0, sizeof tmp);
        gm_memcpy_span(tmp, sizeof tmp, out.tgt_sha, GM_SHA1_SIZE);
        (void)gm_oid_from_raw(&out.tgt_oid, tmp, GM_OID_RAWSZ);
    }
```

</details>

<!-- suggestion_end -->

<!-- fingerprinting:phantom:poseidon:chinchilla -->

<!-- This is an auto-generated comment by CodeRabbit -->
```

_Meta_: https://github.com/neuroglyph/git-mind/pull/173#discussion_r2387021226

{response}

### core/src/edge/edge.c:497 — coderabbitai[bot]

```text
_⚠️ Potential issue_ | _🔴 Critical_

**Fix OID backfill length mismatch (potential OOB read)**

As in attributed.c, passing GM_OID_RAWSZ while sourcing from GM_SHA1_SIZE buffers risks OOB. Zero‑pad into a GM_OID_RAWSZ scratch first.

```diff
-    if (gm_oid_is_zero(&edge.src_oid)) {
-        (void)gm_oid_from_raw(&edge.src_oid, edge.src_sha, GM_OID_RAWSZ);
-    }
-    if (gm_oid_is_zero(&edge.tgt_oid)) {
-        (void)gm_oid_from_raw(&edge.tgt_oid, edge.tgt_sha, GM_OID_RAWSZ);
-    }
+    if (gm_oid_is_zero(&edge.src_oid)) {
+        uint8_t tmp[GM_OID_RAWSZ];
+        gm_memset_safe(tmp, sizeof tmp, 0, sizeof tmp);
+        gm_memcpy_span(tmp, sizeof tmp, edge.src_sha, GM_SHA1_SIZE);
+        (void)gm_oid_from_raw(&edge.src_oid, tmp, GM_OID_RAWSZ);
+    }
+    if (gm_oid_is_zero(&edge.tgt_oid)) {
+        uint8_t tmp[GM_OID_RAWSZ];
+        gm_memset_safe(tmp, sizeof tmp, 0, sizeof tmp);
+        gm_memcpy_span(tmp, sizeof tmp, edge.tgt_sha, GM_SHA1_SIZE);
+        (void)gm_oid_from_raw(&edge.tgt_oid, tmp, GM_OID_RAWSZ);
+    }
```
As per coding guidelines (safe memory ops; zero on error).

<!-- suggestion_start -->

<details>
<summary>📝 Committable suggestion</summary>

> ‼️ **IMPORTANT**
> Carefully review the code before committing. Ensure that it accurately replaces the highlighted code, contains no missing lines, and has no issues with indentation. Thoroughly test & benchmark the code to ensure it meets the requirements.

```suggestion
    if (gm_oid_is_zero(&edge.src_oid)) {
        uint8_t tmp[GM_OID_RAWSZ];
        gm_memset_safe(tmp, sizeof tmp, 0, sizeof tmp);
        gm_memcpy_span(tmp, sizeof tmp, edge.src_sha, GM_SHA1_SIZE);
        (void)gm_oid_from_raw(&edge.src_oid, tmp, GM_OID_RAWSZ);
    }
    if (gm_oid_is_zero(&edge.tgt_oid)) {
        uint8_t tmp[GM_OID_RAWSZ];
        gm_memset_safe(tmp, sizeof tmp, 0, sizeof tmp);
        gm_memcpy_span(tmp, sizeof tmp, edge.tgt_sha, GM_SHA1_SIZE);
        (void)gm_oid_from_raw(&edge.tgt_oid, tmp, GM_OID_RAWSZ);
    }
```

</details>

<!-- suggestion_end -->

<details>
<summary>🤖 Prompt for AI Agents</summary>

```
In core/src/edge/edge.c around lines 492 to 497, the code calls gm_oid_from_raw
using edge.src_sha/edge.tgt_sha buffers sized GM_SHA1_SIZE but passes
GM_OID_RAWSZ which can read past the source buffer; to fix, allocate a
GM_OID_RAWSZ-byte scratch array, zero it, copy GM_SHA1_SIZE bytes from the SHA
buffer into the scratch, then call gm_oid_from_raw with the scratch and
GM_OID_RAWSZ; ensure the scratch is zeroed before use and that any failure from
gm_oid_from_raw leaves the oid zeroed (follow existing safe-memory/error-zeroing
patterns).
```

</details>

<!-- fingerprinting:phantom:poseidon:chinchilla -->

<!-- This is an auto-generated comment by CodeRabbit -->
```

_Meta_: https://github.com/neuroglyph/git-mind/pull/173#discussion_r2387021236

{response}

### core/src/util/oid.c:17 — coderabbitai[bot]

```text
_🛠️ Refactor suggestion_ | _🟠 Major_

<details>
<summary>🧩 Analysis chain</summary>

**Repo-wide check: ensure no straggler `git_oid_cmp`/raw memcmp remain**

Run a quick scan to confirm all equality moves funnel through gm_oid_equal.

---

🏁 Script executed:

```shell
#!/bin/bash
# Find legacy equality patterns that should be gm_oid_equal
rg -nP --type=c --type=h -C2 '\bgit_oid_cmp\s*\(|\bmemcmp\s*\(\s*[^,]*->id\s*,\s*[^,]*->id\s*,\s*GM_OID_RAWSZ\s*\)'
```

Length of output: 201

---



</details>

**Replace raw memcmp in OID comparison with gm_oid_equal**

* In `core/src/util/oid.c:79`, replace  
  `return memcmp(lhs->id, rhs->id, GM_OID_RAWSZ) == 0;`  
  with  
  `return gm_oid_equal(lhs, rhs);`

<details>
<summary>🤖 Prompt for AI Agents</summary>

```
In core/src/util/oid.c around lines 4 to 17 and specifically at line ~79,
replace the raw memcmp comparison `memcmp(lhs->id, rhs->id, GM_OID_RAWSZ) == 0`
with the helper `gm_oid_equal(lhs, rhs)`; update the return to `return
gm_oid_equal(lhs, rhs);` so the OID equality uses the centralized function (no
other changes needed if oid.h is already included).
```

</details>

<!-- fingerprinting:phantom:poseidon:chinchilla -->

<!-- This is an auto-generated comment by CodeRabbit -->
```

_Meta_: https://github.com/neuroglyph/git-mind/pull/173#discussion_r2387021242

{response}

### core/tests/unit/test_cache_query.c:101 — coderabbitai[bot]

```text
_🛠️ Refactor suggestion_ | _🟠 Major_

**Use gm_strcpy_safe in tests (no raw strcpy)**

Per project guidelines, tests also use safety wrappers. Replace both strcpy calls.

Apply:

```diff
-    edges[0].rel_type = GM_REL_IMPLEMENTS; edges[0].confidence = 0x3C00; strcpy(edges[0].src_path, "A"); strcpy(edges[0].tgt_path, "B");
+    edges[0].rel_type = GM_REL_IMPLEMENTS;
+    edges[0].confidence = 0x3C00;
+    (void)gm_strcpy_safe(edges[0].src_path, GM_PATH_MAX, "A");
+    (void)gm_strcpy_safe(edges[0].tgt_path, GM_PATH_MAX, "B");
@@
-    edges[1].rel_type = GM_REL_IMPLEMENTS; edges[1].confidence = 0x3C00; strcpy(edges[1].src_path, "A"); strcpy(edges[1].tgt_path, "C");
+    edges[1].rel_type = GM_REL_IMPLEMENTS;
+    edges[1].confidence = 0x3C00;
+    (void)gm_strcpy_safe(edges[1].src_path, GM_PATH_MAX, "A");
+    (void)gm_strcpy_safe(edges[1].tgt_path, GM_PATH_MAX, "C");
```

Include header if not already:
```c
#include "gitmind/security/string.h"
```
As per coding guidelines.

<details>
<summary>🤖 Prompt for AI Agents</summary>

```
In core/tests/unit/test_cache_query.c around lines 93 to 99, the test uses raw
strcpy which violates project safety guidelines; replace both
strcpy(edges[0].src_path, "A") / strcpy(edges[0].tgt_path, "B") and the pair for
edges[1] with gm_strcpy_safe(dest, sizeof dest, "literal") calls, and ensure the
file includes the safety header by adding #include "gitmind/security/string.h"
at the top if it isn't already present.
```

</details>

<!-- fingerprinting:phantom:poseidon:chinchilla -->

<!-- This is an auto-generated comment by CodeRabbit -->
```

_Meta_: https://github.com/neuroglyph/git-mind/pull/173#discussion_r2387021247

{response}

### docs/code-reviews/PR173/7445bcf00c71b6f768e9eb6b1fd99d2b1c60783f.md:10 — coderabbitai[bot]

```text
_🛠️ Refactor suggestion_ | _🟠 Major_

**Align title/H1, add ToC, and insert SPDX header**

To meet docs guidelines: make H1 match the title, add a "## Table of Contents", and place SPDX after front matter.

As per coding guidelines

```diff
 --- 
-title: 7445bcf00c71b6f768e9eb6b1fd99d2b1c60783f.md
+title: Code Review Feedback
 description: Preserved review artifacts and rationale.
 audience: [contributors]
 domain: [quality]
 tags: [review]
 status: archive
 ---
 
-# Code Review Feedback
+<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->
+
+# Code Review Feedback
+
+## Table of Contents
+- Overview
+- Commits Reviewed
+- Notes
+- Finishing Touches
```

<!-- suggestion_start -->

<details>
<summary>📝 Committable suggestion</summary>

> ‼️ **IMPORTANT**
> Carefully review the code before committing. Ensure that it accurately replaces the highlighted code, contains no missing lines, and has no issues with indentation. Thoroughly test & benchmark the code to ensure it meets the requirements.

```suggestion
---
title: Code Review Feedback
description: Preserved review artifacts and rationale.
audience: [contributors]
domain: [quality]
tags: [review]
status: archive
---

<!-- SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 -->

# Code Review Feedback

## Table of Contents
- Overview
- Commits Reviewed
- Notes
- Finishing Touches
```

</details>

<!-- suggestion_end -->

<details>
<summary>🤖 Prompt for AI Agents</summary>

```
In docs/code-reviews/PR173/7445bcf00c71b6f768e9eb6b1fd99d2b1c60783f.md around
lines 1 to 10, the document needs to follow docs guidelines: make the H1 match
the title, add a "## Table of Contents" section, and insert an SPDX header
immediately after the YAML front matter; update the top-level heading to exactly
match the title line, add a "## Table of Contents" line beneath the H1, and add
a standard SPDX license identifier line (e.g., "SPDX-License-Identifier: MIT")
directly after the closing front-matter delimiter.
```

</details>

<!-- fingerprinting:phantom:poseidon:chinchilla -->

<!-- This is an auto-generated comment by CodeRabbit -->
```

_Meta_: https://github.com/neuroglyph/git-mind/pull/173#discussion_r2387021252

{response}

### docs/code-reviews/PR173/7445bcf00c71b6f768e9eb6b1fd99d2b1c60783f.md:419 — coderabbitai[bot]

```text
_⚠️ Potential issue_ | _🟡 Minor_

**Drop leftover `{response}` placeholder**

The placeholder renders verbatim and is confusing in a static artifact. Remove it.


```diff
- {response}
-
```

<!-- suggestion_start -->

<details>
<summary>📝 Committable suggestion</summary>

> ‼️ **IMPORTANT**
> Carefully review the code before committing. Ensure that it accurately replaces the highlighted code, contains no missing lines, and has no issues with indentation. Thoroughly test & benchmark the code to ensure it meets the requirements.

```suggestion
_Meta_: https://github.com/neuroglyph/git-mind/pull/173#issuecomment-3345476880

```

</details>

<!-- suggestion_end -->

<details>
<summary>🤖 Prompt for AI Agents</summary>

```
In docs/code-reviews/PR173/7445bcf00c71b6f768e9eb6b1fd99d2b1c60783f.md around
lines 122 to 125, there is a leftover literal "{response}" placeholder that
renders verbatim and is confusing; remove the "{response}" token and any
surrounding placeholder markers so the file contains only the intended static
text (or replace it with the correct static content if one was intended), then
save and run a quick preview to confirm no stray placeholders remain.
```

</details>

<!-- fingerprinting:phantom:poseidon:chinchilla -->

<!-- This is an auto-generated comment by CodeRabbit -->
```

_Meta_: https://github.com/neuroglyph/git-mind/pull/173#discussion_r2387021260

{response}

### General comment — coderabbitai[bot]

```text
<!-- This is an auto-generated comment: summarize by coderabbit.ai -->
<!-- walkthrough_start -->

## Walkthrough
Adds a new gm_oid utility API (equal/is_zero/from_raw/from_hex) and migrates core modules and tests to it, replacing libgit2 OID helpers. Updates cache/journal flows to use new helpers and safer init. Cleans up review artifacts, updates docs, and logs activity.

## Changes
| Cohort / File(s) | Summary |
| --- | --- |
| — OID utilities API<br>`core/include/gitmind/util/oid.h`, `core/src/util/oid.c` | Add gm_oid_equal, gm_oid_is_zero, gm_oid_from_raw, gm_oid_from_hex; retain gm_oid_to_hex; implement hex parsing and defensive checks. |
| — Cache & journal integration<br>`core/src/cache/query.c`, `core/src/journal/writer.c` | Swap legacy hex/OID helpers for gm_oid_*; zero-init cache meta; use gm_oid_is_zero, gm_oid_from_hex/raw; adjust commit OID param type. |
| — Edge/Hook/Domain integration<br>`core/src/edge/edge.c`, `core/src/edge/attributed.c`, `core/src/hooks/augment.c`, `core/src/domain/cache/edge_map.c` | Replace git_oid_* usage with gm_oid_* across comparisons, encode/decode, and backfill; include new header. |
| — Tests migration<br>`core/tests/unit/test_cache_branch_limits.c`, `.../test_cache_edge_map.c`, `.../test_cache_query.c`, `.../test_cache_shard_distribution.c`, `.../test_edge_attributed_cbor.c`, `.../test_edge_cbor_oid.c`, `.../test_edge_equal_semantics.c`, `.../test_edge_oid_fallback.c`, `.../test_journal_mixed_cbor.c`, `.../test_journal_safety.c` | Replace git_oid_fromraw/compare with gm_oid_from_raw/equal/is_zero; add GM_OK asserts; include new headers. |
| — Docs updates<br>`AGENTS.md`, `docs/activity/2025-09.md` | Update project notes and add 2025‑09‑29 activity entry; timestamp refresh. |
| — Docs cleanup/additions (review artifacts)<br>`docs/code-reviews/PR172/*.md`, `docs/code-reviews/PR173/7445bcf00c71b6f768e9eb6b1fd99d2b1c60783f.md` | Delete PR172 review artifacts; add PR173 review artifact document. |

## Sequence Diagram(s)
```mermaid
sequenceDiagram
  autonumber
  actor Caller as CacheUser
  participant CQ as cache/query.c
  participant OID as gm_oid utils
  participant Meta as CacheMeta

  Caller->>CQ: gm_cache_is_stale(repo, branch)
  CQ->>Meta: read meta (tip oids)
  alt binary tip present
    CQ->>OID: gm_oid_is_zero(meta.journal_tip_oid_bin)
    alt not zero
      CQ->>OID: gm_oid_equal(meta.journal_tip_oid_bin, current_tip.oid)
      OID-->>CQ: equal?/not
    else zero
      CQ->>OID: gm_oid_from_hex(&cached_tip, meta.journal_tip_oid)
      OID-->>CQ: GM_OK/ERR
    end
  else no binary tip
    CQ->>OID: gm_oid_from_hex(&cached_tip, meta.journal_tip_oid)
    OID-->>CQ: GM_OK/ERR
  end
  CQ-->>Caller: stale?/fresh status
```

```mermaid
sequenceDiagram
  autonumber
  actor Producer as EdgeEncoder
  participant EDGE as edge/edge.c
  participant OID as gm_oid utils

  Producer->>EDGE: encode/decode edge
  EDGE->>OID: gm_oid_is_zero(src_oid/tgt_oid)
  alt OIDs present
    EDGE->>OID: gm_oid_equal(...) for comparisons
  else fallback
    EDGE->>OID: gm_oid_from_raw(&oid, sha_raw, GM_OID_RAWSZ)
    OID-->>EDGE: GM_OK/ERR
  end
  EDGE-->>Producer: result (edge state)
```

## Estimated code review effort
🎯 3 (Moderate) | ⏱️ ~25 minutes

## Possibly related PRs
- neuroglyph/git-mind#172 — Prior OID utilities introduction and integration; overlaps with same OID paths and replacements.
- neuroglyph/git-mind#169 — Earlier OID-first migration touching refs/cache/journal; closely related code areas.
- neuroglyph/git-mind#171 — Repository interaction migration that intersects with files now updated to gm_oid_*.

## Poem
> A nibble of hex, a hop through bytes,  
> I twitch my ears at zeroed nights.  
> From raw to oid, I softly tread—  
> Equal or not? “GM_OK,” I said.  
> Caches tidy, journals sing—  
> Thump-thump! New seams in everything. 🐇✨

<!-- walkthrough_end -->


<!-- pre_merge_checks_walkthrough_start -->

## Pre-merge checks and finishing touches
<details>
<summary>❌ Failed checks (2 warnings)</summary>

|     Check name     | Status     | Explanation                                                                                                                                                                                                                                                          | Resolution                                                                                                                                                                                                                              |
| :----------------: | :--------- | :------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | :-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
|  Description Check | ⚠️ Warning | The pull request description provides a summary of the changes and testing instructions but omits the required Change Areas and Risk sections defined in the repository’s description template, and it does not include the prescribed Code Review Guidance section. | Please update the description to include the Change Areas checklist and the Risk section with appropriate checkboxes from the repository template and add the Code Review Guidance section to fully comply with the required structure. |
| Docstring Coverage | ⚠️ Warning | Docstring coverage is 35.14% which is insufficient. The required threshold is 80.00%.                                                                                                                                                                                | You can run `@coderabbitai generate docstrings` to improve docstring coverage.                                                                                                                                                          |

</details>
<details>
<summary>✅ Passed checks (1 passed)</summary>

|  Check name | Status   | Explanation                                                                                                                                                                                                                                       |
| :---------: | :------- | :------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Title Check | ✅ Passed | The pull request title follows a conventional commit format by indicating a new feature in the core OID module and succinctly describes migrating edge and cache code to gm helpers, which directly reflects a significant part of the changeset. |

</details>

<!-- pre_merge_checks_walkthrough_end -->

<!-- finishing_touch_checkbox_start -->

<details>
<summary>✨ Finishing touches</summary>

- [ ] <!-- {"checkboxId": "7962f53c-55bc-4827-bfbf-6a18da830691"} --> 📝 Generate Docstrings
<details>
<summary>🧪 Generate unit tests</summary>

- [ ] <!-- {"checkboxId": "f47ac10b-58cc-4372-a567-0e02b2c3d479", "radioGroupId": "utg-output-choice-group-unknown_comment_id"} -->   Create PR with unit tests
- [ ] <!-- {"checkboxId": "07f1e7d6-8a8e-4e23-9900-8731c2c87f58", "radioGroupId": "utg-output-choice-group-unknown_comment_id"} -->   Post copyable unit tests in a comment
- [ ] <!-- {"checkboxId": "6ba7b810-9dad-11d1-80b4-00c04fd430c8", "radioGroupId": "utg-output-choice-group-unknown_comment_id"} -->   Commit unit tests in branch `feat/edge-port-followup`

</details>

</details>

<!-- finishing_touch_checkbox_end -->

<!-- tips_start -->

---

Thanks for using CodeRabbit! It's free for OSS, and your support helps us grow. If you like it, consider giving us a shout-out.

<details>
<summary>❤️ Share</summary>

- [X](https://twitter.com/intent/tweet?text=I%20just%20used%20%40coderabbitai%20for%20my%20code%20review%2C%20and%20it%27s%20fantastic%21%20It%27s%20free%20for%20OSS%20and%20offers%20a%20free%20trial%20for%20the%20proprietary%20code.%20Check%20it%20out%3A&url=https%3A//coderabbit.ai)
- [Mastodon](https://mastodon.social/share?text=I%20just%20used%20%40coderabbitai%20for%20my%20code%20review%2C%20and%20it%27s%20fantastic%21%20It%27s%20free%20for%20OSS%20and%20offers%20a%20free%20trial%20for%20the%20proprietary%20code.%20Check%20it%20out%3A%20https%3A%2F%2Fcoderabbit.ai)
- [Reddit](https://www.reddit.com/submit?title=Great%20tool%20for%20code%20review%20-%20CodeRabbit&text=I%20just%20used%20CodeRabbit%20for%20my%20code%20review%2C%20and%20it%27s%20fantastic%21%20It%27s%20free%20for%20OSS%20and%20offers%20a%20free%20trial%20for%20proprietary%20code.%20Check%20it%20out%3A%20https%3A//coderabbit.ai)
- [LinkedIn](https://www.linkedin.com/sharing/share-offsite/?url=https%3A%2F%2Fcoderabbit.ai&mini=true&title=Great%20tool%20for%20code%20review%20-%20CodeRabbit&summary=I%20just%20used%20CodeRabbit%20for%20my%20code%20review%2C%20and%20it%27s%20fantastic%21%20It%27s%20free%20for%20OSS%20and%20offers%20a%20free%20trial%20for%20proprietary%20code)

</details>

<sub>Comment `@coderabbitai help` to get the list of available commands and usage tips.</sub>

<!-- tips_end -->

<!-- internal state start -->


<!-- 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= -->

<!-- internal state end -->
```

_Meta_: https://github.com/neuroglyph/git-mind/pull/173#issuecomment-3345476880

{response}

