# Technical Debt Task List for Git-Mind

<progress value="144" max="165" style="width: 100%; height: 30px;"></progress>

**Progress: 144/165 tasks completed (87.3%)**

This document tracks all technical debt identified in the 2025-06-19 code audit. Each item must be addressed before proceeding to new features.

## Priority 1: Emergency Function Size Violations (Day 1-2)

### Attribution Module - CBOR Functions
- [x] **Break down `gm_edge_attributed_decode_cbor_ex()`** (172 lines → <15 lines each)
  - [x] Extract `decode_cbor_header()` - validate array header
  - [x] Extract `decode_cbor_shas()` - decode source/target SHAs
  - [x] Extract `decode_cbor_metadata()` - decode rel_type, confidence, timestamp
  - [x] Extract `decode_cbor_paths()` - decode src_path, tgt_path, ulid
  - [x] Extract `decode_cbor_attribution()` - decode attribution fields
  - [x] Extract `decode_cbor_lane()` - decode lane field
  
- [x] **Break down `gm_edge_attributed_encode_cbor()`** (111 lines → <15 lines each)
  - [x] Extract `encode_cbor_header()` - write array header
  - [x] Extract `encode_cbor_shas()` - encode source/target SHAs
  - [x] Extract `encode_cbor_metadata()` - encode rel_type, confidence, timestamp
  - [x] Extract `encode_cbor_paths()` - encode src_path, tgt_path, ulid
  - [x] Extract `encode_cbor_attribution()` - encode attribution fields
  - [x] Extract `encode_cbor_lane()` - encode lane field

### Cache Module
- [x] **Break down `gm_cache_rebuild_internal()`** (154 lines → <15 lines each)
  - [x] Extract `cache_prepare_rebuild()` - setup temp directory
  - [x] Extract `cache_collect_edges()` - gather all edges
  - [x] Extract `cache_build_edge_map()` - create edge mappings
  - [x] Extract `cache_write_bitmaps()` - write bitmap files
  - [x] Extract `cache_build_trees()` - create Git trees
  - [x] Extract `cache_create_commit()` - create cache commit
  - [x] Extract `cache_update_ref()` - update refs
  - [x] Extract `cache_cleanup()` - cleanup temp files

### CLI Module
- [x] **Break down `gm_cmd_list()`** (101 lines → <15 lines each)
  - [x] Extract `parse_list_arguments()` - argument parsing
  - [x] Extract `setup_list_filter()` - filter configuration
  - [x] Extract `execute_list_query()` - perform the query
  - [x] Extract `format_list_output()` - output formatting
  - [x] Extract `print_list_summary()` - summary output

- [x] **Break down `gm_cmd_install_hooks()`** (86 lines → <15 lines each)
  - [x] Extract `check_git_hooks_directory()`
  - [x] Extract `backup_existing_hook()`
  - [x] Extract `write_hook_script()`
  - [x] Extract `make_hook_executable()`
  - [x] Extract `verify_hook_installation()`

- [x] **Break down `gm_cmd_link()`** (80 lines → <15 lines each)
  - [x] Extract `parse_link_arguments()`
  - [x] Extract `validate_link_inputs()`
  - [x] Extract `create_edge_from_args()`
  - [x] Extract `save_edge_to_journal()`

## Priority 2: CLI Output Control System (Day 2-3)

### Create Output Abstraction
- [x] **Create `include/gitmind/output.h`**
  - [x] Define `gm_output_t` structure
  - [x] Define output levels (SILENT, NORMAL, VERBOSE)
  - [x] Define output formats (HUMAN, PORCELAIN)

- [x] **Create `src/cli/output.c`**
  - [x] Implement `gm_output_create()`
  - [x] Implement `gm_output_print()` - respects verbose flag
  - [x] Implement `gm_output_error()` - always shows errors
  - [x] Implement `gm_output_porcelain()` - porcelain mode
  - [x] Implement `gm_output_destroy()`

### Update CLI Commands
- [x] **Update `main.c`**
  - [x] Parse global `--verbose` and `--porcelain` flags
  - [x] Create output context
  - [x] Pass output context to all commands

- [x] **Update all CLI commands** to use output abstraction
  - [x] `cache_rebuild.c` - replace all printf/fprintf
  - [x] `install_hooks.c` - replace all printf/fprintf
  - [x] `link.c` - replace all printf/fprintf
  - [x] `list.c` - already uses abstraction

### Testing
- [x] **Add output control tests**
  - [x] Test --verbose flag functionality
  - [x] Test --porcelain output format
  - [x] Verify behavior across all commands

## Priority 3: Define All Constants (Day 3)

### Create Constants Headers
- [x] **Create `include/gitmind/constants_cbor.h`**
  - [x] CBOR type constants (ARRAY, BYTES, TEXT, UINT)
  - [x] CBOR size indicators
  - [x] CBOR array sizes (13 for attributed, 8 for legacy)
  - [x] CBOR field limits

- [x] **Create `include/gitmind/constants_internal.h`**
  - [x] Buffer sizes (PATH, REF, CWD, HOOK)
  - [x] Time conversions (MILLIS_PER_SECOND, NANOS_PER_MILLI)
  - [x] Git constants (EMPTY_TREE_SHA, REFS_PREFIX, HEAD_REF)
  - [x] Exit codes (SAFETY_VIOLATION)
  - [x] File permissions (HOOK_PERMS)

