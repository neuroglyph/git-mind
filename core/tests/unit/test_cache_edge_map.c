/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/cache/internal/edge_map.h"

#include <assert.h>
#include <stddef.h>

#include "gitmind/cache/bitmap.h"
#include "gitmind/error.h"
#include "gitmind/util/oid.h"

static gm_oid_t make_oid(const char *hex) {
    gm_oid_t oid = {{0}};
    assert(gm_oid_from_hex(&oid, hex) == GM_OK);
    return oid;
}

static void dispose_error(gm_result_void_t result) {
    if (result.ok) {
        return;
    }
    if (result.u.err != NULL) {
        gm_error_free(result.u.err);
    }
    assert(result.ok && "unexpected gm_result failure");
}

typedef struct {
    gm_oid_t expected[2];
    uint32_t counts[2];
    size_t seen;
} visit_ctx_t;

static int visit_and_collect(const gm_oid_t *oid, const roaring_bitmap_t *bitmap,
                             void *userdata) {
    visit_ctx_t *ctx = (visit_ctx_t *)userdata;
    for (size_t index = 0; index < 2; ++index) {
        if (gm_oid_equal(&ctx->expected[index], oid)) {
            ctx->counts[index] = (uint32_t)gm_bitmap_count(bitmap);
            ++ctx->seen;
            return GM_OK;
        }
    }
    return GM_ERR_INVALID_ARGUMENT;
}

static int fail_callback(const gm_oid_t *oid, const roaring_bitmap_t *bitmap,
                         void *userdata) {
    (void)oid;
    (void)bitmap;
    (void)userdata;
    return GM_ERR_INVALID_ARGUMENT;
}

static void test_edge_map_basic(void) {
    gm_edge_map_t *map = NULL;
    gm_result_void_t create_result = gm_edge_map_create(8, &map);
    dispose_error(create_result);

    gm_oid_t oid_a = make_oid("0123456789abcdef0123456789abcdef01234567");
    gm_oid_t oid_b = make_oid("89abcdef012345670123456789abcdef01234567");

    dispose_error(gm_edge_map_add(map, &oid_a, 1));
    dispose_error(gm_edge_map_add(map, &oid_a, 2));
    dispose_error(gm_edge_map_add(map, &oid_b, 42));

    visit_ctx_t ctx = {
        .expected = {oid_a, oid_b},
        .counts = {0, 0},
        .seen = 0,
    };

    gm_result_void_t visit_result = gm_edge_map_visit(map, visit_and_collect, &ctx);
    dispose_error(visit_result);

    assert(ctx.seen == 2);
    assert(ctx.counts[0] == 2);
    assert(ctx.counts[1] == 1);

    gm_edge_map_destroy(map);
}

static void test_edge_map_visit_error(void) {
    gm_edge_map_t *map = NULL;
    dispose_error(gm_edge_map_create(4, &map));

    gm_oid_t oid = make_oid("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
    dispose_error(gm_edge_map_add(map, &oid, 7));

    gm_result_void_t visit_result =
        gm_edge_map_visit(map, fail_callback, NULL);
    assert(!visit_result.ok);
    gm_error_free(visit_result.u.err);

    gm_edge_map_destroy(map);
}

int main(void) {
    test_edge_map_basic();
    test_edge_map_visit_error();
    return 0;
}
