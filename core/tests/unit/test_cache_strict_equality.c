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

static gm_result_void_t stub_head_branch(void *self, char *out_name,
                                         size_t out_name_size) {
    (void)self;
    (void)out_name;
    (void)out_name_size;
    return gm_err_void(GM_ERROR(GM_ERR_NOT_IMPLEMENTED, "test stub"));
}

static gm_result_void_t stub_build_tree(void *self, const char *dir_path,
                                        gm_oid_t *out_tree_oid) {
    (void)self;
    (void)dir_path;
    (void)out_tree_oid;
    return gm_err_void(GM_ERROR(GM_ERR_NOT_IMPLEMENTED, "test stub"));
}

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

static void blob_dispose(void *self, uint8_t *data, size_t size) {
    (void)self;
    (void)size;
    free(data);
}

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

static void stub_commit_message_dispose(void *self, char *message) {
    (void)self;
    free(message);
}

static gm_result_void_t stub_walk_commits(void *self, const char *ref_name,
                                          gm_git_commit_visit_cb visit_callback,
                                          void *userdata) {
    (void)self;
    (void)ref_name;
    (void)visit_callback;
    (void)userdata;
    return gm_err_void(GM_ERROR(GM_ERR_NOT_IMPLEMENTED, "test stub"));
}

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

static gm_result_void_t stub_commit_create(void *self,
                                           const gm_git_commit_spec_t *spec,
                                           gm_oid_t *out_commit_oid) {
    (void)self;
    (void)spec;
    (void)out_commit_oid;
    return gm_err_void(GM_ERROR(GM_ERR_NOT_IMPLEMENTED, "test stub"));
}

static gm_result_void_t stub_reference_update(
    void *self, const gm_git_reference_update_spec_t *spec) {
    (void)self;
    (void)spec;
    return gm_err_void(GM_ERROR(GM_ERR_NOT_IMPLEMENTED, "test stub"));
}

static gm_result_void_t stub_resolve_blob_at_head(void *self, const char *path,
                                                  gm_oid_t *out_blob_oid) {
    (void)self;
    (void)path;
    (void)out_blob_oid;
    return gm_err_void(GM_ERROR(GM_ERR_NOT_IMPLEMENTED, "test stub"));
}

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

static void init_stub(test_git_repo_port_t *stub) {
    gm_memset_safe(stub, sizeof(*stub), 0, sizeof(*stub));
    stub->port.vtbl = &TEST_PORT_VTBL;
    stub->port.self = stub;
}

static void set_oid_from_hex(gm_oid_t *oid, const char *hex) {
    int rc = gm_oid_from_hex(oid, hex);
    assert(rc == GM_OK);
}

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

static void fill_hex(char *buffer, size_t buffer_len, char digit) {
    for (size_t i = 0; i + 1U < buffer_len; ++i) {
        buffer[i] = digit;
    }
    buffer[buffer_len - 1U] = '\0';
}

static void reset_journal_sequence(test_git_repo_port_t *stub) {
    stub->journal_tip_index = 0U;
}

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
