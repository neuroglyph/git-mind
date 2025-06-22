# Test Plan: Plugin Architecture 2.0

## Objective

Ensure the plugin system is stable, secure, performant, and developer-friendly across Native, WASM, and Script plugins. Guarantee full lifecycle support (load/init/execute/unload), version compatibility, and plugin isolation.

---

## Test Matrix

| Area | Test Type | Description |
|------|-----------|-------------|
| Plugin Discovery | Unit | Scan plugin directories and parse manifests |
| Plugin Loading | Integration | Load .so/.dylib/.dll and .wasm with correct permissions |
| Manifest Parsing | Unit | Validate and parse `plugin.manifest.toml` |
| Hook Execution | Integration | Ensure registered hooks fire with correct args |
| CLI Commands | Unit | Validate CLI command binding and response |
| WASM Sandbox | Security | Restrict FS/network/memory in WASM plugins |
| ABI Version Mismatch | Regression | Reject incompatible plugins with meaningful error |
| Signature Validation | Security | Verify MIND-GPG signatures match hash |
| Crash Isolation | Stress | Ensure crashing plugin doesn’t bring down system |
| Performance Overhead | Benchmark | Measure runtime overhead with 10–50 active plugins |
| Safe Mode | Functional | Ensure system starts with `--no-plugins` flag |
| Logging | Functional | Verify plugin logs are separated and scoped |
| Permissions | Security | Ensure access is denied for undeclared capabilities |
| Hot Reload | Regression | Reload plugin changes without restart (if supported) |

---

## Acceptance Criteria

✅ Plugin loaded and initialized correctly  
✅ `init()` is called with valid context  
✅ Registered hooks are executed when triggered  
✅ CLI commands defined by plugin appear in `gm --help`  
✅ WASM plugins are sandboxed (no FS access, CPU limits)  
✅ Manifest must be valid TOML and all required fields must exist  
✅ Invalid or unsigned plugins are gracefully skipped  
✅ Plugin crashes are contained with full logs  
✅ At least one plugin can run in each supported mode:

- Native (`.so`, `.dll`, `.dylib`)
- WASM
- Script (Lua/JS)

---

## Tooling & Automation

- 🧪 `gm plugin test --all`: Runs integration tests in sandbox
- 🔄 CI: Run test suite against plugin SDK examples
- 📦 Fuzzers: TOML parser fuzzing for manifest security
- 🔍 Static analyzers for ABI compatibility

---

## Example Plugin Unit Test (Native)

```c
void test_plugin_init() {
    gm_plugin_t *plugin = gm_plugin_load_safe("./plugins/test.so", GM_SANDBOX_MEMORY);
    assert(plugin != NULL);
    assert(plugin->init != NULL);
    assert(plugin->api_version == 2);
}
````

---

## __Example Integration Test__

```
# Load sample plugin and trigger commit analysis hook
gm plugin load ./plugins/refactor-linter.wasm
gm commit analyze HEAD

# Assert output
cat ~/.gm/logs/plugins.log | grep "RefactorLinter activated"
```

---

## __Negative Test Cases__

- Missing required manifest fields
- Invalid WASM bytecode
- ABI mismatch
- Plugin crash during init
- Permissions mismatch
- Invalid GPG signature

---

## __Performance Targets__

- Plugin loading < 50ms per plugin
- Hook dispatch latency < 1ms per plugin
- WASM sandbox memory usage < 50MB per plugin
- No >5% runtime performance penalty with 10 idle plugins

---

## __Regression Checklist__

- Plugin loader fallback behavior
- Compatibility with Plugin v1
- Safe-mode CLI bypass
- Graph integrity with broken hooks
