/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#ifndef GITMIND_TYPES_ID_CONTEXT_H
#define GITMIND_TYPES_ID_CONTEXT_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>
#include "gitmind/result.h"
#include <sodium.h>

/**
 * @brief Thread-safe context for ID operations
 * 
 * This context encapsulates all mutable state required for ID operations,
 * eliminating global variables and ensuring thread safety.
 */
typedef struct gm_id_context {
    /* SipHash key for hash operations */
    uint8_t siphash_key[crypto_shorthash_siphash24_KEYBYTES];
    
    /* Initialization state */
    bool initialized;
    
    /* Mutex for thread-safe initialization */
    pthread_mutex_t init_mutex;
    
    /* Reference count for cleanup */
    size_t ref_count;
    
    /* Mutex for reference counting */
    pthread_mutex_t ref_mutex;
} gm_id_context_t;

/* Result type for context operations */
GM_RESULT_DEF(gm_result_id_context, gm_id_context_t*);

/**
 * @brief Create a new ID context
 * 
 * Creates a new context with randomly generated SipHash key.
 * The context is reference counted and thread-safe.
 * 
 * @return Result containing the new context or an error
 */
gm_result_id_context gm_id_context_create(void);

/**
 * @brief Increment reference count on context
 * 
 * @param ctx The context to reference
 * @return Result indicating success or failure
 */
gm_result_void gm_id_context_ref(gm_id_context_t* ctx);

/**
 * @brief Decrement reference count and destroy if zero
 * 
 * @param ctx The context to unreference
 * @return Result indicating success or failure
 */
gm_result_void gm_id_context_unref(gm_id_context_t* ctx);

/**
 * @brief Get the global default context
 * 
 * Returns a reference to the global default context, creating it
 * if necessary. The caller should NOT unref this context.
 * 
 * @return The global context or NULL on error
 */
gm_id_context_t* gm_id_context_get_default(void);

/**
 * @brief Set a custom default context
 * 
 * Replaces the global default context with a custom one.
 * Takes ownership of the provided context.
 * 
 * @param ctx The context to set as default (transfers ownership)
 * @return Result indicating success or failure
 */
gm_result_void gm_id_context_set_default(gm_id_context_t* ctx);

/**
 * @brief Hash an ID using the given context
 * 
 * Thread-safe hash function that uses the context's SipHash key.
 * 
 * @param ctx The context to use for hashing
 * @param id The ID to hash
 * @return Result containing the hash or an error
 */
gm_result_u32 gm_id_hash_with_context(gm_id_context_t* ctx, gm_id_t id);

#endif /* GITMIND_TYPES_ID_CONTEXT_H */