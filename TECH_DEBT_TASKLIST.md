# Technical Debt Task List for Git-Mind

This document tracks all technical debt identified in the 2025-06-19 code audit. Each item must be addressed before proceeding to new features.

## Priority 1: Emergency Function Size Violations (Day 1-2)

### Attribution Module - CBOR Functions
- [ ] **Break down `gm_edge_attributed_decode_cbor_ex()`** (172 lines → <15 lines each)
  - [ ] Extract `decode_cbor_header()` - validate array header
  - [ ] Extract `decode_cbor_shas()` - decode source/target SHAs
  - [ ] Extract `decode_cbor_metadata()` - decode rel_type, confidence, timestamp
  - [ ] Extract `decode_cbor_paths()` - decode src_path, tgt_path, ulid
  - [ ] Extract `decode_cbor_attribution()` - decode attribution fields
  - [ ] Extract `decode_cbor_lane()` - decode lane field
  
- [ ] **Break down `gm_edge_attributed_encode_cbor()`** (111 lines → <15 lines each)
  - [ ] Extract `encode_cbor_header()` - write array header
  - [ ] Extract `encode_cbor_shas()` - encode source/target SHAs
  - [ ] Extract `encode_cbor_metadata()` - encode rel_type, confidence, timestamp
  - [ ] Extract `encode_cbor_paths()` - encode src_path, tgt_path, ulid
  - [ ] Extract `encode_cbor_attribution()` - encode attribution fields
  - [ ] Extract `encode_cbor_lane()` - encode lane field

### Cache Module
- [ ] **Break down `gm_cache_rebuild_internal()`** (154 lines → <15 lines each)
  - [ ] Extract `cache_prepare_rebuild()` - setup temp directory
  - [ ] Extract `cache_collect_edges()` - gather all edges
  - [ ] Extract `cache_build_edge_map()` - create edge mappings
  - [ ] Extract `cache_write_bitmaps()` - write bitmap files
  - [ ] Extract `cache_build_trees()` - create Git trees
  - [ ] Extract `cache_create_commit()` - create cache commit
  - [ ] Extract `cache_update_ref()` - update refs
  - [ ] Extract `cache_cleanup()` - cleanup temp files

### CLI Module
- [ ] **Break down `gm_cmd_list()`** (101 lines → <15 lines each)
  - [ ] Extract `parse_list_arguments()` - argument parsing
  - [ ] Extract `setup_list_filter()` - filter configuration
  - [ ] Extract `execute_list_query()` - perform the query
  - [ ] Extract `format_list_output()` - output formatting
  - [ ] Extract `print_list_summary()` - summary output

- [ ] **Break down `gm_cmd_install_hooks()`** (86 lines → <15 lines each)
  - [ ] Extract `check_git_hooks_directory()`
  - [ ] Extract `backup_existing_hook()`
  - [ ] Extract `write_hook_script()`
  - [ ] Extract `make_hook_executable()`
  - [ ] Extract `verify_hook_installation()`

- [ ] **Break down `gm_cmd_link()`** (80 lines → <15 lines each)
  - [ ] Extract `parse_link_arguments()`
  - [ ] Extract `validate_link_inputs()`
  - [ ] Extract `create_edge_from_args()`
  - [ ] Extract `save_edge_to_journal()`

## Priority 2: CLI Output Control System (Day 2-3)

### Create Output Abstraction
- [ ] **Create `include/gitmind/output.h`**
  - [ ] Define `gm_output_t` structure
  - [ ] Define output levels (SILENT, NORMAL, VERBOSE)
  - [ ] Define output formats (HUMAN, PORCELAIN)

- [ ] **Create `src/cli/output.c`**
  - [ ] Implement `gm_output_create()`
  - [ ] Implement `gm_output_print()` - respects verbose flag
  - [ ] Implement `gm_output_error()` - always shows errors
  - [ ] Implement `gm_output_json()` - porcelain mode
  - [ ] Implement `gm_output_destroy()`

### Update CLI Commands
- [ ] **Update `main.c`**
  - [ ] Parse global `--verbose` and `--porcelain` flags
  - [ ] Create output context
  - [ ] Pass output context to all commands

- [ ] **Update all CLI commands** to use output abstraction
  - [ ] `cache_rebuild.c` - replace all printf/fprintf
  - [ ] `install_hooks.c` - replace all printf/fprintf
  - [ ] `link.c` - replace all printf/fprintf
  - [ ] `list.c` - replace all printf/fprintf

## Priority 3: Define All Constants (Day 3)

### Create Constants Headers
- [ ] **Create `include/gitmind/constants_cbor.h`**
  - [ ] CBOR type constants (ARRAY, BYTES, TEXT, UINT)
  - [ ] CBOR size indicators
  - [ ] CBOR array sizes (13 for attributed, 8 for legacy)
  - [ ] CBOR field limits