### Replace Magic Values
- [x] **Edge module** - replace all magic numbers/strings
- [x] **Journal module** - replace all magic numbers/strings
- [x] **Attribution module** - replace all magic numbers/strings
- [x] **CLI module** - replace all magic numbers/strings
- [x] **Cache module** - replace all magic numbers/strings
- [x] **Util module** - replace all magic numbers/strings
- [x] **Hooks module** - replace all magic numbers/strings

## Priority 4: Dependency Injection (Day 4)

### Create Abstraction Interfaces
- [x] **Create `include/gitmind/io_ops.h`**
  - [x] Define file operation callbacks
  - [x] Define directory operation callbacks
  - [x] Define process operation callbacks

- [x] **Create `include/gitmind/time_ops.h`**
  - [x] Define time() callback
  - [x] Define clock_gettime() callback

- [x] **Create `include/gitmind/random_ops.h`**
  - [x] Define rand() callback
  - [x] Define seed callback

### Update Context Structure
- [x] **Modify `gm_context_t`** to include:
  - [x] `io_ops` pointer
  - [x] `time_ops` pointer
  - [x] `random_ops` pointer
  - [x] `output` pointer (already present)

### Implement Default Providers
- [x] **Create `src/util/io_default.c`**
- [x] **Create `src/util/time_default.c`**
- [x] **Create `src/util/random_default.c`**

### Update All Modules
- [ ] **Update edge module** to use injected providers
- [ ] **Update journal module** to use injected providers
- [ ] **Update cache module** to use injected providers
- [ ] **Update util module** to use injected providers

## Priority 5: Eliminate Code Duplication (Day 5)

### Merge Duplicate Functions
- [x] **Journal module**
  - [x] Merge `process_commit()` and `process_commit_attributed()`
  - [x] Merge `walk_journal()` and `walk_journal_attributed()`
  - [x] Merge `gm_journal_append()` and `gm_journal_append_attributed()`

- [x] **Cache module**
  - [x] Merge `gm_cache_query_fanout()` and `gm_cache_query_fanin()`
  - [x] Extract common query logic

- [x] **CBOR module**
  - [x] Move duplicated functions from `cbor_decode_ex.c` to shared location
  - [x] Create `src/cbor/common.c` for shared helpers

## Priority 6: Fix Test Structure (Day 6)

### Refactor Tests for Behavior
- [x] **Update `test_behavior.sh`**
  - [x] Remove all stdout/stderr grep checks
  - [x] Use `--porcelain` mode for assertions
  - [x] Check state changes, not output

- [x] **Update `test_attribution.c`**
  - [x] Remove all printf statements
  - [x] Return proper exit codes
  - [x] Test behavior not output

- [x] **Update `test_augments.sh`**
  - [x] Remove output string checks
  - [x] Verify edge creation, not messages

### Organize by User Story
- [ ] **Create test structure**
  ```
  tests/features/
    F001-attribution/
    F002-augments/
    F003-cache/
  ```
- [ ] **Map tests to acceptance criteria**
- [ ] **Create feature documentation**

## Priority 7: Security Issues (Day 7)

### Fix TOCTOU Race Conditions
- [x] **Identify all TOCTOU vulnerabilities**
  - [x] Review code scanning alerts for specific locations
  - [x] Document each race condition scenario
  
- [x] **Fix file system race conditions**
  - [x] Replace stat() then open() patterns with atomic operations
  - [x] Use direct open/mkdir instead of stat checks
  - [x] Use lstat() instead of stat() to avoid symlink races
  - [x] Handle disappearing files gracefully

### Fix GitHub Actions Permissions
- [x] **Update workflow files**
  - [x] Add explicit permissions block to each workflow
  - [x] Use least-privilege principle (only required permissions)
  - [x] CI: read-only permissions
  - [x] Release: write for creating releases
  - [x] Code Quality: read + PR write for comments
  
- [x] **Audit all workflows**
  - [x] `.github/workflows/*.yml` - add permissions
  - [x] Verify no workflows have excessive permissions
  - [x] Document why each permission is needed

## Priority 8: API Documentation (Day 8)

### Document All Public APIs
- [x] **Edge module** - document all public functions
- [x] **Journal module** - document all public functions
- [x] **Attribution module** - document all public functions
- [x] **Cache module** - document all public functions
- [x] **Util module** - document all public functions

### Documentation Template
```c
/**
 * Brief description
 * 
 * Detailed description if needed
 * 
 * @param param1 Description
 * @param param2 Description
 * @return Return value description
 *         Error codes and meanings
 * @example
 *   Example usage
 */
```

## Completion Metrics

### Success Criteria
- [x] All functions ≤ 15 lines
- [ ] All functions ≤ 3 levels of nesting
- [x] Zero magic numbers/strings
- [x] Complete CLI output control
- [ ] Full dependency injection
- [x] No code duplication
- [x] All tests check behavior
- [x] All public APIs documented
- [ ] Project score ≥ 8/10

### Verification Checklist
- [x] Run function length checker (no violations)
- [x] Run magic value detector (no violations)
- [x] Run output control checker (no direct printf)
- [ ] Run dependency injection checker (no direct system calls)
- [ ] Run test quality checker (no stdout assertions)
- [ ] Run documentation checker (all public APIs documented)
- [ ] Run security scanner (no TOCTOU warnings)
- [ ] Verify all workflows have explicit permissions

## Timeline

- **Week 1**: Emergency fixes (Priority 1-4)
- **Week 2**: Cleanup, security, and polish (Priority 5-8)
- **Total**: 8-10 days to clear all technical debt including security issues

---
*Generated from audit findings in `.audits-c/2025-06-19-deep-dive/`*