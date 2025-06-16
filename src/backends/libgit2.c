/* SPDX-License-Identifier: Apache-2.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#define _GNU_SOURCE  /* For strdup */
#include "gitmind_lib.h"
#include "gitmind_internal.h"
#include <git2.h>
#include <string.h>
#include <stdlib.h>

/* Track libgit2 initialization */
static int libgit2_init_count = 0;

/* Initialize libgit2 if needed */
static int ensure_libgit2_init(void) {
    if (libgit2_init_count == 0) {
        int ret = git_libgit2_init();
        if (ret < 0) {
            return GM_ERR_GIT;
        }
        libgit2_init_count = ret;
    }
    return GM_OK;
}

/* Backend implementation */
static int libgit2_open_repo(void* backend_data, const char* path, void** out_handle) {
    /* backend_data will be the context when called from context.c */
    (void)backend_data;
    
    int ret = ensure_libgit2_init();
    if (ret != GM_OK) return ret;
    
    git_repository* repo = NULL;
    if (git_repository_open(&repo, path) < 0) {
        return GM_ERR_NOT_REPO;
    }
    
    *out_handle = repo;
    return GM_OK;
}

static void libgit2_close_repo(void* backend_data, void* handle) {
    /* backend_data will be the context when called from context.c */
    (void)backend_data;
    if (handle) {
        git_repository_free((git_repository*)handle);
    }
}

static int libgit2_hash_object(void* backend_data, const void* data, size_t len,
                              const char* type, char* out_sha) {
    gm_context_t* ctx = (gm_context_t*)backend_data;
    if (!ctx || !ctx->repo_handle) return GM_ERR_INVALID_ARG;
    
    git_repository* repo = (git_repository*)ctx->repo_handle;
    git_odb* odb = NULL;
    git_oid oid;
    git_otype otype = GIT_OBJECT_BLOB;
    int ret = GM_OK;
    
    if (strcmp(type, "tree") == 0) {
        otype = GIT_OBJECT_TREE;
    } else if (strcmp(type, "commit") == 0) {
        otype = GIT_OBJECT_COMMIT;
    }
    
    if (git_repository_odb(&odb, repo) < 0) {
        return GM_ERR_GIT;
    }
    
    /* Write the object to the database */
    if (git_odb_write(&oid, odb, data, len, otype) < 0) {
        ret = GM_ERR_GIT;
        goto cleanup;
    }
    
    git_oid_tostr(out_sha, GIT_OID_HEXSZ + 1, &oid);
    
cleanup:
    if (odb) git_odb_free(odb);
    return ret;
}

static int libgit2_read_object(void* backend_data, const char* sha, void* out_data,
                              size_t max_size, size_t* actual_size) {
    gm_context_t* ctx = (gm_context_t*)backend_data;
    if (!ctx || !ctx->repo_handle) return GM_ERR_INVALID_ARG;
    
    git_repository* repo = (git_repository*)ctx->repo_handle;
    git_oid oid;
    git_odb* odb = NULL;
    git_odb_object* obj = NULL;
    int ret = GM_OK;
    
    if (git_oid_fromstr(&oid, sha) < 0) {
        return GM_ERR_INVALID_ARG;
    }
    
    if (git_repository_odb(&odb, repo) < 0) {
        return GM_ERR_GIT;
    }
    
    if (git_odb_read(&obj, odb, &oid) < 0) {
        ret = GM_ERR_NOT_FOUND;
        goto cleanup;
    }
    
    size_t obj_size = git_odb_object_size(obj);
    if (obj_size > max_size) {
        ret = GM_ERR_IO;
        goto cleanup;
    }
    
    memcpy(out_data, git_odb_object_data(obj), obj_size);
    if (actual_size) *actual_size = obj_size;
    
cleanup:
    if (obj) git_odb_object_free(obj);
    if (odb) git_odb_free(odb);
    return ret;
}

