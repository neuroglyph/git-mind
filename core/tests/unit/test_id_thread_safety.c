/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

#include "gitmind/crypto/backend.h"
#include "gitmind/error.h"
#include "gitmind/types/id.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_THREADS 10
#define HASHES_PER_THREAD 1000

typedef struct {
    int thread_id;
    uint32_t *hashes;
} thread_data_t;

/* Thread function that generates many hashes */
static void *hash_thread(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;

    /* Create a unique ID for this thread */
    gm_id_t id;
    for (int i = 0; i < GM_ID_SIZE; i++) {
        id.bytes[i] = (uint8_t)((data->thread_id + i * 13) % 256);
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
        int result =
            pthread_create(&threads[i], NULL, hash_thread, &thread_data[i]);
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
            /* This is probabilistic but extremely unlikely to fail */
            int different = 0;
            for (int j = 0; j < i; j++) {
                if (thread_data[i].hashes[0] != thread_data[j].hashes[0]) {
                    different = 1;
                    break;
                }
            }
            assert(different);
        }
    }

    /* Cleanup */
    for (int i = 0; i < NUM_THREADS; i++) {
        free(thread_data[i].hashes);
    }

    printf("✓ test_concurrent_hashing\n");
}

/* Thread function for ID generation */
static void *generate_thread(void *arg) {
    gm_id_t *ids = (gm_id_t *)arg;

    for (int i = 0; i < 100; i++) {
        gm_result_id result = gm_id_generate();
        if (GM_IS_OK(result)) {
            ids[i] = GM_UNWRAP(result);
        } else {
            gm_error_free(GM_UNWRAP_ERR(result));
            /* Fill with zeros on error */
            for (int j = 0; j < GM_ID_SIZE; j++) {
                ids[i].bytes[j] = 0;
            }
        }
    }

    return NULL;
}

/* Test that ID generation is thread-safe */
static void test_concurrent_generation(void) {
    pthread_t threads[NUM_THREADS];
    gm_id_t thread_ids[NUM_THREADS][100];

    /* Create threads */
    for (int i = 0; i < NUM_THREADS; i++) {
        int ret =
            pthread_create(&threads[i], NULL, generate_thread, thread_ids[i]);
        assert(ret == 0);
    }

    /* Wait for threads */
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    /* Verify all IDs are unique */
    for (int i = 0; i < NUM_THREADS; i++) {
        for (int j = 0; j < 100; j++) {
            /* Check against all previous IDs */
            for (int ti = 0; ti <= i; ti++) {
                int max_j = (ti == i) ? j : 100;
                for (int tj = 0; tj < max_j; tj++) {
                    if (ti != i || tj != j) {
                        assert(
                            !gm_id_equal(thread_ids[i][j], thread_ids[ti][tj]));
                    }
                }
            }
        }
    }

    printf("✓ test_concurrent_generation\n");
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
        thread_data[i].hashes = calloc(HASHES_PER_THREAD, sizeof(uint32_t));
        assert(thread_data[i].hashes != NULL);
    }

    /* Start all threads at once to maximize race condition likelihood */
    for (int i = 0; i < NUM_THREADS; i++) {
        int result =
            pthread_create(&threads[i], NULL, hash_thread, &thread_data[i]);
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

    /* Initialize crypto subsystem */
    gm_result_void init_result = gm_crypto_init();
    if (GM_IS_ERR(init_result)) {
        gm_error_print(GM_UNWRAP_ERR(init_result));
        return 1;
    }

    test_concurrent_hashing();
    test_concurrent_generation();
    test_initialization_race();

    printf("\n⚠️  Note: These tests check basic thread safety but cannot\n");
    printf("    guarantee absence of all race conditions. The global\n");
    printf("    state in id.c should be refactored to use proper\n");
    printf("    synchronization or context objects.\n");

    printf("\n✅ All thread safety tests passed!\n");
    return 0;
}