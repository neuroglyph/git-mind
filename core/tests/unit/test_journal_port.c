/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gitmind/context.h"
#include "gitmind/ports/journal_command_port.h"
#include "gitmind/edge.h"

#include "core/tests/fakes/git/fake_git_repository_port.h"

/* no env helpers needed */

int main(void) {
    printf("test_journal_port.append... ");

    /* Set branch via fake repo head */

    gm_context_t ctx = {0};
    gm_fake_git_repository_port_t fake = {0};
    assert(gm_fake_git_repository_port_init(&fake, "/fake/.git", "/fake")
               .ok);
    ctx.git_repo_port = fake.port;
    assert(gm_fake_git_repository_port_set_head_branch(&fake, "main").ok);

    gm_cmd_journal_port_t port = {0};
    assert(gm_cmd_journal_port_init(&port, &ctx).ok);

    gm_edge_t e = {0};
    /* minimal valid edge for encoding */
    e.src_oid.id[0] = 1;
    e.tgt_oid.id[0] = 2;
    e.rel_type = 1;
    e.confidence = 1;
    e.timestamp = 0;

    gm_result_void_t r = port.vtbl->append(&port, &e, 1);
    assert(r.ok);

    /* Verify ref update recorded on fake port */
    const char *last_ref = gm_fake_git_repository_port_last_update_ref(&fake);
    assert(last_ref != NULL);
    assert(strstr(last_ref, "refs/gitmind/edges/main") != NULL);

    gm_cmd_journal_port_dispose(&port);
    gm_fake_git_repository_port_dispose(&fake);
    printf("OK\n");
    return 0;
}
