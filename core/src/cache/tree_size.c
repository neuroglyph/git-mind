/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <stddef.h>
#include <stdint.h>

#include <git2/oid.h>
#include <git2/odb.h>
#include <git2/repository.h>
#include <git2/tree.h>
#include <git2/types.h>

#include "gitmind/cache.h"
#include "cache_internal.h"
#include "gitmind/error.h"

/* Calculate size of a Git tree recursively */
/* NOLINTNEXTLINE(misc-no-recursion,readability-function-cognitive-complexity) */
static int calculate_tree_size_recursive(git_repository *repo,
                                         const git_oid *tree_oid,
                                         uint64_t *total_size) {
    git_tree *tree = NULL;
    size_t entry_count;
    int ret;

    /* Look up tree */
    ret = git_tree_lookup(&tree, repo, tree_oid);
    if (ret < 0) {
        return GM_ERR_UNKNOWN;
    }

    /* Get tree object size */
    git_odb *odb = NULL;
    ret = git_repository_odb(&odb, repo);
    if (ret < 0) {
        git_tree_free(tree);
        return GM_ERR_UNKNOWN;
    }

    size_t tree_size;
    git_object_t tree_type;
    ret = git_odb_read_header(&tree_size, &tree_type, odb, tree_oid);
    git_odb_free(odb);

    if (ret == 0) {
        *total_size += tree_size;
    }

    /* Process entries */
    entry_count = git_tree_entrycount(tree);
    for (size_t i = 0; i < entry_count; i++) {
        const git_tree_entry *entry = git_tree_entry_byindex(tree, i);
        if (!entry) {
            continue;
        }

        git_filemode_t mode = git_tree_entry_filemode(entry);
        const git_oid *entry_oid = git_tree_entry_id(entry);

        if (mode == GIT_FILEMODE_TREE) {
            /* Recurse into subdirectory */
            ret = calculate_tree_size_recursive(repo, entry_oid, total_size);
            if (ret != GM_OK) {
                git_tree_free(tree);
                return ret;
            }
        } else if (mode == GIT_FILEMODE_BLOB) {
            /* Add blob size */
            git_odb *odb2 = NULL;
            ret = git_repository_odb(&odb2, repo);
            if (ret == 0) {
                size_t blob_size;
                git_object_t blob_type;
                ret = git_odb_read_header(&blob_size, &blob_type, odb2,
                                          entry_oid);
                if (ret == 0) {
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
int gm_cache_calculate_size(git_repository *repo, const git_oid *tree_oid,
                            uint64_t *size_bytes) {
    *size_bytes = 0;
    return calculate_tree_size_recursive(repo, tree_oid, size_bytes);
}