static int libgit2_read_tree(void* backend_data, const char* tree_sha, char* out_entries) {
    gm_context_t* ctx = (gm_context_t*)backend_data;
    if (!ctx || !ctx->repo_handle) return GM_ERR_INVALID_ARG;
    
    git_repository* repo = (git_repository*)ctx->repo_handle;
    git_oid oid;
    git_tree* tree = NULL;
    int ret = GM_OK;
    char* ptr = out_entries;
    
    if (git_oid_fromstr(&oid, tree_sha) < 0) {
        return GM_ERR_INVALID_ARG;
    }
    
    if (git_tree_lookup(&tree, repo, &oid) < 0) {
        return GM_ERR_NOT_FOUND;
    }
    
    size_t count = git_tree_entrycount(tree);
    for (size_t i = 0; i < count; i++) {
        const git_tree_entry* entry = git_tree_entry_byindex(tree, i);
        const char* name = git_tree_entry_name(entry);
        const git_oid* entry_oid = git_tree_entry_id(entry);
        git_filemode_t mode = git_tree_entry_filemode(entry);
        char oid_str[GIT_OID_HEXSZ + 1];
        
        git_oid_tostr(oid_str, sizeof(oid_str), entry_oid);
        
        /* Format: "mode type sha\tname\n" */
        const char* type_str = (mode == GIT_FILEMODE_TREE) ? "tree" : "blob";
        ptr += sprintf(ptr, "%06o %s %s\t%s\n", mode, type_str, oid_str, name);
    }
    
    git_tree_free(tree);
    return ret;
}

static int libgit2_write_tree(void* backend_data, const char* entries, char* out_sha) {
    gm_context_t* ctx = (gm_context_t*)backend_data;
    if (!ctx || !ctx->repo_handle) return GM_ERR_INVALID_ARG;
    
    git_repository* repo = (git_repository*)ctx->repo_handle;
    git_treebuilder* builder = NULL;
    git_oid tree_oid;
    int ret = GM_OK;
    
    if (git_treebuilder_new(&builder, repo, NULL) < 0) {
        return GM_ERR_GIT;
    }
    
    /* Parse entries format: "mode type sha\tname\n" */
    char* entries_copy = NULL;
    if (entries && *entries) {
        entries_copy = strdup(entries);
        if (!entries_copy) {
            ret = GM_ERR_MEMORY;
            goto cleanup;
        }
        
        char* line = strtok(entries_copy, "\n");
        while (line) {
            char mode_str[7], type[10], sha[GIT_OID_HEXSZ + 1], name[256];
            if (sscanf(line, "%6s %9s %40s\t%255s", mode_str, type, sha, name) == 4) {
                git_oid entry_oid;
                if (git_oid_fromstr(&entry_oid, sha) == 0) {
                    git_filemode_t mode = (git_filemode_t)strtol(mode_str, NULL, 8);
                    if (git_treebuilder_insert(NULL, builder, name, &entry_oid, mode) < 0) {
                        ret = GM_ERR_GIT;
                        goto cleanup;
                    }
                }
            }
            line = strtok(NULL, "\n");
        }
    }
    /* If entries is NULL or empty, we'll write an empty tree */
    
    if (git_treebuilder_write(&tree_oid, builder) < 0) {
        ret = GM_ERR_GIT;
        goto cleanup;
    }
    
    git_oid_tostr(out_sha, GIT_OID_HEXSZ + 1, &tree_oid);
    
cleanup:
    if (entries_copy) free(entries_copy);
    if (builder) git_treebuilder_free(builder);
    return ret;
}

static int libgit2_read_ref(void* backend_data, const char* ref_name, char* out_sha) {
    gm_context_t* ctx = (gm_context_t*)backend_data;
    if (!ctx || !ctx->repo_handle) return GM_ERR_INVALID_ARG;
    
    git_repository* repo = (git_repository*)ctx->repo_handle;
    git_reference* ref = NULL;
    const git_oid* oid = NULL;
    int ret = GM_OK;
    
    if (git_reference_lookup(&ref, repo, ref_name) < 0) {
        return GM_ERR_NOT_FOUND;
    }
    
    oid = git_reference_target(ref);
    if (!oid) {
        /* Symbolic ref - resolve it */
        git_reference* resolved = NULL;
        if (git_reference_resolve(&resolved, ref) < 0) {
            ret = GM_ERR_NOT_FOUND;
            goto cleanup;
        }
        oid = git_reference_target(resolved);
        git_reference_free(resolved);
    }
    
    if (oid) {
        git_oid_tostr(out_sha, GIT_OID_HEXSZ + 1, oid);
    } else {
        ret = GM_ERR_NOT_FOUND;
    }
    
cleanup:
    if (ref) git_reference_free(ref);
    return ret;
}

