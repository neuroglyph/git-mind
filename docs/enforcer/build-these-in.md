0. Baseline

- SRP = each function does one thing
- DI = dependency injections
- Test Double Friendliness
- KISS
- YAGNI
- DRY

 1. Error Handling Revolution

Principle: Fail loudly, catch gracefully, propagate meaningfully.
- Standardize error types: gm_error_t or similar
- Include context + cause chain: e.g., gm_error_with_context(err, "while parsing manifest")
- Enforce Result<T, E> or Outcome-style patterns
- No silent failure, no raw panics
- Typed errors > generic strings
- Expose testable hooks for simulated failures (great for resilience tests)

2. Memory Architecture Revolution

Principle: Know where every byte lives and dies. Embrace arenas, pools, and lifetimes.
- Use arena allocators for ephemeral data (e.g., graph parsing sessions)
- Explicit ownership: no implicit malloc/free chaos
- Implement memory tags for observability (who’s allocating what, why, and how often)
- Preallocate common data structures — especially if you want performance
- Use zero-allocation fast paths where possible (e.g., slice views into strings/arrays)

3. Observability Built-In

Principle: Make the invisible visible.
- Structured logging (TRACE, DEBUG, INFO, WARN, ERROR)
- Tags per module ([plugin-loader], [graph-core])
- Optional per-request tracing ID
- Built-in performance timing: `gm_timer_start`, `gm_timer_stop("plugin_init")`
- Memory counters per subsystem
- Telemetry interface (opt-in, privacy-focused, exportable)

4. Testing Infrastructure Uplift

Principle: Every module is a fortress of unit and integration tests.
- Each module must have unit + black-box tests
- Use test doubles (mocks, fakes, stubs) as first-class tools
- Set up test scaffolds to simulate real plugin behavior
- Test coverage thresholds? Maybe. But favor meaningful tests over quantity.

5. Config & Convention Overhaul

Principle: Consistency wins. Predictability scales.

- Unified config format (TOML? YAML? choose one)
- No magical defaults — explicit > implicit
- Config schema validation early and loudly
- CLI options follow POSIX principles
- Convention-based file locations, pluggable overrides

6. Plugin + Extensibility Hooks

Even before the plugin system ships, architect for it:
- Hook system (on_edge_created, etc.) — even if they’re no-ops now
- Observability wrappers around extension points
- Start codifying “extension-safe” APIs
- Dependency graph clean enough to support plugins loading lazily

7. Security Hygiene as Code
- All unsafe memory usage flagged
- Input validation centralized and reusable
- Dangerous file access behind secure wrappers
- Avoid C-style string APIs like the plague
- Sandbox-awareness baked in: don’t assume full FS or net access

8. DevEx: Internal CLI for Developer Tools
- gm dev graph-dump
- gm dev plugin-simulate
- gm dev test-fixture <module>
- Toggle logs/traces with env vars
- One-liner scripts for profiling, benchmarking, inspection

9. Domain Language Purity
- Central domain types (gm_node_id_t, gm_edge_t, etc.)
- Ban raw primitives from core interfaces (e.g., char* → gm_string_t)
- Enforce semantic naming (gm_context, not ctx if it’s passed deep)

10. Future Proofing Without YAGNI Sin
- Use enums not strings for discriminated unions
- No “magic string” APIs — always use constant tables
- Abstract over file formats (JSON, TOML, Protobuf) so you can swap
- Error messages localizable even if you never do it

BONUS: GitMind-Native Metadata Layer
- Everything can trace back to a source commit, plugin, or decision
- Internal event bus that tracks:
- Who created what
- When it was modified
- Which plugin observed/acted

This turns git-mind into a living epistemology engine, not just a tool.
