/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "gitmind/cache.h"
#include "gitmind/constants_internal.h"
#include "gitmind/context.h"
#include "gitmind/error.h"
#include "gitmind/result.h"
#include "gitmind/security/memory.h"
#include "gitmind/util/oid.h"

typedef struct test_git_repo_port {
    gm_git_repository_port_t port;
    gm_git_reference_tip_t cache_tip;
    gm_git_reference_tip_t journal_tips[3];
    size_t journal_tip_count;
    size_t journal_tip_index;
    gm_git_reference_tip_t glob_tip;
} test_git_repo_port_t;

/**
 * Provide the repository path for the requested kind (test stub).
 *
 * This stub does not implement path resolution and always returns an error
 * indicating the operation is not implemented.
 *
 * @returns A gm_result_void_t error result containing `GM_ERR_NOT_IMPLEMENTED`.
 */
static gm_result_void_t stub_repository_path(void *self,
                                             gm_git_repository_path_kind_t kind,
                                             char *out_buffer,
                                             size_t buffer_size) {
    (void)self;
    (void)kind;
    (void)out_buffer;
    (void)buffer_size;
    return gm_err_void(GM_ERROR(GM_ERR_NOT_IMPLEMENTED, "test stub"));
}

/**
 * Stub implementation for retrieving the current HEAD branch name used in tests.
 *
 * @param self Opaque pointer to the repository port instance (ignored).
 * @param out_name Buffer to receive the branch name (ignored).
 * @param out_name_size Size of the out_name buffer (ignored).
 * @returns A gm_result_void_t representing the `GM_ERR_NOT_IMPLEMENTED` error.
 */
static gm_result_void_t stub_head_branch(void *self, char *out_name,
                                         size_t out_name_size) {
    (void)self;
    (void)out_name;
    (void)out_name_size;
    return gm_err_void(GM_ERROR(GM_ERR_NOT_IMPLEMENTED, "test stub"));
}

/**
 * Test stub for building a tree that always reports the operation as not implemented.
 *
 * This function does not construct or modify any tree and exists only for testing;
 * it always fails with a "not implemented" error.
 *
 * @returns An error result with `GM_ERR_NOT_IMPLEMENTED`.
 */
static gm_result_void_t stub_build_tree(void *self, const char *dir_path,
                                        gm_oid_t *out_tree_oid) {
    (void)self;
    (void)dir_path;
    (void)out_tree_oid;
    return gm_err_void(GM_ERROR(GM_ERR_NOT_IMPLEMENTED, "test stub"));
}

/**
 * Populate the output reference tip based on the provided reference name.
 *
 * If ref_name starts with GM_CACHE_REF_PREFIX, copies the stub's cache_tip to out_tip.
 * If ref_name starts with GITMIND_EDGES_REF_PREFIX, copies the current journal tip
 * (stub->journal_tips[stub->journal_tip_index]) to out_tip and advances journal_tip_index
 * by one unless already at the last tip. For any other ref_name, sets out_tip to an all-zero tip.
 *
 * @param ref_name The reference name used to select which tip to return.
 * @param out_tip Pointer to receive the selected or zeroed gm_git_reference_tip_t.
 * @returns `GM_OK` on success; `GM_ERR_INVALID_ARGUMENT` if ref_name or out_tip is NULL.
 */
