/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
extern int putenv(char *);
#endif

#include "gitmind/telemetry/internal/config.h"

static void set_env(const char *k, const char *v) {
    /* Portable: use putenv with a heap string that must persist */
    char buf[256];
    if (v == NULL) v = ""; /* empty resets to default behavior */
    int n = snprintf(buf, sizeof(buf), "%s=%s", k, v);
    assert(n > 0 && (size_t)n < sizeof(buf));
    char *heap = (char *)malloc((size_t)n + 1);
    assert(heap != NULL);
    memcpy(heap, buf, (size_t)n + 1);
    assert(putenv(heap) == 0);
}

static void test_defaults_branch_mode_only(void) {
    printf("test_telemetry_cfg.defaults... ");
    set_env("GITMIND_METRICS_ENABLED", NULL);
    set_env("GITMIND_METRICS_BRANCH_TAG", NULL);
    set_env("GITMIND_METRICS_MODE_TAG", NULL);
    set_env("GITMIND_METRICS_REPO_TAG", NULL);
    set_env("GITMIND_METRICS_EXTRA_TAGS", NULL);
    set_env("GITMIND_LOG_LEVEL", NULL);
    set_env("GITMIND_LOG_FORMAT", NULL);

    gm_telemetry_cfg_t cfg;
    assert(gm_telemetry_cfg_load(&cfg, NULL).ok);
    assert(cfg.metrics_enabled);
    assert(cfg.tag_branch);
    assert(cfg.tag_mode);
    assert(cfg.repo_tag == GM_REPO_TAG_OFF);

    char tags[128];
    assert(gm_telemetry_build_tags(&cfg, "main", "full", NULL, NULL, tags,
                                   sizeof(tags))
               .ok);
    assert(strcmp(tags, "branch=main,mode=full") == 0);
    printf("OK\n");
}

static void test_extras_and_invalids(void) {
    printf("test_telemetry_cfg.extras... ");
    set_env("GITMIND_METRICS_EXTRA_TAGS",
            "team=dev,invalid key=bad,role=ops,too_many=1");
    gm_telemetry_cfg_t cfg;
    assert(gm_telemetry_cfg_load(&cfg, NULL).ok);
    /* Should keep up to three valid extras, drop invalid key */
    assert(cfg.extra_count <= 3);
    assert(cfg.extras_dropped);

    char tags[256];
    assert(gm_telemetry_build_tags(&cfg, "main", "full", NULL, NULL, tags,
                                   sizeof(tags))
               .ok);
    /* Order-preserving insert up to capacity (5 total) */
    /* We know branch+mode consume 2; at most 3 extras appended */
    assert(strstr(tags, "branch=main") != NULL);
    assert(strstr(tags, "mode=full") != NULL);
    /* team and role should be present; too_many may or may not appear depending on extras cap */
    assert(strstr(tags, "team=dev") != NULL);
    assert(strstr(tags, "role=ops") != NULL);
    printf("OK\n");
}

static void test_repo_hash_via_id(void) {
    printf("test_telemetry_cfg.repo_hash... ");
    set_env("GITMIND_METRICS_REPO_TAG", "hash");
    gm_telemetry_cfg_t cfg;
    assert(gm_telemetry_cfg_load(&cfg, NULL).ok);
    gm_repo_id_t id = {.hi = 0x0123456789ABCDEFULL, .lo = 0x0F1E2D3C4B5A6978ULL};
    char tags[128];
    assert(gm_telemetry_build_tags(&cfg, "main", "full", NULL, &id, tags,
                                   sizeof(tags))
               .ok);
    /* Should have repo=<12-hex> tag */
    assert(strstr(tags, "repo=") != NULL);
    /* Still includes branch/mode by default */
    assert(strstr(tags, "branch=main") != NULL);
    assert(strstr(tags, "mode=full") != NULL);
    printf("OK\n");
}

int main(void) {
    test_defaults_branch_mode_only();
    test_extras_and_invalids();
    test_repo_hash_via_id();
    return 0;
}
