# git-mind End-to-End Test Suite

## Overview

This test suite verifies git-mind behavior from a user's perspective. Tests focus on WHAT the system does, not HOW it does it.

## Test Categories

### 1. Core Functionality (`01_core_functionality.sh`)

- Basic link creation
- Multiple edge types  
- Branch isolation
- File evolution (augments)
- Deleted file handling
- Batch operations
- Unicode support
- Multi-hop traversal

### 2. Edge Cases (`02_edge_cases.sh`)

- Same content, different paths
- Same path, different content
- Rebase scenarios
- Concurrent modifications
- Corrupted journal data
- Case sensitivity
- Massive fanout stress test
- Path length limits
- Symlinks and submodules
- Journal merge conflicts

### 3. Performance Benchmarks (`03_performance_benchmarks.sh`)

- Single edge creation: <5ms
- 10k fanout query: <50ms
- 50-version chain: <10ms
- 2k edge merge: <200ms
- Cold start query: <100ms
- Memory usage: <100MB
- CBOR efficiency: <256 bytes/edge
- Concurrent operations

## Running Tests

### Run All Tests

```bash
cd tests/e2e
./run_all_tests.sh
```

### Run Individual Suite

```bash
./01_core_functionality.sh
./02_edge_cases.sh
./03_performance_benchmarks.sh
```

### Run Specific Test

```bash
# Source framework and run single test
source test_framework.sh
init_test_repo "mytest"
# ... your test code
```

## Test Framework

The `test_framework.sh` provides:

- Test repo creation
- Assertion helpers
- Benchmark utilities
- Memory measurement
- Pretty output with colors

### Key Functions

- `init_test_repo` - Create isolated test repository
- `create_file` - Create and stage file
- `assert_success` - Verify command succeeds
- `assert_failure` - Verify command fails
- `assert_contains` - Check output contains string
- `assert_edge_exists` - Verify edge in output
- `benchmark` - Time a command
- `measure_memory` - Check memory usage

## Design Principles

1. __Test Behavior, Not Implementation__
   - Don't test stdout format details
   - Don't rely on internal structures
   - Focus on user-visible effects

2. __Isolated Tests__
   - Each test runs in fresh repo
   - No dependency between tests
   - Automatic cleanup

3. __Fast Execution__
   - Entire suite runs in <30 seconds
   - Individual tests <1 second
   - Benchmarks have time targets

4. __Clear Failures__
   - Show what failed and why
   - Expected vs actual output
   - Exit codes indicate success

## Adding New Tests

1. Create test function:

```bash
test_my_feature() {
    echo -e "\n${YELLOW}Test: My Feature${NC}"
    
    init_test_repo "myfeature"
    create_file "test.md" "content"
    commit "Add test file"
    
    # Test behavior
    assert_success "gm link test.md test.md"
    assert_edge_exists "test.md" "test.md"
}
```

2. Add to appropriate test file
3. Call function in script
4. Run and verify

## Performance Targets

These targets ensure git-mind remains fast:

| Operation | Target | Why |
|-----------|--------|-----|
| Single edge | <5ms | Interactive use |
| 10k fanout | <50ms | Large projects |
| Cold query | <100ms | First use experience |
| Memory | <100MB | Embedded systems |

## Known Limitations

- Push/pull tests require actual remote
- Concurrent tests are probabilistic
- Some filesystem-specific tests may skip
- Memory measurement requires GNU time

## Future Tests

- Web UI integration
- Cache layer performance
- Graph traversal algorithms
- Distributed scenarios
- Large repository scale (Linux kernel size)