static gm_result_void_t reference_tip(void *self, const char *ref_name,
                                      gm_git_reference_tip_t *out_tip) {
    test_git_repo_port_t *stub = (test_git_repo_port_t *)self;
    if (ref_name == NULL || out_tip == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "ref_name and out_tip required"));
    }

    if (strncmp(ref_name, GM_CACHE_REF_PREFIX,
                strlen(GM_CACHE_REF_PREFIX)) == 0) {
        *out_tip = stub->cache_tip;
        return gm_ok_void();
    }

    if (strncmp(ref_name, GITMIND_EDGES_REF_PREFIX,
                strlen(GITMIND_EDGES_REF_PREFIX)) == 0) {
        if (stub->journal_tip_count == 0U) {
            gm_memset_safe(out_tip, sizeof(*out_tip), 0, sizeof(*out_tip));
            return gm_ok_void();
        }
        size_t index = stub->journal_tip_index;
        if (index >= stub->journal_tip_count) {
            index = stub->journal_tip_count - 1U;
        }
        *out_tip = stub->journal_tips[index];
        if (stub->journal_tip_index + 1U < stub->journal_tip_count) {
            stub->journal_tip_index += 1U;
        }
        return gm_ok_void();
    }

    gm_memset_safe(out_tip, sizeof(*out_tip), 0, sizeof(*out_tip));
    return gm_ok_void();
}

/**
 * Retrieve the latest reference tip matching a glob pattern from the test stub.
 *
 * The provided pattern is validated for non-NULL but not inspected; the function
 * always returns the stub's preset glob_tip into `out_tip`.
 *
 * @param pattern Glob pattern to match (validated for non-NULL; otherwise ignored).
 * @param out_tip Output pointer that receives the latest matching reference tip.
 * @returns `gm_ok_void()` on success; `gm_err_void(GM_ERR_INVALID_ARGUMENT, ...)`
 *          if `pattern` or `out_tip` is NULL.
 */
static gm_result_void_t reference_glob_latest(void *self, const char *pattern,
                                              gm_git_reference_tip_t *out_tip) {
    test_git_repo_port_t *stub = (test_git_repo_port_t *)self;
    if (pattern == NULL || out_tip == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "pattern and out_tip required"));
    }
    (void)pattern;
    *out_tip = stub->glob_tip;
    return gm_ok_void();
}

/**
 * Stub implementation of reading a blob at a commit path that always fails.
 *
 * If provided, sets `*out_data` to NULL and `*out_size` to 0 before returning.
 *
 * @param out_data If non-NULL, receives NULL to indicate no data was produced.
 * @param out_size If non-NULL, receives 0 to indicate zero bytes.
 * @returns An error result indicating the operation is not implemented.
 */
static gm_result_void_t stub_commit_read_blob(void *self,
                                              const gm_oid_t *commit_oid,
                                              const char *path,
                                              uint8_t **out_data,
                                              size_t *out_size) {
    (void)self;
    (void)commit_oid;
    (void)path;
    if (out_data != NULL) {
        *out_data = NULL;
    }
    if (out_size != NULL) {
        *out_size = 0U;
    }
    return gm_err_void(GM_ERROR(GM_ERR_NOT_IMPLEMENTED, "test stub"));
}

/**
 * Free memory backing a blob buffer.
 *
 * @param data Pointer to the blob buffer to free.
 * @param size Size of the blob buffer in bytes (unused by this implementation).
 */
static void blob_dispose(void *self, uint8_t *data, size_t size) {
    (void)self;
    (void)size;
    free(data);
}

/**
 * Stub implementation that attempts to read a commit message but always fails and clears the output pointer.
 *
 * @param commit_oid Commit OID to read; ignored by this test stub.
 * @param out_message Pointer to receive the commit message; if non-NULL, set to NULL.
 * @returns An error indicating the operation is not implemented (`GM_ERR_NOT_IMPLEMENTED`) wrapped in a `gm_result_void_t`.
 */
static gm_result_void_t stub_commit_read_message(void *self,
                                                 const gm_oid_t *commit_oid,
                                                 char **out_message) {
    (void)self;
    (void)commit_oid;
    if (out_message != NULL) {
        *out_message = NULL;
    }
    return gm_err_void(GM_ERROR(GM_ERR_NOT_IMPLEMENTED, "test stub"));
}

/**
 * Free a heap-allocated commit message.
 *
 * @param message Pointer to a heap-allocated NUL-terminated commit message to free; may be NULL.
 */
static void stub_commit_message_dispose(void *self, char *message) {
    (void)self;
    free(message);
}

