/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#pragma once
#include <stddef.h>
#include <stdint.h>
#include <roaring/roaring.h>

/* Strongly-typed aliases – clearer in diagnostics than raw typedef */
typedef roaring_bitmap_t gm_bitmap_t;
typedef gm_bitmap_t *gm_bitmap_ptr;

/* C23 gives us single-argument static_assert and nullptr */
static_assert(sizeof(gm_bitmap_t) == sizeof(roaring_bitmap_t));

/* Bitmap file header */
typedef struct {
    char magic[8];    /* "GMCACHE\0" */
    uint32_t version; /* Format version */
    uint32_t flags;   /* Feature flags */
} gm_bitmap_header_t;

/* ─────────  Thin façade  ───────── */

static inline gm_bitmap_ptr gm_bitmap_create(void)
{
    return roaring_bitmap_create();
}

static inline void gm_bitmap_free(gm_bitmap_ptr bitmap)
{
    roaring_bitmap_free(bitmap);
}

static inline void gm_bitmap_add(gm_bitmap_ptr bitmap, uint32_t value)
{
    roaring_bitmap_add(bitmap, value);
}

[[nodiscard]] static inline bool gm_bitmap_contains(const gm_bitmap_t *bitmap,
                                                    uint32_t value)
{
    return roaring_bitmap_contains(bitmap, value);
}

static inline uint64_t gm_bitmap_count(const gm_bitmap_t *bitmap)
{
    return roaring_bitmap_get_cardinality(bitmap);
}

/* Non-inline functions for complex operations */
void gm_bitmap_add_many(gm_bitmap_ptr bitmap, const uint32_t *edge_ids, size_t count);

uint32_t *gm_bitmap_to_array(const gm_bitmap_t *bitmap, size_t *count);

int gm_bitmap_serialize(const gm_bitmap_t *bitmap, uint8_t **buffer, size_t *size);

int gm_bitmap_deserialize(const uint8_t *buffer, size_t size, gm_bitmap_ptr *bitmap);

int gm_bitmap_write_file(const gm_bitmap_t *bitmap, const char *path);

int gm_bitmap_read_file(const char *path, gm_bitmap_ptr *bitmap);

void gm_bitmap_stats(const gm_bitmap_t *bitmap, uint64_t *cardinality, uint64_t *size_bytes);

/* Bitmap operations */
gm_bitmap_ptr gm_bitmap_or(const gm_bitmap_t *left, const gm_bitmap_t *right);
gm_bitmap_ptr gm_bitmap_and(const gm_bitmap_t *left, const gm_bitmap_t *right);
gm_bitmap_ptr gm_bitmap_xor(const gm_bitmap_t *left, const gm_bitmap_t *right);
gm_bitmap_ptr gm_bitmap_andnot(const gm_bitmap_t *left, const gm_bitmap_t *right);
