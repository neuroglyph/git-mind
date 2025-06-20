# Single Responsibility Principle Refactoring Plan

## Overview
This audit identified 15 major SRP violations across the git-mind codebase. These violations contribute to high cyclomatic complexity and make the code difficult to test and maintain.

## Common Anti-Patterns Found

1. **Orchestrator Overload** - Functions trying to manage entire workflows
2. **Mixed Abstraction Levels** - Low-level I/O mixed with business logic  
3. **Embedded Infrastructure** - Error handling, logging mixed with core logic
4. **Dual Purpose Functions** - Functions that both decide AND execute
5. **State Mutation** - Functions that compute AND modify state

## Priority Refactoring Targets

### Priority 1: Core Infrastructure (Highest Impact)

#### 1. `cache_rebuild_internal()` - src/cache/builder.c
**Current**: 67-line monster doing everything
**Refactor into**:
- `prepare_cache_rebuild()` - Setup temp directory and context
- `collect_all_edges()` - Gather edges from journal  
- `build_edge_maps()` - Create forward/reverse mappings
- `persist_cache_data()` - Write bitmaps and trees
- `finalize_cache_commit()` - Create commit and update refs

#### 2. `journal_append_generic()` - src/journal/writer.c  
**Current**: Buffer management + encoding + batching
**Refactor into**:
- `prepare_journal_buffer()` - Allocate and manage buffer
- `encode_edges_batch()` - CBOR encoding only
- `should_commit_batch()` - Batch size decision logic
- `commit_journal_batch()` - Commit creation only

#### 3. `process_commit_generic()` - src/journal/reader.c
**Current**: Extract + decode + route + iterate
**Refactor into**:
- `extract_commit_cbor()` - Get CBOR data from commit
- `decode_next_edge()` - Decode single edge from buffer
- `route_edge_callback()` - Call appropriate callback

### Priority 2: CLI Layer (User-Facing)

#### 4. `safety_check()` - src/cli/main.c
**Current**: Path check + git config + error display + enforcement
**Refactor into**:
- `validate_working_directory()` - Path validation only
- `check_remote_safety()` - Remote URL validation
- `enforce_safety_policy()` - Decision logic only
- `report_safety_violation()` - Error formatting/display

#### 5. `execute_cache_rebuild()` - src/cli/cache_rebuild.c
**Current**: Timing + execution + stats + reporting
**Refactor into**:
- `measure_operation()` - Generic timing wrapper
- `gather_cache_statistics()` - Stats collection only
- Keep rebuild execution separate from reporting

### Priority 3: Data Processing

#### 6. `write_bitmaps_to_temp()` - src/cache/builder.c
**Current**: Processes two maps with duplicate logic
**Refactor into**:
- `write_bitmap_map()` - Generic map writer
- `ensure_shard_directory()` - Directory creation
- `write_single_bitmap()` - File writing only

#### 7. `cache_query_generic()` - src/cache/query.c
**Current**: Initialize + try cache + fallback
**Refactor into**:
- `query_from_cache()` - Cache query only
- `query_from_journal()` - Journal scan only
- `select_query_strategy()` - Decision logic

### Priority 4: Edge Operations

#### 8. `gm_edge_create()` - src/edge/edge.c
**Current**: Resolve SHAs + paths + metadata + ULID
**Refactor into**:
- `resolve_edge_shas()` - SHA resolution only
- `initialize_edge_metadata()` - Set timestamps, etc.
- `copy_edge_paths()` - Path management
- Keep ULID generation separate

#### 9. `validate_link_inputs()` - src/cli/link.c
**Current**: Parse + validate + retrieve attribution
**Refactor into**:
- `parse_link_type()` - Type parsing
- `validate_confidence()` - Confidence validation  
- `get_attribution_from_env()` - Environment retrieval

### Implementation Strategy

1. **Start with leaf functions** - Refactor bottom-up to avoid breaking changes
2. **Extract pure functions first** - Separate computation from I/O
3. **Use dependency injection** - Pass dependencies explicitly
4. **Create small, testable units** - Each function should be easily unit tested
5. **Document responsibility** - Each function should have a single clear purpose

### Measuring Success

- [ ] Average function CCN ≤ 5
- [ ] No function > 20 lines (except unavoidable switch statements)
- [ ] Each function has ONE clear responsibility
- [ ] Function names accurately describe their single purpose
- [ ] Unit tests can test each function in isolation

### Example Refactoring Pattern

```c
// BEFORE: Mixed responsibilities
int process_and_save_data(Context *ctx, Data *data) {
    // Validate
    if (!data->field) return ERROR;
    
    // Transform
    data->field = transform(data->field);
    
    // Save
    FILE *f = fopen(path, "w");
    fwrite(data, size, 1, f);
    fclose(f);
    
    // Report
    printf("Saved %d bytes\n", size);
    
    return OK;
}

// AFTER: Single responsibilities
int validate_data(const Data *data) {
    return data->field ? OK : ERROR;
}

Data transform_data(Data data) {
    data.field = transform(data.field);
    return data;
}

int save_data(const Data *data, const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) return ERROR;
    
    size_t written = fwrite(data, size, 1, f);
    fclose(f);
    
    return (written == 1) ? OK : ERROR;
}

void report_save(size_t size) {
    printf("Saved %zu bytes\n", size);
}
```

### Next Steps

1. Create feature branch `refactor/srp-violations`
2. Start with Priority 1 targets
3. Add unit tests for each extracted function
4. Update integration tests as needed
5. Document each function's single responsibility

---
*Generated from SRP audit performed on 2025-06-20*