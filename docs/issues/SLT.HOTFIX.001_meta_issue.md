# [SLT.HOTFIX.001] Enforce trust threshold in pre-receive hook (Meta-Issue)

**Description:** This meta-issue tracks the overall progress of implementing robust trust threshold enforcement in the `pre-receive.shiplog` hook. It ensures that updates to `refs/_shiplog/trust/root` are co-signed by at least the configured threshold of maintainers, preventing unauthorized or under-signed trust changes.

**Acceptance Criteria:** All child issues are completed and verified. The `pre-receive.shiplog` hook correctly enforces the trust threshold for both `chain` and `attestation` modes, and documentation is updated.

**Test Plan:** The comprehensive test plan for the overall feature is covered by the individual child issues.

**Child Issues:**
- [ ] [SLT.HOTFIX.001.1] Pre-receive hook: Parse trust.json and load maintainers
- [ ] [SLT.HOTFIX.001.2] Pre-receive hook: Implement signature collection (Chain Mode)
- [ ] [SLT.HOTFIX.001.3] Pre-receive hook: Implement signature collection (Attestation Mode)
- [ ] [SLT.HOTFIX.001.4] Pre-receive hook: Enforce threshold and provide error messages
- [ ] [SLT.HOTFIX.001.5] Pre-receive hook: Update documentation
