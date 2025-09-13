#!/bin/bash
# SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0
# © 2025 J. Kirby Ross / Neuroglyph Collective

# Freaky Edge Cases E2E Tests

source "$(dirname "$0")/test_framework.sh"

echo "=== FREAKY EDGE CASES ==="

# Test 0: Helpful errors for invalid paths
test_invalid_path_errors() {
    echo -e "\n${YELLOW}Test 0: Helpful Errors for Invalid Paths${NC}"

    init_test_repo "invalid-paths"

    # No files yet; linking should fail with clear message for source
    assert_failure "gm link missing.md other.md"
    assert_contains "gm link missing.md other.md" "Error: Path not found: missing.md"

    # Create source, leave target missing
    create_file "exists.md" "content"
    commit "Add exists.md"
    assert_failure "gm link exists.md other.md"
    assert_contains "gm link exists.md other.md" "Error: Path not found: other.md"

    # Directory instead of regular file should fail
    mkdir -p dir
    assert_failure "gm link exists.md dir"
    assert_contains "gm link exists.md dir" "Error: Not a regular file: dir"
}

# Test 1: Same Content, Different Paths
test_same_content_different_paths() {
    echo -e "\n${YELLOW}Test 1: Same Content, Different Paths${NC}"
    
    init_test_repo "same-content"
    
    # Create two files with identical content
    create_file "README.md" "This is the documentation"
    create_file "docs/intro.md" "This is the documentation"
    commit "Identical files"
    
    # Get SHAs (should be same)
    local sha1=$(git hash-object README.md)
    local sha2=$(git hash-object docs/intro.md)
    [ "$sha1" = "$sha2" ] || echo "Expected same SHA for identical content"
    
    # Create links to both
    create_file "index.md" "Index"
    commit "Add index"
    
    assert_success "gm link index.md README.md"
    assert_success "gm link index.md docs/intro.md"
    
    # Should have 2 distinct edges despite same blob SHA
    local count=$(count_edges)
    [ "$count" -eq 2 ] || echo "Expected 2 edges, got $count"
    
    # Both edges should be visible
    assert_edge_exists "index.md" "README.md"
    assert_edge_exists "index.md" "docs/intro.md"
}

# Test 2: Same Path, Different Content
test_same_path_different_content() {
    echo -e "\n${YELLOW}Test 2: Same Path, Different Content${NC}"
    
    init_test_repo "same-path"
    
    # Create file and link
    create_file "config.yml" "version: 1"
    create_file "app.js" "console.log('v1')"
    commit "Version 1"
    
    assert_success "gm link app.js config.yml --type depends_on"
    
    # Get original SHA
    local sha1=$(git hash-object config.yml)
    
    # Modify file (same path, new content)
    echo "version: 2" > config.yml
    git add config.yml
    commit "Version 2"
    
    # Get new SHA
    local sha2=$(git hash-object config.yml)
    [ "$sha1" != "$sha2" ] || echo "SHA should change after edit"
    
    # Original link still points to old SHA
    # This is the philosophical question: Do we want this behavior?
    assert_edge_exists "app.js" "config.yml" "DEPENDS_ON"
    
    # Create new link - should use new SHA
    create_file "app2.js" "console.log('v2')"
    commit "Add app v2"
    assert_success "gm link app2.js config.yml"
    
    # Now we have 2 edges, potentially to different SHA versions
    local count=$(count_edges)
    [ "$count" -eq 2 ] || echo "Expected 2 edges, got $count"
}

# Test 3: Rebase Scenarios
test_rebase_scenarios() {
    echo -e "\n${YELLOW}Test 3: Rebase Scenarios${NC}"
    
    init_test_repo "rebase"
    
    # Create base commit
    create_file "base.md" "Base"
    commit "Base commit"
    
    # Create feature branch with links
    git checkout -b feature --quiet
    create_file "feature.md" "Feature"
    commit "Add feature"
    assert_success "gm link feature.md base.md"
    
    # Make change on main
    git checkout main --quiet
    echo "Updated base" > base.md
    git add base.md
    commit "Update base"
    
    # Rebase feature
    git checkout feature --quiet
    git rebase main --quiet
    
    # Link should still exist after rebase
    assert_edge_exists "feature.md" "base.md"
    
    # Journal commits should be re-parented
    local commits=$(journal_commits)
    echo "  Journal commits after rebase: $commits"
}

