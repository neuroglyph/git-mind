**Implementation Details (Step-by-Step Checklist):**

This task involves modifying the `contrib/hooks/pre-receive.shiplog` script to parse the `trust.json` file from the incoming trust commit.

**Goal:** Read `trust.json` from the new trust commit, extract `threshold`, `sig_mode`, and `maintainers` list.

**Estimated Time:** 2 hours
**Difficulty:** 2/5
**Complexity:** 2/5

---

**Step 0: Understand the Environment and Tools**

*   **`pre-receive.shiplog`:** This is a Bash script that runs on the Git server *before* a push is accepted. It receives `old_sha`, `new_sha`, and `refname` as input for each ref being pushed.
*   **`git show <commit>:<path>`:** This Git command allows you to retrieve the content of a file at a specific commit. We will use this to get `trust.json` from the `new_sha`.
*   **`jq`:** This is a command-line JSON processor. We will use it to parse the `trust.json` content.
*   **`mktemp`:** A command to create temporary files or directories. We will use this to store the `trust.json` content temporarily.
*   **`trap`:** A Bash built-in command to execute commands when the script exits (e.g., to clean up temporary files).

---

**Step 1: Locate and Understand the Relevant Section in `pre-receive.shiplog`**

1.  **Open the file:** Using your preferred text editor (e.g., `nano`, `vim`, `code`), open `contrib/hooks/pre-receive.shiplog`.
    *   `/Users/james/git/git-mind/shiplog/contrib/hooks/pre-receive.shiplog`
2.  **Find the `validate_trust_update` function:** This function is responsible for validating pushes to the `TRUST_REF`. We will modify this function.
3.  **Identify where `load_trust_state` is called:** The `validate_trust_update` function calls `load_trust_state "$new"`. This `load_trust_state` function is where we need to ensure the `trust.json` is correctly parsed.

---

**Step 2: Modify `load_trust_state` to Parse `trust.json`**

The existing `load_trust_state` function already loads `trust.json` into `CURRENT_TRUST_JSON`. We need to extract the specific fields.

1.  **Add variables for extracted fields:** Inside `load_trust_state`, after `CURRENT_TRUST_JSON=$(git show "$commit:trust.json" 2>/dev/null || true)`, add the following lines to extract `threshold` and `sig_mode`:
    ```bash
    TRUST_THRESHOLD=$(printf '%s' "$CURRENT_TRUST_JSON" | "$JQ_BIN" -r '.threshold // 1')
    TRUST_SIG_MODE=$(printf '%s' "$CURRENT_TRUST_JSON" | "$JQ_BIN" -r '.sig_mode // "chain"')
    ```
    *   **Explanation:**
        *   `printf '%s' "$CURRENT_TRUST_JSON"`: Prints the JSON content.
        *   `"$JQ_BIN" -r '.threshold // 1'`: Uses `jq` to extract the `.threshold` field. If it's missing, it defaults to `1`. `-r` means raw output.
        *   `TRUST_THRESHOLD` and `TRUST_SIG_MODE`: These will be new global variables in the script to hold the extracted values.
2.  **Add validation for `threshold`:** After extracting `TRUST_THRESHOLD`, add a check to ensure it's a valid positive integer.
    ```bash
    if ! printf '%s' "$TRUST_THRESHOLD" | grep -Eq '^[1-9][0-9]*$'; then
      error "trust.json invalid threshold value: $TRUST_THRESHOLD"
    fi
    ```
    *   **Explanation:** `grep -Eq '^[1-9][0-9]*$'` checks if the value matches a regular expression for a positive integer. If not, it calls the `error` function (already defined in the hook) to exit with an error.
3.  **Add validation for `sig_mode`:** After extracting `TRUST_SIG_MODE`, add a check to ensure it's one of the allowed values.
    ```bash
    case "$TRUST_SIG_MODE" in
      chain|attestation) : ;; # Valid modes
      *) error "trust.json invalid sig_mode value: $TRUST_SIG_MODE" ;;
    esac
    ```
    *   **Explanation:** This `case` statement checks if `TRUST_SIG_MODE` is either "chain" or "attestation". If not, it's an error.