/**
 * Test stub for walking commits that always reports the operation as not implemented.
 *
 * @returns An error result with `GM_ERR_NOT_IMPLEMENTED`.
 */
static gm_result_void_t stub_walk_commits(void *self, const char *ref_name,
                                          gm_git_commit_visit_cb visit_callback,
                                          void *userdata) {
    (void)self;
    (void)ref_name;
    (void)visit_callback;
    (void)userdata;
    return gm_err_void(GM_ERROR(GM_ERR_NOT_IMPLEMENTED, "test stub"));
}

/**
 * Stub implementation of commit tree size retrieval for tests.
 *
 * If `out_size_bytes` is non-NULL, it is set to 0. The function always
 * returns an error result indicating the operation is not implemented.
 *
 * @param self Unused stub instance pointer.
 * @param commit_oid Unused commit OID.
 * @param out_size_bytes If non-NULL, receives 0.
 * @returns A gm_result_void_t containing `GM_ERR_NOT_IMPLEMENTED`.
 */
static gm_result_void_t stub_commit_tree_size(void *self,
                                              const gm_oid_t *commit_oid,
                                              uint64_t *out_size_bytes) {
    (void)self;
    (void)commit_oid;
    if (out_size_bytes != NULL) {
        *out_size_bytes = 0U;
    }
    return gm_err_void(GM_ERROR(GM_ERR_NOT_IMPLEMENTED, "test stub"));
}

/**
 * Test stub for creating a commit in the repository port.
 *
 * This implementation does not create a commit and always returns a not-implemented error.
 *
 * @param self Pointer to the repository port instance (unused).
 * @param spec Specification of the commit to create (unused).
 * @param out_commit_oid Output OID for the created commit (unused).
 * @returns A gm_result_void_t representing `GM_ERR_NOT_IMPLEMENTED`.
 */
static gm_result_void_t stub_commit_create(void *self,
                                           const gm_git_commit_spec_t *spec,
                                           gm_oid_t *out_commit_oid) {
    (void)self;
    (void)spec;
    (void)out_commit_oid;
    return gm_err_void(GM_ERROR(GM_ERR_NOT_IMPLEMENTED, "test stub"));
}

/**
 * Stub implementation of a reference update used by tests that always reports not implemented.
 *
 * @returns A `gm_result_void_t` representing an error with code `GM_ERR_NOT_IMPLEMENTED` and message "test stub".
 */
static gm_result_void_t stub_reference_update(
    void *self, const gm_git_reference_update_spec_t *spec) {
    (void)self;
    (void)spec;
    return gm_err_void(GM_ERROR(GM_ERR_NOT_IMPLEMENTED, "test stub"));
}

/**
 * Stub that would resolve the blob OID for `path` at HEAD in test repository ports.
 *
 * @param self Pointer to the repository port instance (unused).
 * @param path Path of the blob to resolve.
 * @param out_blob_oid Output pointer to receive the blob OID when resolution succeeds.
 * @returns A `gm_result_void_t` error value indicating not implemented (`GM_ERR_NOT_IMPLEMENTED`) for this test stub.
 */
static gm_result_void_t stub_resolve_blob_at_head(void *self, const char *path,
                                                  gm_oid_t *out_blob_oid) {
    (void)self;
    (void)path;
    (void)out_blob_oid;
    return gm_err_void(GM_ERROR(GM_ERR_NOT_IMPLEMENTED, "test stub"));
}

/**
 * Test stub that intentionally does not resolve a blob at a specific commit.
 *
 * @returns An error result with code GM_ERR_NOT_IMPLEMENTED indicating the operation is not implemented in the test stub.
 */
static gm_result_void_t stub_resolve_blob_at_commit(void *self,
                                                    const gm_oid_t *commit_oid,
                                                    const char *path,
                                                    gm_oid_t *out_blob_oid) {
    (void)self;
    (void)commit_oid;
    (void)path;
    (void)out_blob_oid;
    return gm_err_void(GM_ERROR(GM_ERR_NOT_IMPLEMENTED, "test stub"));
}