- [ ] **Create `include/gitmind/constants_internal.h`**
  - [ ] Buffer sizes (PATH, REF, CWD, HOOK)
  - [ ] Time conversions (MILLIS_PER_SECOND, NANOS_PER_MILLI)
  - [ ] Git constants (EMPTY_TREE_SHA, REFS_PREFIX, HEAD_REF)
  - [ ] Exit codes (SAFETY_VIOLATION)
  - [ ] File permissions (HOOK_PERMS)

### Replace Magic Values
- [ ] **Edge module** - replace all magic numbers/strings
- [ ] **Journal module** - replace all magic numbers/strings
- [ ] **Attribution module** - replace all magic numbers/strings
- [ ] **CLI module** - replace all magic numbers/strings
- [ ] **Cache module** - replace all magic numbers/strings
- [ ] **Util module** - replace all magic numbers/strings
- [ ] **Hooks module** - replace all magic numbers/strings

## Priority 4: Dependency Injection (Day 4)

### Create Abstraction Interfaces
- [ ] **Create `include/gitmind/io_ops.h`**
  - [ ] Define file operation callbacks
  - [ ] Define directory operation callbacks
  - [ ] Define process operation callbacks

- [ ] **Create `include/gitmind/time_ops.h`**
  - [ ] Define time() callback
  - [ ] Define clock_gettime() callback

- [ ] **Create `include/gitmind/random_ops.h`**
  - [ ] Define rand() callback
  - [ ] Define seed callback

### Update Context Structure
- [ ] **Modify `gm_context_t`** to include:
  - [ ] `io_ops` pointer
  - [ ] `time_ops` pointer
  - [ ] `random_ops` pointer
  - [ ] `output` pointer

### Implement Default Providers
- [ ] **Create `src/util/io_default.c`**
- [ ] **Create `src/util/time_default.c`**
- [ ] **Create `src/util/random_default.c`**

### Update All Modules
- [ ] **Update edge module** to use injected providers
- [ ] **Update journal module** to use injected providers
- [ ] **Update cache module** to use injected providers
- [ ] **Update util module** to use injected providers

## Priority 5: Eliminate Code Duplication (Day 5)

### Merge Duplicate Functions
- [ ] **Journal module**
  - [ ] Merge `process_commit()` and `process_commit_attributed()`
  - [ ] Merge `walk_journal()` and `walk_journal_attributed()`
  - [ ] Merge `gm_journal_append()` and `gm_journal_append_attributed()`

- [ ] **Cache module**
  - [ ] Merge `gm_cache_query_fanout()` and `gm_cache_query_fanin()`
  - [ ] Extract common query logic

- [ ] **CBOR module**
  - [ ] Move duplicated functions from `cbor_decode_ex.c` to shared location
  - [ ] Create `src/cbor/common.c` for shared helpers

## Priority 6: Fix Test Structure (Day 6)

### Refactor Tests for Behavior
- [ ] **Update `test_behavior.sh`**
  - [ ] Remove all stdout/stderr grep checks
  - [ ] Use `--porcelain` mode for assertions
  - [ ] Check state changes, not output

- [ ] **Update `test_attribution.c`**
  - [ ] Remove all printf statements
  - [ ] Return proper exit codes
  - [ ] Test behavior not output

- [ ] **Update `test_augments.sh`**
  - [ ] Remove output string checks
  - [ ] Verify edge creation, not messages

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

## Priority 7: API Documentation (Day 7)

### Document All Public APIs
- [ ] **Edge module** - document all public functions
- [ ] **Journal module** - document all public functions
- [ ] **Attribution module** - document all public functions
- [ ] **Cache module** - document all public functions
- [ ] **Util module** - document all public functions

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
- [ ] All functions ≤ 15 lines
- [ ] All functions ≤ 3 levels of nesting
- [ ] Zero magic numbers/strings
- [ ] Complete CLI output control
- [ ] Full dependency injection
- [ ] No code duplication
- [ ] All tests check behavior
- [ ] All public APIs documented
- [ ] Project score ≥ 8/10

### Verification Checklist
- [ ] Run function length checker (no violations)
- [ ] Run magic value detector (no violations)
- [ ] Run output control checker (no direct printf)
- [ ] Run dependency injection checker (no direct system calls)
- [ ] Run test quality checker (no stdout assertions)
- [ ] Run documentation checker (all public APIs documented)

## Timeline

- **Week 1**: Emergency fixes (Priority 1-4)
- **Week 2**: Cleanup and polish (Priority 5-7)
- **Total**: 7-10 days to clear all technical debt

---
*Generated from audit findings in `.audits-c/2025-06-19-deep-dive/`*