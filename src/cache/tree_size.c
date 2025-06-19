/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <git2.h>
#include <stdint.h>
#include "../../include/gitmind.h"

/* Calculate size of a Git tree recursively */
static int calculate_tree_size_recursive(git_repository* repo, const git_oid* tree_oid,
                                       uint64_t* total_size) {
    git_tree* tree = NULL;
    size_t entry_count;
    int rc;
    
    /* Look up tree */
    rc = git_tree_lookup(&tree, repo, tree_oid);
    if (rc < 0) {
        return GM_ERROR;
    }
    
    /* Get tree object size */
    git_odb* odb = NULL;
    rc = git_repository_odb(&odb, repo);
    if (rc < 0) {
        git_tree_free(tree);
        return GM_ERROR;
    }
    
    size_t tree_size;
    git_object_t tree_type;
    rc = git_odb_read_header(&tree_size, &tree_type, odb, tree_oid);
    git_odb_free(odb);
    
    if (rc == 0) {
        *total_size += tree_size;
    }
    
    /* Process entries */
    entry_count = git_tree_entrycount(tree);
    for (size_t i = 0; i < entry_count; i++) {
        const git_tree_entry* entry = git_tree_entry_byindex(tree, i);
        if (!entry) continue;
        
        git_filemode_t mode = git_tree_entry_filemode(entry);
        const git_oid* entry_oid = git_tree_entry_id(entry);
        
        if (mode == GIT_FILEMODE_TREE) {
            /* Recurse into subdirectory */
            rc = calculate_tree_size_recursive(repo, entry_oid, total_size);
            if (rc != GM_OK) {
                git_tree_free(tree);
                return rc;
            }
        } else if (mode == GIT_FILEMODE_BLOB) {
            /* Add blob size */
            git_odb* odb2 = NULL;
            rc = git_repository_odb(&odb2, repo);
            if (rc == 0) {
                size_t blob_size;
                git_object_t blob_type;
                rc = git_odb_read_header(&blob_size, &blob_type, odb2, entry_oid);
                if (rc == 0) {
                    *total_size += blob_size;
                }
                git_odb_free(odb2);
            }
        }
    }
    
    git_tree_free(tree);
    return GM_OK;
}

/* Calculate total size of cache tree */
int gm_cache_calculate_size(git_repository* repo, const git_oid* tree_oid,
                           uint64_t* size_bytes) {
    *size_bytes = 0;
    return calculate_tree_size_recursive(repo, tree_oid, size_bytes);
}