# Test 4: Concurrent Link Creation
test_concurrent_links() {
    echo -e "\n${YELLOW}Test 4: Concurrent Link Creation${NC}"
    
    # Simulate with two worktrees
    init_test_repo "concurrent"
    create_file "shared.md" "Shared file"
    commit "Initial"
    
    # Create worktree
    git worktree add ../concurrent-wt --quiet
    
    # Create link in main worktree
    assert_success "gm link shared.md shared.md --type references"
    
    # Create link in other worktree
    cd ../concurrent-wt
    create_file "other.md" "Other file"
    git add other.md
    git commit -m "Add other" --quiet
    assert_success "$GIT_MIND link other.md shared.md"
    
    # Push both (would need remote in real test)
    # git push origin main refs/gitmind/edges/main
    
    # Both links should exist
    local count=$("$GIT_MIND" list | grep -E '^[A-Z_]+:' | wc -l)
    echo "  Total edges across worktrees: $count"
    
    # Cleanup worktree
    cd ../concurrent
    git worktree remove ../concurrent-wt --force
}

# Test 5: Corrupted CBOR in Journal
test_corrupted_cbor() {
    echo -e "\n${YELLOW}Test 5: Corrupted CBOR Journal${NC}"
    
    init_test_repo "corrupt"
    create_file "a.md" "File A"
    create_file "b.md" "File B"
    commit "Files"
    
    # Create valid link
    assert_success "gm link a.md b.md"
    
    # Manually create corrupted journal commit
    # This simulates corruption or manual tampering
    local empty_tree="4b825dc642cb6eb9a060e54bf8d69288fbee4904"
    local bad_cbor="CORRUPTED GARBAGE DATA NOT CBOR"
    
    # Get current journal head
    local ref="refs/gitmind/edges/main"
    local parent=$(git rev-parse "$ref" 2>/dev/null || echo "")
    
    if [ -n "$parent" ]; then
        # Create commit with bad data
        local commit=$(echo "$bad_cbor" | git commit-tree "$empty_tree" -p "$parent")
        git update-ref "$ref" "$commit"
    fi
    
    # List should handle corruption gracefully
    gm list > /dev/null 2>&1
    local exit_code=$?
    echo "  List with corruption exit code: $exit_code"
    
    # Should still show the valid edge
    assert_edge_exists "a.md" "b.md"
}

# Test 6: Case Sensitivity
test_case_sensitivity() {
    echo -e "\n${YELLOW}Test 6: Filesystem Case Sensitivity${NC}"
    
    init_test_repo "case"
    
    # Test depends on filesystem
    create_file "readme.md" "Lower"
    
    # Try to create conflicting case
    if touch README.md 2>/dev/null && [ -f readme.md ] && [ -f README.md ]; then
        echo "  Filesystem is case-sensitive"
        echo "UPPER" > README.md
        git add README.md
        commit "Both cases"
        
        # Both files should be linkable
        assert_success "gm link readme.md README.md"
        assert_edge_exists "readme.md" "README.md"
    else
        echo "  Filesystem is case-insensitive"
        # Can't test conflicting cases
        commit "Single case"
    fi
}

# Test 7: Massive Fanout
test_massive_fanout() {
    echo -e "\n${YELLOW}Test 7: Massive Fanout (Stress Test)${NC}"
    
    init_test_repo "fanout"
    create_file "hub.md" "Central hub"
    commit "Add hub"
    
    # Create 100 files linking to hub
    echo "  Creating 100 edges..."
    for i in $(seq 1 100); do
        create_file "node$i.md" "Node $i"
    done
    commit "Add 100 nodes"
    
    # Link all to hub
    local start=$(date +%s%N)
    for i in $(seq 1 100); do
        gm link "node$i.md" hub.md 2>/dev/null
    done
    local end=$(date +%s%N)
    
    local elapsed=$(( (end - start) / 1000000 ))
    echo "  Time to create 100 edges: ${elapsed}ms"
    
    # Measure list performance
    benchmark "List 100-edge fanout" "gm list" 100
    
    # Measure memory
    if command -v /usr/bin/time >/dev/null 2>&1; then
        echo "  Memory usage for listing:"
        measure_memory "gm list" 50
    fi
}

