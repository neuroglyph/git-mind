/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/cache/bitmap.h"

#include <errno.h>
#include <roaring/roaring.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gitmind/constants.h"
#include "gitmind/error.h"

/* Magic number and version */
#define BITMAP_MAGIC "GMCACHE\0"
#define BITMAP_VERSION 1

void gm_bitmap_add_many(gm_bitmap_ptr bitmap, const uint32_t *edge_ids,
                        size_t count) {
    roaring_bitmap_add_many(bitmap, count, edge_ids);
}

uint32_t *gm_bitmap_to_array(const gm_bitmap_t *bitmap, size_t *count) {
    uint64_t cardinality = roaring_bitmap_get_cardinality(bitmap);
    if (cardinality == 0) {
        *count = 0;
        return NULL;
    }

    uint32_t *array = malloc(cardinality * sizeof(uint32_t));
    if (!array) {
        *count = 0;
        return NULL;
    }

    roaring_bitmap_to_uint32_array(bitmap, array);
    *count = cardinality;
    return array;
}

int gm_bitmap_serialize(const gm_bitmap_t *bitmap, uint8_t **buffer,
                        size_t *size) {
    /* Calculate sizes */
    size_t header_size = sizeof(gm_bitmap_header_t);
    size_t bitmap_size = roaring_bitmap_size_in_bytes(bitmap);
    size_t total_size = header_size + bitmap_size;

    /* Allocate buffer */
    *buffer = malloc(total_size);
    if (!*buffer) {
        return GM_NO_MEMORY;
    }

    /* Write header */
    gm_bitmap_header_t header = {
        .magic = BITMAP_MAGIC,
        .version = BITMAP_VERSION,
        .flags = 0,
    };
    static_assert(sizeof(header) == header_size, "header size drift");
    *(gm_bitmap_header_t *)*buffer = header;

    /* Serialize bitmap */
    size_t written =
        roaring_bitmap_serialize(bitmap, (char *)(*buffer + header_size));
    if (written != bitmap_size) {
        free(*buffer);
        *buffer = NULL;
        return GM_ERR_UNKNOWN;
    }

    *size = total_size;
    return GM_OK;
}

int gm_bitmap_deserialize(const uint8_t *buffer, size_t size,
                          gm_bitmap_ptr *bitmap) {
    size_t header_size = sizeof(gm_bitmap_header_t);

    /* Validate size */
    if (size < header_size) {
        return GM_ERR_UNKNOWN;
    }

    /* Read and validate header */
    const gm_bitmap_header_t *hdr = (const gm_bitmap_header_t *)buffer;
    if (memcmp(hdr->magic, BITMAP_MAGIC, sizeof hdr->magic) != 0) {
        return GM_ERR_UNKNOWN;
    }

    if (hdr->version != BITMAP_VERSION) {
        return GM_ERR_UNKNOWN;
    }

    /* Deserialize bitmap */
    *bitmap = roaring_bitmap_deserialize((const char *)(buffer + header_size));
    if (!*bitmap) {
        return GM_ERR_UNKNOWN;
    }

    return GM_OK;
}

int gm_bitmap_write_file(const gm_bitmap_t *bitmap, const char *path) {
    uint8_t *buffer = NULL;
    size_t size = 0;

    /* Serialize to buffer */
    int result = gm_bitmap_serialize(bitmap, &buffer, &size);
    if (result != GM_OK) {
        return result;
    }

    /* Write to file */
    FILE *file = fopen(path, "wb");
    if (!file) {
        free(buffer);
        return GM_IO_ERROR;
    }

    size_t written = fwrite(buffer, 1, size, file);
    int save_errno = errno;
    if (fclose(file) != 0) {
        free(buffer);
        return GM_IO_ERROR;
    }
    free(buffer);

    if (written != size) {
        errno = save_errno;
        return GM_IO_ERROR;
    }

    return GM_OK;
}

int gm_bitmap_read_file(const char *path, gm_bitmap_ptr *bitmap) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        return GM_NOT_FOUND;
    }

    /* Get file size */
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return GM_IO_ERROR;
    }
    long file_size = ftell(file);
    if (file_size < 0) {
        fclose(file);
        return GM_IO_ERROR;
    }
    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return GM_IO_ERROR;
    }

    if (file_size < 0 || file_size > INT32_MAX) {
        fclose(file);
        return GM_ERR_UNKNOWN;
    }

    size_t buffer_size = (size_t)file_size;

    /* Read file */
    uint8_t *buffer = malloc(buffer_size);
    if (!buffer) {
        fclose(file);
        return GM_NO_MEMORY;
    }

    size_t read = fread(buffer, 1, buffer_size, file);
    if (fclose(file) != 0) {
        free(buffer);
        return GM_IO_ERROR;
    }

    if (read != buffer_size) {
        free(buffer);
        return GM_IO_ERROR;
    }

    /* Deserialize */
    int result = gm_bitmap_deserialize(buffer, buffer_size, bitmap);
    free(buffer);
    return result;
}

void gm_bitmap_stats(const gm_bitmap_t *bitmap, uint64_t *cardinality,
                     uint64_t *size_bytes) {
    /* Parameters are naturally similar - differentiate by names */
    if (cardinality) {
        *cardinality = roaring_bitmap_get_cardinality(bitmap);
    }
    if (size_bytes) {
        *size_bytes = roaring_bitmap_size_in_bytes(bitmap);
    }
}

/* Bitmap operations */
gm_bitmap_ptr gm_bitmap_or(const gm_bitmap_t *left,
                           const gm_bitmap_t *right) {
    return roaring_bitmap_or(left, right);
}

gm_bitmap_ptr gm_bitmap_and(const gm_bitmap_t *left,
                            const gm_bitmap_t *right) {
    return roaring_bitmap_and(left, right);
}

gm_bitmap_ptr gm_bitmap_xor(const gm_bitmap_t *left,
                            const gm_bitmap_t *right) {
    return roaring_bitmap_xor(left, right);
}

gm_bitmap_ptr gm_bitmap_andnot(const gm_bitmap_t *left,
                               const gm_bitmap_t *right) {
    return roaring_bitmap_andnot(left, right);
}
