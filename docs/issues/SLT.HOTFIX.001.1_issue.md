# [SLT.HOTFIX.001.1] Pre-receive hook: Parse trust.json and load maintainers

**Description:** Modify `contrib/hooks/pre-receive.shiplog` to read the `trust.json` file from the new trust commit and extract the `threshold`, `sig_mode`, and `maintainers` list.

**Acceptance Criteria:**
*   The `pre-receive.shiplog` hook successfully parses `trust.json` from the new trust commit.
*   The `threshold`, `sig_mode`, and `maintainers` list are correctly extracted and made available for subsequent processing within the hook.
*   The hook rejects the push with a clear error message if `trust.json` is malformed or missing from the trust commit.

**Test Plan:**
*   **Setup:** Create a bare Git remote repository with the `pre-receive.shiplog` hook installed. Set up a local repository with `git shiplog init`.
*   **Test Case (Success - Valid `trust.json`):**
    1.  Create a valid `trust.json` with `threshold`, `sig_mode`, and `maintainers`.
    2.  Create a trust commit with this `trust.json` and push it.
    3.  **Expected:** Push succeeds (no error from parsing `trust.json`).
*   **Test Case (Failure - Missing `trust.json`):**
    1.  Create a trust commit that *does not* contain a `trust.json` file.
    2.  Push this commit.
    3.  **Expected:** Push is rejected with an error indicating `trust.json` is missing.
*   **Test Case (Failure - Malformed `trust.json`):**
    1.  Create a trust commit with a malformed `trust.json` (e.g., invalid JSON syntax).
    2.  Push this commit.
    3.  **Expected:** Push is rejected with an error indicating `trust.json` is malformed.

**Parent Issue:** SLT.HOTFIX.001 (Meta-Issue)