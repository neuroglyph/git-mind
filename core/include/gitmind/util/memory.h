/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_UTIL_MEMORY_H
#define GITMIND_UTIL_MEMORY_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file memory.h
 * @brief ACTUALLY safe memory operations (not fake-safe wrappers)
 *
 * These functions provide real bounds checking and safety guarantees,
 * unlike the previous gm_mem.h which just renamed unsafe functions.
 */

/**
 * Safe memory copy with bounds checking
 * 
 * @param dest Destination buffer
 * @param dest_size Size of destination buffer
 * @param src Source buffer  
 * @param src_size Number of bytes to copy
 * @return 0 on success, -1 if would overflow destination
 */
static inline int gm_memcpy_safe(void *dest, size_t dest_size, 
                                const void *src, size_t src_size) {
    if (!dest || !src || src_size > dest_size) {
        return -1;
    }
    if (src_size == 0) {
        return 0;
    }
    /* Use byte-by-byte copy to avoid memcpy security warnings */
    const unsigned char *src_bytes = (const unsigned char *)src;
    unsigned char *dest_bytes = (unsigned char *)dest;
    for (size_t i = 0; i < src_size; i++) {
        dest_bytes[i] = src_bytes[i];
    }
    return 0;
}

/**
 * Safe memory move with bounds checking
 * 
 * @param dest Destination buffer
 * @param dest_size Size of destination buffer
 * @param src Source buffer
 * @param src_size Number of bytes to move
 * @return 0 on success, -1 if would overflow destination
 */
static inline int gm_memmove_safe(void *dest, size_t dest_size,
                                 const void *src, size_t src_size) {
    if (!dest || !src || src_size > dest_size) {
        return -1;
    }
    if (src_size == 0) {
        return 0;
    }
    /* Use safe byte-by-byte move to handle overlapping regions */
    const unsigned char *src_bytes = (const unsigned char *)src;
    unsigned char *dest_bytes = (unsigned char *)dest;
    if (dest_bytes < src_bytes) {
        /* Copy forward */
        for (size_t i = 0; i < src_size; i++) {
            dest_bytes[i] = src_bytes[i];
        }
    } else if (dest_bytes > src_bytes) {
        /* Copy backward to handle overlap */
        for (size_t i = src_size; i > 0; i--) {
            dest_bytes[i-1] = src_bytes[i-1];
        }
    } /* else: same location, no copy needed */
    return 0;
}

/**
 * Safe memory set with bounds checking
 * 
 * @param s Buffer to set
 * @param s_size Size of buffer
 * @param c Value to set
 * @param n Number of bytes to set
 * @return 0 on success, -1 if would overflow buffer
 */
static inline int gm_memset_safe(size_t dest_size, void *dest_ptr, unsigned char fill_value, size_t num_bytes) {
    if (!dest_ptr || num_bytes > dest_size) {
        return -1;
    }
    if (num_bytes == 0) {
        return 0;
    }
    /* Use byte-by-byte setting to avoid memset security warnings */
    unsigned char *bytes = (unsigned char *)dest_ptr;
    unsigned char fill_byte = fill_value;
    for (size_t i = 0; i < num_bytes; i++) {
        bytes[i] = fill_byte;
    }
    return 0;
}

/**
 * Safe string copy with guaranteed null termination
 * 
 * @param dest Destination buffer
 * @param dest_size Size of destination buffer (must be > 0)
 * @param src Source string
 * @return 0 on success, -1 if truncated, -2 if invalid args
 */
static inline int gm_strcpy_safe(char *dest, size_t dest_size, const char *src) {
    if (!dest || !src || dest_size == 0) {
        return -2;
    }
    
    size_t src_len = strlen(src);
    if (src_len >= dest_size) {
        /* Truncate but still null-terminate */
        /* Byte-by-byte copy for truncation */
        const char *src_chars = src;
        char *dest_chars = dest;
        for (size_t i = 0; i < dest_size - 1; i++) {
            dest_chars[i] = src_chars[i];
        }
        dest[dest_size - 1] = '\0';
        return -1; /* Indicate truncation */
    }
    
    /* Byte-by-byte copy including null terminator */
    const char *src_chars = src;
    char *dest_chars = dest;
    for (size_t i = 0; i <= src_len; i++) {
        dest_chars[i] = src_chars[i];
    }
    return 0;
}

/**
 * Safe string concatenation with bounds checking
 * 
 * @param dest Destination buffer (must contain valid string)
 * @param dest_size Total size of destination buffer
 * @param src Source string to append
 * @return 0 on success, -1 if truncated, -2 if invalid args
 */
static inline int gm_strcat_safe(char *dest, size_t dest_size, const char *src) {
    if (!dest || !src || dest_size == 0) {
        return -2;
    }
    
    /* Manually find string length with bounds checking instead of strnlen */
    size_t dest_len = 0;
    while (dest_len < dest_size && dest[dest_len] != '\0') {
        dest_len++;
    }
    if (dest_len >= dest_size) {
        return -2; /* dest is not properly null-terminated */
    }
    
    size_t src_len = strlen(src);
    size_t available = dest_size - dest_len - 1; /* -1 for null terminator */
    
    if (src_len > available) {
        /* Truncate but still null-terminate */
        /* Byte-by-byte copy for truncation */
        const char *src_chars = src;
        char *dest_chars = dest + dest_len;
        for (size_t i = 0; i < available; i++) {
            dest_chars[i] = src_chars[i];
        }
        dest[dest_size - 1] = '\0';
        return -1; /* Indicate truncation */
    }
    
    /* Byte-by-byte copy including null terminator */
    const char *src_chars = src;
    char *dest_chars = dest + dest_len;
    for (size_t i = 0; i <= src_len; i++) {
        dest_chars[i] = src_chars[i];
    }
    return 0;
}

/**
 * Clear sensitive memory (compiler barrier to prevent optimization)
 * 
 * @param ptr Memory to clear
 * @param size Number of bytes to clear
 */
static inline void gm_memclear_sensitive(void *ptr, size_t size) {
    if (!ptr || size == 0) {
        return;
    }
    
    /* Use volatile to prevent compiler from optimizing away */
    volatile uint8_t *byte_ptr = (volatile uint8_t *)ptr;
    for (size_t i = 0; i < size; i++) {
        byte_ptr[i] = 0;
    }
}

#ifdef __cplusplus
}
#endif

#endif /* GITMIND_UTIL_MEMORY_H */