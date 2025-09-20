/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_CACHE_INTERNAL_EDGE_MAP_H
#define GITMIND_CACHE_INTERNAL_EDGE_MAP_H

#include <stddef.h>
#include <stdint.h>

#include "gitmind/cache/bitmap.h"
#include "gitmind/result.h"
#include "gitmind/types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct gm_edge_map gm_edge_map_t;

typedef int (*gm_edge_map_visit_cb)(const gm_oid_t *oid,
                                    const roaring_bitmap_t *bitmap,
                                    void *userdata);

GM_NODISCARD gm_result_void_t gm_edge_map_create(size_t bucket_count,
                                                 gm_edge_map_t **out_map);
void gm_edge_map_destroy(gm_edge_map_t *map);
GM_NODISCARD gm_result_void_t gm_edge_map_add(gm_edge_map_t *map,
                                              const gm_oid_t *oid,
                                              uint32_t edge_id);
GM_NODISCARD gm_result_void_t gm_edge_map_visit(const gm_edge_map_t *map,
                                                gm_edge_map_visit_cb callback,
                                                void *userdata);

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_CACHE_INTERNAL_EDGE_MAP_H */
