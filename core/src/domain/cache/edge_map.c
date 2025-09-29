/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/cache/internal/edge_map.h"

#include <stdint.h>
#include <stdlib.h>

#include "gitmind/error.h"
#include "gitmind/util/memory.h"
#include "gitmind/util/oid.h"

struct gm_edge_map_entry {
    gm_oid_t oid;
    roaring_bitmap_t *bitmap;
    struct gm_edge_map_entry *next;
};

struct gm_edge_map {
    struct gm_edge_map_entry **buckets;
    size_t bucket_count;
};

static const uint32_t GM_CACHE_OID_HASH_INIT = 0x9E3779B9U;
static const uint32_t GM_CACHE_OID_HASH_MIX = 0x85EBCA6BU;
static const uint32_t GM_CACHE_OID_HASH_FINAL = 0x7FEB352DU;
static const unsigned GM_CACHE_HASH_SHIFT1 = 13U;
static const unsigned GM_CACHE_HASH_SHIFT2 = 16U;
static const unsigned GM_CACHE_HASH_SHIFT3 = 15U;

static size_t gm_edge_map_hash(const gm_oid_t *oid, size_t bucket_count) {
    const uint8_t *raw_bytes = (const uint8_t *)oid->id;
    uint32_t hash_value = GM_CACHE_OID_HASH_INIT;

    for (size_t index = 0; index < GM_OID_RAWSZ; ++index) {
        hash_value ^= raw_bytes[index];
        hash_value *= GM_CACHE_OID_HASH_MIX;
        hash_value ^= (hash_value >> GM_CACHE_HASH_SHIFT1);
    }

    hash_value ^= (hash_value >> GM_CACHE_HASH_SHIFT2);
    hash_value *= GM_CACHE_OID_HASH_FINAL;
    hash_value ^= (hash_value >> GM_CACHE_HASH_SHIFT3);

    if (bucket_count == 0U) {
        return 0U;
    }

    return (size_t)(hash_value % (uint32_t)bucket_count);
}

static struct gm_edge_map_entry *gm_edge_map_entry_create(const gm_oid_t *oid,
                                                          uint32_t edge_id) {
    struct gm_edge_map_entry *entry =
        calloc(1, sizeof(struct gm_edge_map_entry));
    if (entry == NULL) {
        return NULL;
    }

    entry->oid = *oid;
    entry->bitmap = gm_bitmap_create();
    if (entry->bitmap == NULL) {
        free(entry);
        return NULL;
    }

    gm_bitmap_add(entry->bitmap, edge_id);
    return entry;
}

static void gm_edge_map_entry_destroy(struct gm_edge_map_entry *entry) {
    if (entry == NULL) {
        return;
    }
    gm_bitmap_free(entry->bitmap);
    free(entry);
}

gm_result_void_t gm_edge_map_create(size_t bucket_count, gm_edge_map_t **out_map) {
    if (out_map == NULL) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, "edge map output pointer required"));
    }

    gm_edge_map_t *map = calloc(1, sizeof(gm_edge_map_t));
    if (map == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_OUT_OF_MEMORY, "edge map alloc failed"));
    }

    map->buckets = calloc(bucket_count, sizeof(*map->buckets));
    if (map->buckets == NULL) {
        free(map);
        return gm_err_void(GM_ERROR(GM_ERR_OUT_OF_MEMORY, "edge map buckets alloc failed"));
    }

    map->bucket_count = bucket_count;
    *out_map = map;
    return gm_ok_void();
}

void gm_edge_map_destroy(gm_edge_map_t *map) {
    if (map == NULL) {
        return;
    }

    for (size_t bucket = 0; bucket < map->bucket_count; ++bucket) {
        struct gm_edge_map_entry *entry = map->buckets[bucket];
        while (entry != NULL) {
            struct gm_edge_map_entry *next = entry->next;
            gm_edge_map_entry_destroy(entry);
            entry = next;
        }
    }

    free(map->buckets);
    free(map);
}

gm_result_void_t gm_edge_map_add(gm_edge_map_t *map, const gm_oid_t *oid,
                                  uint32_t edge_id) {
    if (map == NULL || oid == NULL) {
        return gm_err_void(
            GM_ERROR(GM_ERR_INVALID_ARGUMENT, "edge map add requires map/oid"));
    }

    size_t bucket_index = gm_edge_map_hash(oid, map->bucket_count);
    struct gm_edge_map_entry *entry = map->buckets[bucket_index];
    while (entry != NULL) {
        if (gm_oid_equal(&entry->oid, oid)) {
            gm_bitmap_add(entry->bitmap, edge_id);
            return gm_ok_void();
        }
        entry = entry->next;
    }

    struct gm_edge_map_entry *new_entry =
        gm_edge_map_entry_create(oid, edge_id);
    if (new_entry == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_OUT_OF_MEMORY, "edge map entry alloc failed"));
    }

    new_entry->next = map->buckets[bucket_index];
    map->buckets[bucket_index] = new_entry;
    return gm_ok_void();
}

gm_result_void_t gm_edge_map_visit(const gm_edge_map_t *map,
                                    gm_edge_map_visit_cb callback,
                                    void *userdata) {
    if (map == NULL || callback == NULL) {
        return gm_err_void(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                    "edge map visit requires map/callback"));
    }

    for (size_t bucket = 0; bucket < map->bucket_count; ++bucket) {
        const struct gm_edge_map_entry *entry = map->buckets[bucket];
        while (entry != NULL) {
            int result = callback(&entry->oid, entry->bitmap, userdata);
            if (result != GM_OK) {
                return gm_err_void(
                    GM_ERROR(result, "edge map visitor returned error"));
            }
            entry = entry->next;
        }
    }

    return gm_ok_void();
}
