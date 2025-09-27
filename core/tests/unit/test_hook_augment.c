/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "gitmind/context.h"
#include "gitmind/hooks/augment.h"
#include "gitmind/ports/git_repository_port.h"
#include "gitmind/types.h"

#include "core/tests/fakes/git/fake_git_repository_port.h"

static void fill_oid(gm_oid_t *oid, uint8_t seed) {
    memset(oid, 0, sizeof(*oid));
    oid->id[0] = seed;
}

static void test_get_blob_sha_head_and_parent(void) {
    printf("test_get_blob_sha_head_and_parent... ");
    gm_fake_git_repository_port_t fake;
    assert(gm_fake_git_repository_port_init(&fake, NULL, NULL).ok);
    assert(gm_fake_git_repository_port_set_head_branch(&fake, "main").ok);

    gm_oid_t head_commit;
    gm_oid_t parent_commit;
    fill_oid(&head_commit, 0x10U);
    fill_oid(&parent_commit, 0x20U);

    assert(gm_fake_git_repository_port_add_ref_commit(&fake, "refs/heads/main",
                                                      &head_commit, NULL)
               .ok);
    assert(gm_fake_git_repository_port_add_ref_commit(&fake, "refs/heads/main",
                                                      &parent_commit, NULL)
               .ok);

    gm_oid_t parents[1] = {parent_commit};
    assert(gm_fake_git_repository_port_set_commit_parents(&fake, &head_commit,
                                                          parents, 1U)
               .ok);

    gm_git_reference_tip_t tip = {
        .has_target = true,
        .oid = head_commit,
    };
    gm_fake_git_repository_port_set_tip(&fake, &tip);

    gm_oid_t new_blob;
    gm_oid_t old_blob;
    fill_oid(&new_blob, 0xA0U);
    fill_oid(&old_blob, 0xB0U);

    assert(gm_fake_git_repository_port_add_commit_blob_mapping(
               &fake, &head_commit, "README.md", &new_blob)
               .ok);
    assert(gm_fake_git_repository_port_add_commit_blob_mapping(
               &fake, &parent_commit, "README.md", &old_blob)
               .ok);

    gm_oid_t resolved;
    int rc = gm_hook_get_blob_sha(&fake.port, "HEAD", "README.md", &resolved);
    assert(rc == GM_OK);
    assert(memcmp(resolved.id, new_blob.id, GM_OID_RAWSZ) == 0);

    gm_oid_t resolved_parent;
    rc = gm_hook_get_blob_sha(&fake.port, "HEAD~1", "README.md",
                              &resolved_parent);
    assert(rc == GM_OK);
    assert(memcmp(resolved_parent.id, old_blob.id, GM_OID_RAWSZ) == 0);

    gm_fake_git_repository_port_dispose(&fake);
    printf("OK\n");
}

static void test_is_merge_commit_detection(void) {
    printf("test_is_merge_commit_detection... ");
    gm_fake_git_repository_port_t fake;
    assert(gm_fake_git_repository_port_init(&fake, NULL, NULL).ok);
    assert(gm_fake_git_repository_port_set_head_branch(&fake, "feature").ok);

    gm_oid_t parent_a;
    gm_oid_t parent_b;
    gm_oid_t head_commit;
    fill_oid(&parent_a, 0x31U);
    fill_oid(&parent_b, 0x32U);
    fill_oid(&head_commit, 0x33U);

    assert(gm_fake_git_repository_port_add_ref_commit(&fake,
                                                      "refs/heads/feature",
                                                      &head_commit, NULL)
               .ok);
    assert(gm_fake_git_repository_port_add_ref_commit(&fake,
                                                      "refs/heads/feature",
                                                      &parent_a, NULL)
               .ok);
    assert(gm_fake_git_repository_port_add_ref_commit(&fake,
                                                      "refs/heads/feature",
                                                      &parent_b, NULL)
               .ok);

    gm_oid_t parents[2] = {parent_a, parent_b};
    assert(gm_fake_git_repository_port_set_commit_parents(&fake, &head_commit,
                                                          parents, 2U)
               .ok);

    gm_git_reference_tip_t tip = {
        .has_target = true,
        .oid = head_commit,
    };
    gm_fake_git_repository_port_set_tip(&fake, &tip);

    bool is_merge = false;
    int rc = gm_hook_is_merge_commit(&fake.port, &is_merge);
    assert(rc == GM_OK);
    assert(is_merge);

    gm_fake_git_repository_port_dispose(&fake);
    printf("OK\n");
}

static void test_is_merge_commit_linear_history(void) {
    printf("test_is_merge_commit_linear_history... ");
    gm_fake_git_repository_port_t fake;
    assert(gm_fake_git_repository_port_init(&fake, NULL, NULL).ok);
    assert(gm_fake_git_repository_port_set_head_branch(&fake, "linear").ok);

    gm_oid_t parent;
    gm_oid_t head_commit;
    fill_oid(&parent, 0x41U);
    fill_oid(&head_commit, 0x42U);

    assert(gm_fake_git_repository_port_add_ref_commit(&fake,
                                                      "refs/heads/linear",
                                                      &head_commit, NULL)
               .ok);
    assert(gm_fake_git_repository_port_add_ref_commit(&fake,
                                                      "refs/heads/linear",
                                                      &parent, NULL)
               .ok);

    gm_oid_t parents[1] = {parent};
    assert(gm_fake_git_repository_port_set_commit_parents(&fake, &head_commit,
                                                          parents, 1U)
               .ok);

    gm_git_reference_tip_t tip = {
        .has_target = true,
        .oid = head_commit,
    };
    gm_fake_git_repository_port_set_tip(&fake, &tip);

    bool is_merge = true;
    int rc = gm_hook_is_merge_commit(&fake.port, &is_merge);
    assert(rc == GM_OK);
    assert(!is_merge);

    gm_fake_git_repository_port_dispose(&fake);
    printf("OK\n");
}

int main(void) {
    test_get_blob_sha_head_and_parent();
    test_is_merge_commit_detection();
    test_is_merge_commit_linear_history();
    return 0;
}
