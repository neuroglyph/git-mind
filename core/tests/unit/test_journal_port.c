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

#ifndef _WIN32
extern int putenv(char *);
#endif

static void set_env(const char *k, const char *v) {
    char buf[256];
    if (v == NULL) v = "";
    int n = snprintf(buf, sizeof(buf), "%s=%s", k, v);
    assert(n > 0 && (size_t)n < sizeof(buf));
    char *heap = (char *)malloc((size_t)n + 1);
    assert(heap != NULL);
    memcpy(heap, buf, (size_t)n + 1);
    assert(putenv(heap) == 0);
}

int main(void) {
    printf("test_journal_port.append... ");

    /* Force branch for writer */
    set_env("GITMIND_TEST_BRANCH", "main");

    gm_context_t ctx = {0};
    gm_fake_git_repository_port_t fake = {0};
    assert(gm_fake_git_repository_port_init(&fake, "/fake/.git", "/fake")
               .ok);
    ctx.git_repo_port = fake.port;

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

    /* Clear test branch env to avoid leaking into subsequent tests */
    set_env("GITMIND_TEST_BRANCH", "");

    gm_cmd_journal_port_dispose(&port);
    gm_fake_git_repository_port_dispose(&fake);
    printf("OK\n");
    return 0;
}