/**
 * Stub that reports a commit has zero parents and indicates the operation is not implemented.
 *
 * If `out_parent_count` is non-NULL, it is set to 0 to indicate no parents.
 *
 * @param self Opaque repository port instance (unused).
 * @param commit_oid OID of the commit to query (unused).
 * @param out_parent_count Pointer to receive the parent count; set to 0 if non-NULL.
 * @returns An error result indicating the function is not implemented (`GM_ERR_NOT_IMPLEMENTED`).
 */
static gm_result_void_t stub_commit_parent_count(void *self,
                                                 const gm_oid_t *commit_oid,
                                                 size_t *out_parent_count) {
    (void)self;
    (void)commit_oid;
    if (out_parent_count != NULL) {
        *out_parent_count = 0U;
    }
    return gm_err_void(GM_ERROR(GM_ERR_NOT_IMPLEMENTED, "test stub"));
}

static const gm_git_repository_port_vtbl_t TEST_PORT_VTBL = {
    .repository_path = stub_repository_path,
    .head_branch = stub_head_branch,
    .build_tree_from_directory = stub_build_tree,
    .reference_tip = reference_tip,
    .reference_glob_latest = reference_glob_latest,
    .commit_read_blob = stub_commit_read_blob,
    .blob_dispose = blob_dispose,
    .commit_read_message = stub_commit_read_message,
    .commit_message_dispose = stub_commit_message_dispose,
    .walk_commits = stub_walk_commits,
    .commit_tree_size = stub_commit_tree_size,
    .commit_create = stub_commit_create,
    .reference_update = stub_reference_update,
    .resolve_blob_at_head = stub_resolve_blob_at_head,
    .resolve_blob_at_commit = stub_resolve_blob_at_commit,
    .commit_parent_count = stub_commit_parent_count,
};

/**
 * Prepare a test_git_repo_port_t for use by zeroing it and configuring its repository port vtable and self pointer.
 * @param stub Pointer to the test stub to initialize.
 */
static void init_stub(test_git_repo_port_t *stub) {
    gm_memset_safe(stub, sizeof(*stub), 0, sizeof(*stub));
    stub->port.vtbl = &TEST_PORT_VTBL;
    stub->port.self = stub;
}

/**
 * Initialize `oid` from a hexadecimal string and abort on failure.
 *
 * @param oid Pointer to the OID to be populated.
 * @param hex NUL-terminated hexadecimal string representation of the OID.
 */
static void set_oid_from_hex(gm_oid_t *oid, const char *hex) {
    int rc = gm_oid_from_hex(oid, hex);
    assert(rc == GM_OK);
}

/**
 * Initialize a git reference tip structure with the given OID hex, commit time, and flags.
 *
 * Populates the provided gm_git_reference_tip_t by zeroing it and setting has_target and commit_time.
 * If `zero_binary` is false and `hex` is non-NULL, the binary OID is parsed from `hex` and stored.
 * If `hex` is non-NULL, its characters are copied into `tip->oid_hex` and NUL-terminated (truncated if necessary).
 *
 * @param tip Pointer to the tip structure to initialize.
 * @param hex Hexadecimal string representation of the OID to store or copy; may be NULL.
 * @param commit_time Commit timestamp to assign to the tip.
 * @param has_target Whether the tip has a target reference.
 * @param zero_binary If true, do not populate the binary OID even if `hex` is provided.
 */
static void set_tip(gm_git_reference_tip_t *tip, const char *hex,
                    uint64_t commit_time, bool has_target, bool zero_binary) {
    gm_memset_safe(tip, sizeof(*tip), 0, sizeof(*tip));
    tip->has_target = has_target;
    tip->commit_time = commit_time;
    if (!zero_binary && hex != NULL) {
        set_oid_from_hex(&tip->oid, hex);
    }
    if (hex != NULL) {
        size_t len = strlen(hex);
        if (len >= sizeof(tip->oid_hex)) {
            len = sizeof(tip->oid_hex) - 1U;
        }
        memcpy(tip->oid_hex, hex, len);
        tip->oid_hex[len] = '\0';
    }
}