4.  **Make `maintainers` available (optional, for later steps):** The `maintainers` list is an array of objects. For this task, we just need to ensure `trust.json` is parsed. The actual extraction of maintainer details will be part of subsequent child issues. For now, `CURRENT_TRUST_JSON` holds the full maintainer list.

---

**Step 3: Test the Changes Locally (Manual Test)**

1.  **Save the hook:** Save your changes to `contrib/hooks/pre-receive.shiplog`.
2.  **Create a test repository:**
    *   `mkdir test_repo && cd test_repo`
    *   `git init`
    *   `git config user.name "Test User" && git config user.email "test@example.com"`
    *   `git commit --allow-empty -m "Initial commit"`
3.  **Create a bare remote:**
    *   `mkdir ../bare_remote.git && cd ../bare_remote.git`
    *   `cp ../test_repo/contrib/hooks/pre-receive.shiplog hooks/pre-receive` (Copy your modified hook)
    *   `chmod +x hooks/pre-receive`
    *   `cd ../test_repo`
    *   `git remote add origin ../bare_remote.git`
4.  **Test with a valid `trust.json`:**
    *   `mkdir .shiplog`
    *   `cat > .shiplog/trust.json <<'EOF'`
    *   `{ "version": 1, "id": "test-root", "threshold": 1, "sig_mode": "chain", "maintainers": [ { "name": "Test User", "email": "test@example.com", "role": "root" } ] }`
    *   `EOF`
    *   `git add .shiplog/trust.json`
    *   `git commit -m "Add valid trust.json"`
    *   `git push origin HEAD:refs/_shiplog/trust/root`
    *   **Expected:** Push should succeed.
5.  **Test with a malformed `trust.json`:**
    *   `cat > .shiplog/trust.json <<'EOF'`
    *   `{ "version": 1, "id": "test-root", "threshold": "one", "sig_mode": "chain", "maintainers": [ { "name": "Test User", "email": "test@example.com", "role": "root" } ] }`
    *   `EOF`
    *   `git add .shiplog/trust.json`
    *   `git commit -m "Add malformed trust.json"`
    *   `git push origin HEAD:refs/_shiplog/trust/root`
    *   **Expected:** Push should be rejected with an error about "invalid threshold value".
6.  **Test with missing `trust.json`:**
    *   `rm .shiplog/trust.json`
    *   `git rm .shiplog/trust.json`
    *   `git commit -m "Remove trust.json"`
    *   `git push origin HEAD:refs/_shiplog/trust/root`
    *   **Expected:** Push should be rejected with an error about "trust ref ... is missing trust.json".
7.  **Clean up:** `cd .. && rm -rf test_repo bare_remote.git`

---

**Step 4: Add Bats Tests (as per Test Plan)**

This step involves creating a new Bats test file or modifying an existing one to automate the manual tests from Step 3.

1.  **Create a new test file:** `test/00_trust_json_parsing.bats`
2.  **Add setup/teardown:** Use `load helpers/common` and `shiplog_standard_setup`/`shiplog_standard_teardown`.
3.  **Implement test cases:** Translate the manual test cases from Step 3 into Bats tests, using `run` and `assert_failure`/`assert_success`.
    *   Ensure the remote hook is installed in the test setup.
    *   Use `git --git-dir="$REMOTE_DIR"` for remote operations.

---

**Step 5: Commit Changes**

1.  `git add contrib/hooks/pre-receive.shiplog test/00_trust_json_parsing.bats`
2.  `git commit -m "feat(hook): Parse trust.json and validate threshold/sig_mode"`
3.  `make test` (to run all tests, including the new ones)

---

**Step 6: Update Documentation (if necessary for this sub-task)**

For this specific sub-task, documentation updates are minimal as it's an internal parsing change. The main documentation updates will be in a later sub-task.