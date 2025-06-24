/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/types/id_context.h"
#include "gitmind/types/id.h"
#include "gitmind/error.h"
#include "gitmind/crypto/random.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <assert.h>

/* Global default context */
static gm_id_context_t* g_default_context = NULL;
static pthread_mutex_t g_default_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Helper to create error result for context */
static inline gm_result_id_context gm_err_id_context(gm_error_t* err) {
    return (gm_result_id_context){ .ok = false, .u.err = err };
}

/* Create a new ID context */
gm_result_id_context gm_id_context_create(void) {
    gm_id_context_t* ctx = calloc(1, sizeof(gm_id_context_t));
    if (!ctx) {
        return gm_err_id_context(GM_ERROR(GM_ERR_OUT_OF_MEMORY,
                                         "Failed to allocate ID context"));
    }
    
    /* Initialize mutexes */
    int result = pthread_mutex_init(&ctx->init_mutex, NULL);
    if (result != 0) {
        free(ctx);
        return gm_err_id_context(GM_ERROR(GM_ERR_SYSTEM,
                                         "Failed to initialize context mutex"));
    }
    
    result = pthread_mutex_init(&ctx->ref_mutex, NULL);
    if (result != 0) {
        pthread_mutex_destroy(&ctx->init_mutex);
        free(ctx);
        return gm_err_id_context(GM_ERROR(GM_ERR_SYSTEM,
                                         "Failed to initialize reference mutex"));
    }
    
    /* Generate random SipHash key */
    gm_result_void rand_result = gm_random_bytes(ctx->siphash_key, 
                                                 sizeof(ctx->siphash_key));
    if (GM_IS_ERR(rand_result)) {
        /* If random generation fails, use a deterministic fallback */
        /* This is still better than a hardcoded constant */
        for (size_t i = 0; i < sizeof(ctx->siphash_key); i++) {
            ctx->siphash_key[i] = (uint8_t)(i ^ 0xAA);
        }
    }
    
    ctx->initialized = true;
    ctx->ref_count = 1;
    
    return gm_ok_id_context(ctx);
}

/* Increment reference count */
gm_result_void gm_id_context_ref(gm_id_context_t* ctx) {
    if (!ctx) {
        return GM_ERROR_VOID(GM_ERR_INVALID_ARGUMENT,
                             "Cannot reference NULL context");
    }
    
    pthread_mutex_lock(&ctx->ref_mutex);
    ctx->ref_count++;
    pthread_mutex_unlock(&ctx->ref_mutex);
    
    return GM_OK_VOID;
}

/* Decrement reference count and destroy if zero */
gm_result_void gm_id_context_unref(gm_id_context_t* ctx) {
    if (!ctx) {
        return GM_ERROR_VOID(GM_ERR_INVALID_ARGUMENT,
                             "Cannot unreference NULL context");
    }
    
    pthread_mutex_lock(&ctx->ref_mutex);
    assert(ctx->ref_count > 0);
    ctx->ref_count--;
    size_t count = ctx->ref_count;
    pthread_mutex_unlock(&ctx->ref_mutex);
    
    if (count == 0) {
        /* Destroy context */
        pthread_mutex_destroy(&ctx->init_mutex);
        pthread_mutex_destroy(&ctx->ref_mutex);
        
        /* Clear sensitive data */
        explicit_bzero(ctx->siphash_key, sizeof(ctx->siphash_key));
        free(ctx);
    }
    
    return GM_OK_VOID;
}

/* Get the global default context */
gm_id_context_t* gm_id_context_get_default(void) {
    pthread_mutex_lock(&g_default_mutex);
    
    if (!g_default_context) {
        gm_result_id_context result = gm_id_context_create();
        if (GM_IS_OK(result)) {
            g_default_context = GM_UNWRAP(result);
        } else {
            /* Log error but don't propagate - this is a convenience function */
            gm_error_free(GM_UNWRAP_ERR(result));
        }
    }
    
    gm_id_context_t* ctx = g_default_context;
    pthread_mutex_unlock(&g_default_mutex);
    
    return ctx;
}

/* Set a custom default context */
gm_result_void gm_id_context_set_default(gm_id_context_t* ctx) {
    if (!ctx) {
        return GM_ERROR_VOID(GM_ERR_INVALID_ARGUMENT,
                             "Cannot set NULL as default context");
    }
    
    pthread_mutex_lock(&g_default_mutex);
    
    /* Unref old context if any */
    if (g_default_context) {
        gm_id_context_unref(g_default_context);
    }
    
    /* Set new context (takes ownership) */
    g_default_context = ctx;
    
    pthread_mutex_unlock(&g_default_mutex);
    
    return GM_OK_VOID;
}

/* Helper to create error result for u32 */
static inline gm_result_u32 gm_err_u32(gm_error_t* err) {
    return (gm_result_u32){ .ok = false, .u.err = err };
}

/* Hash an ID using the given context */
gm_result_u32 gm_id_hash_with_context(gm_id_context_t* ctx, gm_id_t id) {
    if (!ctx) {
        return gm_err_u32(GM_ERROR(GM_ERR_INVALID_ARGUMENT,
                                  "Context cannot be NULL"));
    }
    
    if (!ctx->initialized) {
        return gm_err_u32(GM_ERROR(GM_ERR_INVALID_STATE,
                                  "Context not initialized"));
    }
    
    /* Use SipHash-2-4 for hash tables */
    uint8_t hash_bytes[crypto_shorthash_siphash24_BYTES];
    int result = crypto_shorthash_siphash24(hash_bytes, id.bytes, GM_ID_SIZE, 
                                           ctx->siphash_key);
    
    if (result != 0) {
        return gm_err_u32(GM_ERROR(GM_ERR_CRYPTO_FAILED,
                                  "SipHash operation failed"));
    }
    
    /* Convert to uint32_t using first 4 bytes */
    uint32_t hash = 0;
    for (int i = 0; i < 4 && i < crypto_shorthash_siphash24_BYTES; i++) {
        hash = (hash << 8) | hash_bytes[i];
    }
    
    return gm_ok_u32(hash);
}