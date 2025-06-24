/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/types/id.h"
#include "gitmind/error.h"
#include <assert.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

#define NUM_THREADS 10
#define HASHES_PER_THREAD 1000

typedef struct {
    int thread_id;
    uint32_t* hashes;
} thread_data_t;

/* Thread function that generates many hashes */
static void* hash_thread(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    
    /* Create a unique ID for this thread */
    gm_id_t id;
    for (int i = 0; i < GM_ID_SIZE; i++) {
        id.bytes[i] = (uint8_t)((data->thread_id * 256 + i) % 256);
    }
    
    /* Generate many hashes */
    for (int i = 0; i < HASHES_PER_THREAD; i++) {
        gm_result_u32 result = gm_id_hash(id);
        if (GM_IS_OK(result)) {
            data->hashes[i] = GM_UNWRAP(result);
        } else {
            data->hashes[i] = 0;
            gm_error_free(GM_UNWRAP_ERR(result));
        }
    }
    
    return NULL;
}

/* Test that multiple threads can safely hash IDs */
static void test_concurrent_hashing(void) {
    pthread_t threads[NUM_THREADS];
    thread_data_t thread_data[NUM_THREADS];
    
    /* Allocate hash arrays */
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].hashes = calloc(HASHES_PER_THREAD, sizeof(uint32_t));
        assert(thread_data[i].hashes != NULL);
    }
    
    /* Start threads */
    for (int i = 0; i < NUM_THREADS; i++) {
        int result = pthread_create(&threads[i], NULL, hash_thread, &thread_data[i]);
        assert(result == 0);
    }
    
    /* Wait for threads */
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    /* Verify results */
    for (int i = 0; i < NUM_THREADS; i++) {
        /* Each thread should produce consistent hashes */
        uint32_t first_hash = thread_data[i].hashes[0];
        for (int j = 1; j < HASHES_PER_THREAD; j++) {
            assert(thread_data[i].hashes[j] == first_hash);
        }
        
        /* Different threads should produce different hashes (different IDs) */
        if (i > 0) {
            assert(thread_data[i].hashes[0] != thread_data[0].hashes[0]);
        }
    }
    
    /* Cleanup */
    for (int i = 0; i < NUM_THREADS; i++) {
        free(thread_data[i].hashes);
    }
    
    printf("✓ test_concurrent_hashing\n");
}

/* Test race condition on initialization */
static void test_initialization_race(void) {
    /* This test attempts to trigger the race condition in 
     * g_siphash_key_initialized by having multiple threads
     * call gm_id_hash simultaneously on first use */
    
    pthread_t threads[NUM_THREADS];
    thread_data_t thread_data[NUM_THREADS];
    
    /* Allocate hash arrays */
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].hashes = calloc(1, sizeof(uint32_t));
        assert(thread_data[i].hashes != NULL);
    }
    
    /* Start all threads at once to maximize race condition likelihood */
    for (int i = 0; i < NUM_THREADS; i++) {
        int result = pthread_create(&threads[i], NULL, hash_thread, &thread_data[i]);
        assert(result == 0);
    }
    
    /* Wait for threads */
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    /* If we get here without crashing, the implementation handled
     * concurrent initialization (though it may still have race conditions) */
    
    /* Cleanup */
    for (int i = 0; i < NUM_THREADS; i++) {
        free(thread_data[i].hashes);
    }
    
    printf("✓ test_initialization_race (basic check passed)\n");
}

int main(void) {
    printf("Running ID thread safety tests...\n\n");
    
    test_concurrent_hashing();
    test_initialization_race();
    
    printf("\n⚠️  Note: These tests check basic thread safety but cannot\n");
    printf("    guarantee absence of all race conditions. The global\n");
    printf("    state in id.c should be refactored to use proper\n");
    printf("    synchronization or context objects.\n");
    
    printf("\n✅ All thread safety tests passed!\n");
    return 0;
}