/**
 * Fill a buffer with a repeated character and ensure null-termination.
 *
 * Writes `digit` into each of the first `buffer_len - 1` bytes and sets the
 * final byte to the NUL terminator (`'\0'`).
 *
 * @param buffer Pointer to the destination buffer.
 * @param buffer_len Total size of `buffer` in bytes; must be at least 1.
 * @param digit Character to repeat throughout the buffer (except the terminator).
 */
static void fill_hex(char *buffer, size_t buffer_len, char digit) {
    for (size_t i = 0; i + 1U < buffer_len; ++i) {
        buffer[i] = digit;
    }
    buffer[buffer_len - 1U] = '\0';
}

/**
 * Reset the journal tip sequence index to the beginning.
 *
 * This sets the test repository port's journal_tip_index to 0 so subsequent
 * journal tip retrievals start from the first entry.
 *
 * @param stub Test repository port whose journal sequence will be reset.
 */
static void reset_journal_sequence(test_git_repo_port_t *stub) {
    stub->journal_tip_index = 0U;
}

/**
 * Run unit tests that validate strict equality behavior of the cache check.
 *
 * Executes four scenarios that exercise gm_cache_is_stale with combinations of
 * binary OID presence and legacy hex values to verify OID-first equality and
 * hex-string fallback behavior.
 *
 * @returns 0 on success; a non-zero result (via failed assertion or abort) on failure.
 */
int main(void) {
    printf("test_cache_strict_equality... ");

    time_t now = time(NULL);
    assert(now > 0);

    /* Scenario 1: OID-first equality ignores mismatched legacy hex */
    test_git_repo_port_t stub1;
    init_stub(&stub1);
    stub1.journal_tip_count = 2U;

    set_tip(&stub1.cache_tip, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
            (uint64_t)now, true, false);
    stub1.glob_tip = stub1.cache_tip;

    set_tip(&stub1.journal_tips[0], "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
            (uint64_t)now, true, false);
    fill_hex(stub1.journal_tips[0].oid_hex,
             sizeof stub1.journal_tips[0].oid_hex, 'c');

    set_tip(&stub1.journal_tips[1], "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb",
            (uint64_t)now, true, false);

    gm_context_t ctx1 = {0};
    ctx1.git_repo_port = stub1.port;

    bool stale = gm_cache_is_stale(&ctx1, "main");
    assert(stale == false);

    /* Scenario 2: OID-first equality detects mismatch */
    reset_journal_sequence(&stub1);
    set_tip(&stub1.journal_tips[1], "cccccccccccccccccccccccccccccccccccccccc",
            (uint64_t)now, true, false);
    stale = gm_cache_is_stale(&ctx1, "main");
    assert(stale == true);

    /* Scenario 3: fallback uses hex string when binary OID missing */
    test_git_repo_port_t stub2;
    init_stub(&stub2);
    stub2.journal_tip_count = 2U;

    set_tip(&stub2.cache_tip, "dddddddddddddddddddddddddddddddddddddddd",
            (uint64_t)now, true, false);
    stub2.glob_tip = stub2.cache_tip;

    set_tip(&stub2.journal_tips[0], "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
            (uint64_t)now, true, true);
    set_tip(&stub2.journal_tips[1], "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee",
            (uint64_t)now, true, false);

    gm_context_t ctx2 = {0};
    ctx2.git_repo_port = stub2.port;

    stale = gm_cache_is_stale(&ctx2, "develop");
    assert(stale == false);

    /* Scenario 4: fallback detects mismatch when hex differs */
    reset_journal_sequence(&stub2);
    set_tip(&stub2.journal_tips[1], "ffffffffffffffffffffffffffffffffffffffff",
            (uint64_t)now, true, false);
    stale = gm_cache_is_stale(&ctx2, "develop");
    assert(stale == true);

    printf("OK\n");
    return 0;
}
