/* SPDX-License-Identifier: Apache-2.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind_lib.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Test backend state */
typedef struct {
    int should_fail_next;
    char fake_tree_sha[41];
    char fake_ref_sha[41];
    char fake_commit_sha[41];
} test_state_t;

static test_state_t test_state = {0};

/* Test implementations */
static int test_open_repo(void* backend_data, const char* path, void** out_handle) {
    (void)backend_data;
    (void)path;
    
    if (test_state.should_fail_next) {
        test_state.should_fail_next = 0;
        return GM_ERR_NOT_REPO;
    }
    
    *out_handle = (void*)0xDEADBEEF;  /* Fake handle */
    return GM_OK;
}

static void test_close_repo(void* backend_data, void* handle) {
    (void)backend_data;
    (void)handle;
}

static int test_hash_object(void* backend_data, const void* data, size_t len,
                           const char* type, char* out_sha) {
    (void)backend_data;
    (void)data;
    
    if (test_state.should_fail_next) {
        test_state.should_fail_next = 0;
        return GM_ERR_GIT;
    }
    
    /* Generate deterministic fake SHA */
    snprintf(out_sha, 41, "test%s%08zx", type, len);
    return GM_OK;
}

static int test_read_object(void* backend_data, const char* sha, void* out_data,
                           size_t max_size, size_t* actual_size) {
    (void)backend_data;
    (void)sha;
    
    const char* fake_content = "test object content";
    size_t len = strlen(fake_content);
    
    if (len > max_size) return GM_ERR_IO;
    
    memcpy(out_data, fake_content, len);
    if (actual_size) *actual_size = len;
    return GM_OK;
}

static int test_read_tree(void* backend_data, const char* tree_sha, char* out_entries) {
    (void)backend_data;
    (void)tree_sha;
    
    /* Return fake tree entries */
    strcpy(out_entries, "100644 blob abc123 file1.txt\n040000 tree def456 subdir");
    return GM_OK;
}

static int test_write_tree(void* backend_data, const char* entries, char* out_sha) {
    (void)backend_data;
    (void)entries;
    
    if (test_state.fake_tree_sha[0]) {
        strcpy(out_sha, test_state.fake_tree_sha);
    } else {
        strcpy(out_sha, "testtree12345678");
    }
    return GM_OK;
}

static int test_read_ref(void* backend_data, const char* ref_name, char* out_sha) {
    (void)backend_data;
    
    if (test_state.fake_ref_sha[0]) {
        strcpy(out_sha, test_state.fake_ref_sha);
    } else if (strcmp(ref_name, "refs/gitmind/graph") == 0) {
        strcpy(out_sha, "testcommit123456");
    } else {
        return GM_ERR_NOT_FOUND;
    }
    return GM_OK;
}

static int test_update_ref(void* backend_data, const char* ref_name,
                          const char* new_sha, const char* message) {
    (void)backend_data;
    (void)ref_name;
    (void)message;
    
    /* Store for future reads */
    strncpy(test_state.fake_ref_sha, new_sha, 40);
    test_state.fake_ref_sha[40] = '\0';
    return GM_OK;
}

static int test_create_commit(void* backend_data, const char* tree_sha,
                             const char* parent_sha, const char* message, char* out_sha) {
    (void)backend_data;
    (void)message;
    
    /* Generate fake commit SHA */
    if (parent_sha && parent_sha[0]) {
        snprintf(out_sha, 41, "commit%s", tree_sha);
    } else {
        snprintf(out_sha, 41, "orphan%s", tree_sha);
    }
    
    /* Store for future reference */
    strcpy(test_state.fake_commit_sha, out_sha);
    return GM_OK;
}

static int test_read_commit_tree(void* backend_data, const char* commit_sha, char* out_tree_sha) {
    (void)backend_data;
    
    /* Return consistent tree SHA */
    if (strncmp(commit_sha, "orphan", 6) == 0) {
        strcpy(out_tree_sha, "emptytree000000");
    } else {
        strcpy(out_tree_sha, "testtree12345678");
    }
    return GM_OK;
}

static int test_write_note(void* backend_data, const char* notes_ref,
                          const char* object_sha, const char* note_content) {
    (void)backend_data;
    (void)notes_ref;
    (void)object_sha;
    (void)note_content;
    return GM_OK;
}

static int test_read_note(void* backend_data, const char* notes_ref,
                         const char* object_sha, char* out_content, size_t max_size) {
    (void)backend_data;
    (void)notes_ref;
    (void)object_sha;
    
    /* Return fake note */
    const char* fake = "test note";
    strncpy(out_content, fake, max_size - 1);
    out_content[max_size - 1] = '\0';
    return GM_OK;
}

/* Test backend singleton */
static gm_backend_ops_t test_backend = {
    .open_repo = test_open_repo,
    .close_repo = test_close_repo,
    .hash_object = test_hash_object,
    .read_object = test_read_object,
    .read_tree = test_read_tree,
    .write_tree = test_write_tree,
    .read_ref = test_read_ref,
    .update_ref = test_update_ref,
    .create_commit = test_create_commit,
    .read_commit_tree = test_read_commit_tree,
    .write_note = test_write_note,
    .read_note = test_read_note,
    .data = NULL
};

/* Get test backend */
const gm_backend_ops_t* gm_backend_test(void) {
    return &test_backend;
}

/* Test control functions */
void gm_test_backend_reset(void) {
    memset(&test_state, 0, sizeof(test_state));
}

void gm_test_backend_fail_next(void) {
    test_state.should_fail_next = 1;
}

void gm_test_backend_set_tree_sha(const char* sha) {
    strncpy(test_state.fake_tree_sha, sha, 40);
    test_state.fake_tree_sha[40] = '\0';
}