# Test 8: Path Length Limits
test_path_limits() {
    echo -e "\n${YELLOW}Test 8: Path Length Limits${NC}"
    
    init_test_repo "paths"
    
    # Create deeply nested path (near 255 char limit)
    local deep="very/deep/nested/directory/structure/that/goes/on/and/on"
    deep="$deep/and/continues/further/down/the/tree/until/we/reach"
    deep="$deep/the/maximum/path/length/allowed/by/the/filesystem"
    
    create_file "$deep/file.md" "Deep file"
    create_file "root.md" "Root"
    commit "Deep paths"
    
    # Try to link (may fail on path length)
    if gm link root.md "$deep/file.md" 2>/dev/null; then
        echo "  Successfully linked deep path"
        assert_edge_exists "root.md" "$deep/file.md"
    else
        echo "  Path too long for filesystem"
    fi
    
    # Test path truncation in display
    local long_name="this_is_a_very_long_filename_that_might_get_truncated_in_display_"
    long_name="${long_name}because_it_exceeds_reasonable_terminal_width.md"
    
    create_file "$long_name" "Long name"
    commit "Long filename"
    
    assert_success "gm link root.md '$long_name'"
}

# Test 9: Submodules and Symlinks
test_special_git_objects() {
    echo -e "\n${YELLOW}Test 9: Submodules and Symlinks${NC}"
    
    init_test_repo "special"
    
    # Test symlinks
    create_file "target.md" "Link target"
    ln -s target.md link.md
    git add link.md
    commit "Add symlink"
    
    # Link to symlink
    create_file "doc.md" "Documentation"
    commit "Add doc"
    
    # This links to the symlink blob (contains "target.md")
    assert_success "gm link doc.md link.md"
    assert_edge_exists "doc.md" "link.md"
    
    # Test submodules (if we can)
    if git submodule add https://github.com/octocat/Hello-World.git hello 2>/dev/null; then
        commit "Add submodule"
        
        # Submodule is a commit object, not blob
        # Should this work or fail?
        if gm link doc.md hello 2>/dev/null; then
            echo "  Submodule link created (commit SHA)"
        else
            echo "  Submodule link rejected (not a blob)"
        fi
        
        # Cleanup
        git submodule deinit -f hello 2>/dev/null
        git rm -f hello 2>/dev/null
        rm -rf .git/modules/hello
    else
        echo "  Skipping submodule test (no network?)"
    fi
}

# Test 10: Journal Merge Conflicts
test_journal_merge() {
    echo -e "\n${YELLOW}Test 10: Journal Merge Conflicts${NC}"
    
    init_test_repo "merge"
    create_file "base.md" "Base file"
    commit "Initial"
    
    # Branch 1
    git checkout -b branch1 --quiet
    create_file "a.md" "File A"
    commit "Add A"
    assert_success "gm link a.md base.md"
    
    # Branch 2
    git checkout main --quiet
    git checkout -b branch2 --quiet
    create_file "b.md" "File B"
    commit "Add B"
    assert_success "gm link b.md base.md"
    
    # Merge branches
    git checkout main --quiet
    git merge branch1 --quiet --no-edit
    git merge branch2 --quiet --no-edit
    
    # Both links should exist after merge
    assert_edge_exists "a.md" "base.md"
    assert_edge_exists "b.md" "base.md"
    
    # Journal refs should have merged
    local commits=$(journal_commits)
    echo "  Journal commits after merge: $commits"
}

# Run all edge case tests
test_invalid_path_errors
test_same_content_different_paths
test_same_path_different_content
test_rebase_scenarios
test_concurrent_links
test_corrupted_cbor
test_case_sensitivity
test_massive_fanout
test_path_limits
test_special_git_objects
test_journal_merge

report