static int libgit2_update_ref(void* backend_data, const char* ref_name,
                             const char* new_sha, const char* message) {
    gm_context_t* ctx = (gm_context_t*)backend_data;
    if (!ctx || !ctx->repo_handle) return GM_ERR_INVALID_ARG;
    
    git_repository* repo = (git_repository*)ctx->repo_handle;
    git_oid oid;
    git_reference* ref = NULL;
    int ret = GM_OK;
    
    if (git_oid_fromstr(&oid, new_sha) < 0) {
        return GM_ERR_INVALID_ARG;
    }
    
    /* Create or update the reference */
    if (git_reference_create(&ref, repo, ref_name, &oid, 1, message) < 0) {
        ret = GM_ERR_GIT;
    }
    
    if (ref) git_reference_free(ref);
    return ret;
}


static int libgit2_write_note(void* backend_data, const char* notes_ref,
                             const char* object_sha, const char* note_content) {
    gm_context_t* ctx = (gm_context_t*)backend_data;
    if (!ctx || !ctx->repo_handle) return GM_ERR_INVALID_ARG;
    
    git_repository* repo = (git_repository*)ctx->repo_handle;
    git_oid object_oid;
    git_signature* sig = NULL;
    int ret = GM_OK;
    
    /* Special handling for type/path mappings which use synthetic SHAs */
    if (strstr(notes_ref, "gitmind/types") || strstr(notes_ref, "gitmind/paths")) {
        /* For type/path mappings, we need to create a blob first since
         * Git notes can only be attached to real Git objects */
        git_odb* odb = NULL;
        git_oid blob_oid;
        
        if (git_repository_odb(&odb, repo) < 0) {
            return GM_ERR_GIT;
        }
        
        /* Create a blob with the mapping content */
        if (git_odb_write(&blob_oid, odb, note_content, strlen(note_content), GIT_OBJECT_BLOB) < 0) {
            git_odb_free(odb);
            return GM_ERR_GIT;
        }
        
        git_odb_free(odb);
        
        /* Use the blob OID for the note */
        memcpy(&object_oid, &blob_oid, sizeof(git_oid));
    } else {
        /* Regular note - parse the provided SHA */
        if (git_oid_fromstr(&object_oid, object_sha) < 0) {
            return GM_ERR_INVALID_ARG;
        }
    }
    
    /* Create a signature for the note */
    if (git_signature_default(&sig, repo) < 0) {
        /* Fallback to a default signature */
        if (git_signature_now(&sig, "GitMind", "gitmind@neuroglyph.com") < 0) {
            return GM_ERR_GIT;
        }
    }
    
    /* Write the note (force=1 to overwrite if exists) */
    git_oid note_oid;
    if (git_note_create(&note_oid, repo, notes_ref, sig, sig, &object_oid, 
                        note_content, 1) < 0) {
        const git_error* e = git_error_last();
        fprintf(stderr, "DEBUG: git_note_create failed: %s (notes_ref=%s)\n", 
                e ? e->message : "unknown error", notes_ref);
        ret = GM_ERR_GIT;
    }
    
    if (sig) git_signature_free(sig);
    return ret;
}

static int libgit2_read_note(void* backend_data, const char* notes_ref,
                            const char* object_sha, char* out_content, size_t max_size) {
    gm_context_t* ctx = (gm_context_t*)backend_data;
    if (!ctx || !ctx->repo_handle) return GM_ERR_INVALID_ARG;
    
    git_repository* repo = (git_repository*)ctx->repo_handle;
    git_oid object_oid;
    git_note* note = NULL;
    int ret = GM_OK;
    
    if (git_oid_fromstr(&object_oid, object_sha) < 0) {
        return GM_ERR_INVALID_ARG;
    }
    
    if (git_note_read(&note, repo, notes_ref, &object_oid) < 0) {
        return GM_ERR_NOT_FOUND;
    }
    
    const char* message = git_note_message(note);
    size_t len = strlen(message);
    if (len >= max_size) {
        ret = GM_ERR_IO;
        goto cleanup;
    }
    
    strcpy(out_content, message);
    
cleanup:
    if (note) git_note_free(note);
    return ret;
}

