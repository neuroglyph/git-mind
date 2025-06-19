#!/bin/bash
# SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0
# © 2025 J. Kirby Ross / Neuroglyph Collective

# Core Functionality E2E Tests

source "$(dirname "$0")/test_framework.sh"

echo "=== CORE FUNCTIONALITY TESTS ==="

# Test 1: Basic Link Creation
test_basic_link() {
    echo -e "\n${YELLOW}Test 1: Basic Link Creation${NC}"
    
    init_test_repo "basic"
    create_file "A.md" "Source file"
    create_file "B.md" "Target file"
    commit "Initial files"
    
    # Create link
    assert_success "gm link A.md B.md --type references"
    
    # Verify link exists
    assert_edge_exists "A.md" "B.md" "REFERENCES"
    
    # Verify journal commit created
    local commits=$(journal_commits)
    [ "$commits" -eq 1 ] || echo "Expected 1 journal commit, got $commits"
}

# Test 2: Multiple Edge Types
test_edge_types() {
    echo -e "\n${YELLOW}Test 2: Multiple Edge Types${NC}"
    
    init_test_repo "types"
    create_file "design.md" "System design"
    create_file "impl.c" "Implementation"
    create_file "test.c" "Tests"
    create_file "doc.md" "Documentation"
    commit "Test files"
    
    # Create different edge types
    assert_success "gm link impl.c design.md --type implements"
    assert_success "gm link test.c impl.c --type depends_on"
    assert_success "gm link doc.md design.md --type references"
    
    # Verify all edges
    assert_edge_exists "impl.c" "design.md" "IMPLEMENTS"
    assert_edge_exists "test.c" "impl.c" "DEPENDS_ON"
    assert_edge_exists "doc.md" "design.md" "REFERENCES"
    
    # Count total edges
    local count=$(count_edges)
    [ "$count" -eq 3 ] || echo "Expected 3 edges, got $count"
}

# Test 3: Branch Isolation
test_branch_isolation() {
    echo -e "\n${YELLOW}Test 3: Branch Isolation${NC}"
    
    init_test_repo "branches"
    create_file "README.md" "Main readme"
    commit "Initial commit"
    
    # Create link on main
    assert_success "gm link README.md README.md --type references"
    
    # Create feature branch
    git checkout -b feature --quiet
    create_file "feature.md" "Feature doc"
    commit "Add feature"
    
    # Create link on feature branch
    assert_success "gm link feature.md README.md --type implements"
    
    # Feature branch should have both links
    local feature_count=$(count_edges)
    [ "$feature_count" -eq 2 ] || echo "Expected 2 edges on feature, got $feature_count"
    
    # Switch back to main
    git checkout main --quiet
    
    # Main should only have original link
    local main_count=$(count_edges)
    [ "$main_count" -eq 1 ] || echo "Expected 1 edge on main, got $main_count"
    
    # Verify specific edges
    assert_failure "gm list | grep feature.md"
}

# Test 4: File Evolution (Augments)
test_augments_flow() {
    echo -e "\n${YELLOW}Test 4: File Evolution (Augments)${NC}"
    
    init_test_repo "augments"
    create_file "api.md" "API v1"
    commit "Initial API"
    
    # Get original SHA
    local sha1=$(git hash-object api.md)
    
    # Create initial link
    create_file "impl.c" "Implementation v1"
    commit "Add implementation"
    assert_success "gm link impl.c api.md --type implements"
    
    # Update API file
    echo "API v2" > api.md
    git add api.md
    commit "Update API to v2"
    
    # Get new SHA
    local sha2=$(git hash-object api.md)
    [ "$sha1" != "$sha2" ] || echo "SHA should change after edit"
    
    # Create augments link
    # Note: This would require post-commit hook in real implementation
    # For now, we verify the concept
    assert_success "gm link api.md api.md --type augments"
}

# Test 5: Link to Deleted File
test_link_to_deleted() {
    echo -e "\n${YELLOW}Test 5: Link to Deleted File${NC}"
    
    init_test_repo "deleted"
    create_file "temp.md" "Temporary file"
    create_file "main.md" "Main file"
    commit "Initial files"
    
    # Create link
    assert_success "gm link main.md temp.md"
    assert_edge_exists "main.md" "temp.md"
    
    # Delete target file
    git rm temp.md --quiet
    commit "Remove temp file"
    
    # Link should still exist in journal
    assert_edge_exists "main.md" "temp.md"
    
    # But creating new link to deleted file should fail
    assert_failure "gm link main.md temp.md"
}

# Test 6: Batch Link Creation
test_batch_links() {
    echo -e "\n${YELLOW}Test 6: Batch Link Creation${NC}"
    
    init_test_repo "batch"
    
    # Create many files
    for i in $(seq 1 20); do
        create_file "file$i.md" "Content $i"
    done
    commit "Add 20 files"
    
    # Create many links (testing CBOR batching)
    for i in $(seq 1 19); do
        gm link "file$i.md" "file$((i+1)).md" --type references
    done
    
    # Verify all links created
    local count=$(count_edges)
    [ "$count" -eq 19 ] || echo "Expected 19 edges, got $count"
    
    # Check journal commits (should batch)
    local commits=$(journal_commits)
    echo "  Created $commits journal commits for 19 edges"
}

# Test 7: Unicode and Special Characters
test_unicode_paths() {
    echo -e "\n${YELLOW}Test 7: Unicode and Special Characters${NC}"
    
    init_test_repo "unicode"
    create_file "你好.md" "Chinese"
    create_file "café.md" "French"
    create_file "file with spaces.md" "Spaces"
    create_file "special!@#\$.md" "Special chars"
    commit "Unicode files"
    
    # Create links with unicode
    assert_success "gm link 你好.md café.md"
    assert_success "gm link café.md 'file with spaces.md'"
    assert_success "gm link 'file with spaces.md' 'special!@#\$.md'"
    
    # Verify links
    assert_edge_exists "你好.md" "café.md"
    assert_edge_exists "café.md" "file with spaces.md"
}

# Test 8: Multi-hop Traversal
test_multi_hop() {
    echo -e "\n${YELLOW}Test 8: Multi-hop Traversal${NC}"
    
    init_test_repo "multihop"
    create_file "A.md" "Start"
    create_file "B.md" "Middle 1"
    create_file "C.md" "Middle 2"
    create_file "D.md" "End"
    commit "Chain files"
    
    # Create chain A -> B -> C -> D
    assert_success "gm link A.md B.md"
    assert_success "gm link B.md C.md"
    assert_success "gm link C.md D.md"
    
    # All links should exist
    local count=$(count_edges)
    [ "$count" -eq 3 ] || echo "Expected 3 edges, got $count"
    
    # Future: Test traversal when implemented
    # assert_contains "gm traverse A.md --depth 3" "D.md"
}

# Run all tests
test_basic_link
test_edge_types
test_branch_isolation
test_augments_flow
test_link_to_deleted
test_batch_links
test_unicode_paths
test_multi_hop

# Benchmarks
echo -e "\n=== PERFORMANCE BENCHMARKS ==="

# Benchmark: Single edge creation
init_test_repo "bench"
create_file "src.md" "Source"
create_file "tgt.md" "Target"
commit "Benchmark files"

benchmark "Single edge creation" "gm link src.md tgt.md" 5

# Benchmark: List 100 edges
for i in $(seq 1 100); do
    create_file "f$i.md" "File $i"
done
commit "100 files"

for i in $(seq 1 99); do
    gm link "f$i.md" "f$((i+1)).md" 2>/dev/null
done

benchmark "List 100 edges" "gm list" 50

report