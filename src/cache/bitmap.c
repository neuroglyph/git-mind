/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "bitmap.h"
#include "../../include/gitmind.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

/* Magic number and version */
#define BITMAP_MAGIC "GMCACHE\0"
#define BITMAP_VERSION 1

roaring_bitmap_t* gm_bitmap_create(void) {
    return roaring_bitmap_create();
}

void gm_bitmap_add(roaring_bitmap_t* bitmap, uint32_t edge_id) {
    roaring_bitmap_add(bitmap, edge_id);
}

void gm_bitmap_add_many(roaring_bitmap_t* bitmap, const uint32_t* edge_ids, size_t count) {
    roaring_bitmap_add_many(bitmap, count, edge_ids);
}

bool gm_bitmap_contains(const roaring_bitmap_t* bitmap, uint32_t edge_id) {
    return roaring_bitmap_contains(bitmap, edge_id);
}

uint32_t* gm_bitmap_to_array(const roaring_bitmap_t* bitmap, size_t* count) {
    uint64_t cardinality = roaring_bitmap_get_cardinality(bitmap);
    if (cardinality == 0) {
        *count = 0;
        return NULL;
    }
    
    uint32_t* array = malloc(cardinality * sizeof(uint32_t));
    if (!array) {
        *count = 0;
        return NULL;
    }
    
    roaring_bitmap_to_uint32_array(bitmap, array);
    *count = cardinality;
    return array;
}

int gm_bitmap_serialize(const roaring_bitmap_t* bitmap, uint8_t** buffer, size_t* size) {
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
    gm_bitmap_header_t header;
    memcpy(header.magic, BITMAP_MAGIC, 8);
    header.version = BITMAP_VERSION;
    header.flags = 0;
    memcpy(*buffer, &header, header_size);
    
    /* Serialize bitmap */
    size_t written = roaring_bitmap_serialize(bitmap, (char*)(*buffer + header_size));
    if (written != bitmap_size) {
        free(*buffer);
        *buffer = NULL;
        return GM_ERROR;
    }
    
    *size = total_size;
    return GM_OK;
}

int gm_bitmap_deserialize(const uint8_t* buffer, size_t size, roaring_bitmap_t** bitmap) {
    size_t header_size = sizeof(gm_bitmap_header_t);
    
    /* Validate size */
    if (size < header_size) {
        return GM_ERROR;
    }
    
    /* Read and validate header */
    gm_bitmap_header_t header;
    memcpy(&header, buffer, header_size);
    
    if (memcmp(header.magic, BITMAP_MAGIC, 8) != 0) {
        return GM_ERROR;
    }
    
    if (header.version != BITMAP_VERSION) {
        return GM_ERROR;
    }
    
    /* Deserialize bitmap */
    *bitmap = roaring_bitmap_deserialize((const char*)(buffer + header_size));
    if (!*bitmap) {
        return GM_ERROR;
    }
    
    return GM_OK;
}

int gm_bitmap_write_file(const roaring_bitmap_t* bitmap, const char* path) {
    uint8_t* buffer = NULL;
    size_t size = 0;
    
    /* Serialize to buffer */
    int rc = gm_bitmap_serialize(bitmap, &buffer, &size);
    if (rc != GM_OK) {
        return rc;
    }
    
    /* Write to file */
    FILE* f = fopen(path, "wb");
    if (!f) {
        free(buffer);
        return GM_IO_ERROR;
    }
    
    size_t written = fwrite(buffer, 1, size, f);
    int save_errno = errno;
    fclose(f);
    free(buffer);
    
    if (written != size) {
        errno = save_errno;
        return GM_IO_ERROR;
    }
    
    return GM_OK;
}

int gm_bitmap_read_file(const char* path, roaring_bitmap_t** bitmap) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        return GM_NOT_FOUND;
    }
    
    /* Get file size */
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (file_size < 0 || file_size > INT32_MAX) {
        fclose(f);
        return GM_ERROR;
    }
    
    /* Read file */
    uint8_t* buffer = malloc(file_size);
    if (!buffer) {
        fclose(f);
        return GM_NO_MEMORY;
    }
    
    size_t read = fread(buffer, 1, file_size, f);
    fclose(f);
    
    if (read != (size_t)file_size) {
        free(buffer);
        return GM_IO_ERROR;
    }
    
    /* Deserialize */
    int rc = gm_bitmap_deserialize(buffer, file_size, bitmap);
    free(buffer);
    return rc;
}

void gm_bitmap_free(roaring_bitmap_t* bitmap) {
    if (bitmap) {
        roaring_bitmap_free(bitmap);
    }
}

void gm_bitmap_stats(const roaring_bitmap_t* bitmap, uint64_t* cardinality,
                    uint64_t* size_bytes) {
    if (cardinality) {
        *cardinality = roaring_bitmap_get_cardinality(bitmap);
    }
    if (size_bytes) {
        *size_bytes = roaring_bitmap_size_in_bytes(bitmap);
    }
}

/* Bitmap operations */
roaring_bitmap_t* gm_bitmap_or(const roaring_bitmap_t* a, const roaring_bitmap_t* b) {
    return roaring_bitmap_or(a, b);
}

roaring_bitmap_t* gm_bitmap_and(const roaring_bitmap_t* a, const roaring_bitmap_t* b) {
    return roaring_bitmap_and(a, b);
}

roaring_bitmap_t* gm_bitmap_xor(const roaring_bitmap_t* a, const roaring_bitmap_t* b) {
    return roaring_bitmap_xor(a, b);
}

roaring_bitmap_t* gm_bitmap_andnot(const roaring_bitmap_t* a, const roaring_bitmap_t* b) {
    return roaring_bitmap_andnot(a, b);
}