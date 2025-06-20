/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GM_BITMAP_H
#define GM_BITMAP_H

#include <roaring.h>
#include <stdbool.h>
#include <stdint.h>

/* Bitmap file header */
typedef struct {
    char magic[8];    /* "GMCACHE\0" */
    uint32_t version; /* Format version */
    uint32_t flags;   /* Feature flags */
} gm_bitmap_header_t;

/* Initialize a new roaring bitmap */
roaring_bitmap_t *gm_bitmap_create(void);

/* Add edge ID to bitmap */
void gm_bitmap_add(roaring_bitmap_t *bitmap, uint32_t edge_id);

/* Add multiple edge IDs to bitmap */
void gm_bitmap_add_many(roaring_bitmap_t *bitmap, const uint32_t *edge_ids,
                        size_t count);

/* Check if edge ID is in bitmap */
bool gm_bitmap_contains(const roaring_bitmap_t *bitmap, uint32_t edge_id);

/* Get all edge IDs from bitmap */
uint32_t *gm_bitmap_to_array(const roaring_bitmap_t *bitmap, size_t *count);

/* Serialize bitmap to buffer with header */
int gm_bitmap_serialize(const roaring_bitmap_t *bitmap, uint8_t **buffer,
                        size_t *size);

/* Deserialize bitmap from buffer with header validation */
int gm_bitmap_deserialize(const uint8_t *buffer, size_t size,
                          roaring_bitmap_t **bitmap);

/* Write bitmap to file */
int gm_bitmap_write_file(const roaring_bitmap_t *bitmap, const char *path);

/* Read bitmap from file */
int gm_bitmap_read_file(const char *path, roaring_bitmap_t **bitmap);

/* Free bitmap */
void gm_bitmap_free(roaring_bitmap_t *bitmap);

/* Get bitmap statistics */
void gm_bitmap_stats(const roaring_bitmap_t *bitmap, uint64_t *cardinality,
                     uint64_t *size_bytes);

/* Bitmap operations */
roaring_bitmap_t *gm_bitmap_or(const roaring_bitmap_t *a,
                               const roaring_bitmap_t *b);
roaring_bitmap_t *gm_bitmap_and(const roaring_bitmap_t *a,
                                const roaring_bitmap_t *b);
roaring_bitmap_t *gm_bitmap_xor(const roaring_bitmap_t *a,
                                const roaring_bitmap_t *b);
roaring_bitmap_t *gm_bitmap_andnot(const roaring_bitmap_t *a,
                                   const roaring_bitmap_t *b);

#endif /* GM_BITMAP_H */