static int libgit2_create_commit(void* backend_data, const char* tree_sha,
                                const char* parent_sha, const char* message, char* out_sha) {
    gm_context_t* ctx = (gm_context_t*)backend_data;
    if (!ctx || !ctx->repo_handle) return GM_ERR_INVALID_ARG;
    
    git_repository* repo = (git_repository*)ctx->repo_handle;
    git_oid tree_oid, parent_oid, commit_oid;
    git_tree* tree = NULL;
    git_commit* parent = NULL;
    git_signature* sig = NULL;
    int ret = GM_OK;
    
    if (git_oid_fromstr(&tree_oid, tree_sha) < 0) {
        return GM_ERR_INVALID_ARG;
    }
    
    if (git_tree_lookup(&tree, repo, &tree_oid) < 0) {
        return GM_ERR_NOT_FOUND;
    }
    
    /* Get parent commit if provided */
    const git_commit* parents[1] = {NULL};
    int parent_count = 0;
    if (parent_sha && strlen(parent_sha) > 0) {
        if (git_oid_fromstr(&parent_oid, parent_sha) < 0) {
            ret = GM_ERR_INVALID_ARG;
            goto cleanup;
        }
        if (git_commit_lookup(&parent, repo, &parent_oid) < 0) {
            ret = GM_ERR_NOT_FOUND;
            goto cleanup;
        }
        parents[0] = parent;
        parent_count = 1;
    }
    
    /* Create signature */
    if (git_signature_default(&sig, repo) < 0) {
        /* Fallback to a default signature */
        if (git_signature_now(&sig, "GitMind", "gitmind@neuroglyph.com") < 0) {
            ret = GM_ERR_GIT;
            goto cleanup;
        }
    }
    
    /* Create commit */
    if (git_commit_create(&commit_oid, repo, NULL, sig, sig, NULL, 
                          message, tree, parent_count, parents) < 0) {
        ret = GM_ERR_GIT;
        goto cleanup;
    }
    
    git_oid_tostr(out_sha, GIT_OID_HEXSZ + 1, &commit_oid);
    
cleanup:
    if (tree) git_tree_free(tree);
    if (parent) git_commit_free(parent);
    if (sig) git_signature_free(sig);
    return ret;
}

static int libgit2_read_commit_tree(void* backend_data, const char* commit_sha, char* out_tree_sha) {
    gm_context_t* ctx = (gm_context_t*)backend_data;
    if (!ctx || !ctx->repo_handle) return GM_ERR_INVALID_ARG;
    
    git_repository* repo = (git_repository*)ctx->repo_handle;
    git_oid commit_oid;
    git_commit* commit = NULL;
    int ret = GM_OK;
    
    if (git_oid_fromstr(&commit_oid, commit_sha) < 0) {
        return GM_ERR_INVALID_ARG;
    }
    
    if (git_commit_lookup(&commit, repo, &commit_oid) < 0) {
        return GM_ERR_NOT_FOUND;
    }
    
    const git_oid* tree_oid = git_commit_tree_id(commit);
    git_oid_tostr(out_tree_sha, GIT_OID_HEXSZ + 1, tree_oid);
    
    git_commit_free(commit);
    return ret;
}

/* Backend singleton */
static gm_backend_ops_t libgit2_backend = {
    .open_repo = libgit2_open_repo,
    .close_repo = libgit2_close_repo,
    .hash_object = libgit2_hash_object,
    .read_object = libgit2_read_object,
    .read_tree = libgit2_read_tree,
    .write_tree = libgit2_write_tree,
    .read_ref = libgit2_read_ref,
    .update_ref = libgit2_update_ref,
    .create_commit = libgit2_create_commit,
    .read_commit_tree = libgit2_read_commit_tree,
    .write_note = libgit2_write_note,
    .read_note = libgit2_read_note,
    .data = NULL
};

/* Get the libgit2 backend */
const gm_backend_ops_t* gm_backend_libgit2(void) {
    return &libgit2_backend;
}

/* Cleanup on exit */
__attribute__((destructor))
static void cleanup_libgit2(void) {
    if (libgit2_init_count > 0) {
        git_libgit2_shutdown();
        libgit2_init_count = 0;
    }
}