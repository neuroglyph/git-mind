#!/bin/bash
# SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0
# Test --verbose and --porcelain output control

. tests/test_common.sh

# Test --porcelain output for link command
test_porcelain_link() {
    cd "$TEST_DIR"
    
    # Create files
    create_file "src/main.c" "int main() { return 0; }"
    create_file "src/config.h" "#define VERSION 1"
    
    # Create link with --porcelain
    output=$(./git-mind --porcelain link src/main.c src/config.h --type depends_on 2>&1)
    
    # Check porcelain output format
    if [[ ! "$output" =~ "status=created" ]]; then
        fail "Missing status in porcelain output"
    fi
    if [[ ! "$output" =~ "source=src/main.c" ]]; then
        fail "Missing source in porcelain output"
    fi
    if [[ ! "$output" =~ "target=src/config.h" ]]; then
        fail "Missing target in porcelain output"
    fi
    if [[ ! "$output" =~ "type=3" ]]; then  # depends_on = 3
        fail "Missing type in porcelain output"
    fi
    if [[ ! "$output" =~ "confidence=1.000" ]]; then
        fail "Missing confidence in porcelain output"
    fi
    if [[ ! "$output" =~ "ulid=" ]]; then
        fail "Missing ulid in porcelain output"
    fi
    
    # Should not contain human-readable format
    if [[ "$output" =~ "Created link:" ]]; then
        fail "Porcelain output contains human-readable text"
    fi
    
    pass
}

# Test --verbose output
test_verbose_cache_rebuild() {
    cd "$TEST_DIR"
    
    # Create some links first
    create_file "a.txt" "content a"
    create_file "b.txt" "content b"
    ./git-mind link a.txt b.txt >/dev/null 2>&1
    
    # Run cache rebuild without --verbose (should be quiet)
    output=$(./git-mind cache-rebuild 2>&1)
    if [[ "$output" =~ "Rebuilding cache" ]]; then
        fail "Non-verbose mode shows verbose messages"
    fi
    
    # Run cache rebuild with --verbose
    output=$(./git-mind --verbose cache-rebuild 2>&1)
    if [[ ! "$output" =~ "Rebuilding cache" ]]; then
        fail "Verbose mode doesn't show verbose messages"
    fi
    
    pass
}

# Test --porcelain for cache-rebuild
test_porcelain_cache_rebuild() {
    cd "$TEST_DIR"
    
    # Create a link
    create_file "x.txt" "x"
    create_file "y.txt" "y"
    ./git-mind link x.txt y.txt >/dev/null 2>&1
    
    # Rebuild cache with --porcelain
    output=$(./git-mind --porcelain cache-rebuild 2>&1)
    
    # Check porcelain output
    if [[ ! "$output" =~ "status=success" ]]; then
        fail "Missing status in porcelain cache output"
    fi
    if [[ ! "$output" =~ "edges=" ]]; then
        fail "Missing edges count in porcelain output"
    fi
    if [[ ! "$output" =~ "cache_size_kb=" ]]; then
        fail "Missing cache size in porcelain output"
    fi
    if [[ ! "$output" =~ "build_time_seconds=" ]]; then
        fail "Missing build time in porcelain output"
    fi
    
    # Should not contain human-readable text
    if [[ "$output" =~ "Cache rebuilt successfully" ]]; then
        fail "Porcelain output contains human-readable text"
    fi
    
    pass
}

# Test --porcelain for install-hooks
test_porcelain_install_hooks() {
    cd "$TEST_DIR"
    
    # Install hooks with --porcelain
    output=$(./git-mind --porcelain install-hooks 2>&1)
    
    # Check porcelain output
    if [[ ! "$output" =~ "status=installed" ]]; then
        fail "Missing status in porcelain hooks output"
    fi
    if [[ ! "$output" =~ "hook=post-commit" ]]; then
        fail "Missing hook type in porcelain output"
    fi
    
    # Run again - should show already installed
    output=$(./git-mind --porcelain install-hooks 2>&1)
    if [[ ! "$output" =~ "status=already-installed" ]]; then
        fail "Wrong status for already installed hooks"
    fi
    
    pass
}

# Run tests
test_porcelain_link
test_verbose_cache_rebuild
test_porcelain_cache_rebuild
test_porcelain_install_hooks