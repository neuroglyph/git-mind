/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "gitmind/telemetry/internal/log_format.h"

static void test_text(void) {
    printf("test_log_formatter.text... ");
    const gm_log_kv_t kvs[] = {
        {.key = "event", .value = "rebuild_ok"},
        {.key = "branch", .value = "main"},
        {.key = "mode", .value = "full"},
    };
    char out[128];
    assert(gm_log_format_render_default(kvs, 3, false, out, sizeof(out)).ok);
    assert(strstr(out, "event=rebuild_ok") != NULL);
    assert(strstr(out, "branch=main") != NULL);
    assert(strstr(out, "mode=full") != NULL);
    printf("OK\n");
}

static void test_json(void) {
    printf("test_log_formatter.json... ");
    const gm_log_kv_t kvs[] = {
        {.key = "event", .value = "rebuild_failed"},
        {.key = "branch", .value = "dev"},
        {.key = "mode", .value = "full"},
        {.key = "code", .value = "5"},
    };
    char out[256];
    assert(gm_log_format_render_default(kvs, 4, true, out, sizeof(out)).ok);
    assert(out[0] == '{' && out[strlen(out) - 1] == '}');
    assert(strstr(out, "\"event\":\"rebuild_failed\"") != NULL);
    assert(strstr(out, "\"branch\":\"dev\"") != NULL);
    assert(strstr(out, "\"code\":\"5\"") != NULL);
    printf("OK\n");
}

static void test_json_escaping(void) {
    printf("test_log_formatter.json_escape... ");
    const gm_log_kv_t kvs[] = {
        {.key = "event", .value = "test\"with\"quotes"},
        {.key = "msg", .value = "line1\nline2"},
    };
    char out[256];
    assert(gm_log_format_render_default(kvs, 2, true, out, sizeof(out)).ok);
    assert(strstr(out, "\\\"") != NULL);
    assert(strstr(out, "\\n") != NULL);
    printf("OK\n");
}

int main(void) {
    test_text();
    test_json();
    test_json_escaping();
    return 0;
}
