/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <roaring/roaring.h>

/* Strongly-typed aliases – clearer in diagnostics than raw typedef */
using gm_bitmap_t   = roaring_bitmap_t;
using gm_bitmap_ptr = gm_bitmap_t *;

/* C23 gives us single-argument static_assert */
static_assert(sizeof(gm_bitmap_t) == sizeof(roaring_bitmap_t));

/* Bitmap file header */
typedef struct {
    char magic[8];    /* "GMCACHE\0" */
    uint32_t version; /* Format version */
    uint32_t flags;   /* Feature flags */
} gm_bitmap_header_t;

/* ─────────  Thin façade with C23 conventions  ───────── */

/* Initialize a new roaring bitmap */
static inline gm_bitmap_ptr gm_bitmap_create(void)
{
    return roaring_bitmap_create();
}

/* Free bitmap */
static inline void gm_bitmap_free(gm_bitmap_ptr bm)
{
    roaring_bitmap_free(bm);
}

/* Add edge ID to bitmap */
[[nodiscard]] static inline bool gm_bitmap_add(gm_bitmap_ptr bm, uint32_t value)
{
    return roaring_bitmap_add(bm, value);
}

/* Check if edge ID is in bitmap */
[[nodiscard]] static inline bool gm_bitmap_contains(const gm_bitmap_t *bm, uint32_t value)
{
    return roaring_bitmap_contains(bm, value);
}

/* Get bitmap cardinality */
static inline uint64_t gm_bitmap_count(const gm_bitmap_t *bm)
{
    return roaring_bitmap_get_cardinality(bm);
}

/* Add multiple edge IDs to bitmap */
void gm_bitmap_add_many(gm_bitmap_ptr bitmap, const uint32_t *edge_ids, size_t count);

/* Get all edge IDs from bitmap */
uint32_t *gm_bitmap_to_array(const gm_bitmap_t *bitmap, size_t *count);

/* Serialize bitmap to buffer with header */
int gm_bitmap_serialize(const gm_bitmap_t *bitmap, uint8_t **buffer, size_t *size);

/* Deserialize bitmap from buffer with header validation */
int gm_bitmap_deserialize(const uint8_t *buffer, size_t size, gm_bitmap_ptr *bitmap);

/* Write bitmap to file */
int gm_bitmap_write_file(const gm_bitmap_t *bitmap, const char *path);

/* Read bitmap from file */
int gm_bitmap_read_file(const char *path, gm_bitmap_ptr *bitmap);

/* Get bitmap statistics */
void gm_bitmap_stats(const gm_bitmap_t *bitmap, uint64_t *cardinality, uint64_t *size_bytes);

/* Bitmap operations */
gm_bitmap_ptr gm_bitmap_or(const gm_bitmap_t *a, const gm_bitmap_t *b);
gm_bitmap_ptr gm_bitmap_and(const gm_bitmap_t *a, const gm_bitmap_t *b);
gm_bitmap_ptr gm_bitmap_xor(const gm_bitmap_t *a, const gm_bitmap_t *b);
gm_bitmap_ptr gm_bitmap_andnot(const gm_bitmap_t *a, const gm_bitmap_t *b);

