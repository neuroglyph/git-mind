/* SPDX-License-Identifier: Apache-2.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

/**
 * Example: Using GitMind as a library
 * 
 * Compile with:
 *   cc -o example library_usage.c -lgitmind -lgit2
 */

#include <gitmind_lib.h>
#include <stdio.h>
#include <stdlib.h>

/* Custom backend example - stores links in memory */
typedef struct {
    gm_link_t* links;
    size_t count;
    size_t capacity;
} memory_backend_data_t;

static int memory_hash_object(void* backend_data, const void* data, size_t len, 
                             const char* type, char* out_sha) {
    /* Simple fake hash for demo */
    snprintf(out_sha, 41, "memory_%08zx", len);
    return GM_OK;
}

static int memory_update_ref(void* backend_data, const char* ref_name, 
                            const char* new_sha, const char* message) {
    printf("[Memory Backend] Updated ref %s to %s: %s\n", ref_name, new_sha, message);
    return GM_OK;
}

/* ... implement other ops ... */

static gm_backend_ops_t memory_backend = {
    .hash_object = memory_hash_object,
    .update_ref = memory_update_ref,
    /* ... other ops ... */
    .data = NULL  /* Would point to memory_backend_data_t */
};

/* Example 1: Using default libgit2 backend */
void example_default_backend(void) {
    printf("=== Example 1: Default Backend ===\n");
    
    /* Create context with default backend */
    gm_context_t* ctx = gm_create_context(NULL);
    if (!ctx) {
        fprintf(stderr, "Failed to create context\n");
        return;
    }
    
    /* Initialize in current directory */
    if (gm_init(ctx, ".") != GM_OK) {
        fprintf(stderr, "Init failed: %s\n", gm_last_error(ctx));
        gm_destroy_context(ctx);
        return;
    }
    
    /* Create a link */
    if (gm_link_create(ctx, "README.md", "docs/api.md", "documents") != GM_OK) {
        fprintf(stderr, "Link failed: %s\n", gm_last_error(ctx));
    } else {
        printf("Link created successfully\n");
    }
    
    /* List all links */
    gm_link_set_t* links = NULL;
    if (gm_link_list(ctx, &links, NULL, NULL) == GM_OK) {
        printf("Found %zu links:\n", links->count);
        for (size_t i = 0; i < links->count; i++) {
            printf("  %s -> %s (%s)\n", 
                   links->links[i].source,
                   links->links[i].target,
                   links->links[i].type);
        }
        gm_link_set_free(links);
    }
    
    gm_destroy_context(ctx);
}

/* Example 2: Using custom backend */
void example_custom_backend(void) {
    printf("\n=== Example 2: Custom Backend ===\n");
    
    /* Create context with custom backend */
    gm_context_t* ctx = gm_create_context(&memory_backend);
    if (!ctx) {
        fprintf(stderr, "Failed to create context\n");
        return;
    }
    
    /* Operations will use our memory backend */
    if (gm_init(ctx, ":memory:") != GM_OK) {
        fprintf(stderr, "Init failed: %s\n", gm_last_error(ctx));
    }
    
    /* Create link - will call our custom hash_object */
    if (gm_link_create(ctx, "file1.c", "file2.c", "includes") == GM_OK) {
        printf("Link created in memory backend\n");
    }
    
    gm_destroy_context(ctx);
}

/* Example 3: Using test backend for unit tests */
void example_test_backend(void) {
    printf("\n=== Example 3: Test Backend ===\n");
    
    /* Get the test backend */
    const gm_backend_ops_t* test_backend = gm_backend_test();
    gm_context_t* ctx = gm_create_context(test_backend);
    
    /* Test backend always succeeds, returns predictable values */
    if (gm_init(ctx, "test://repo") == GM_OK) {
        printf("Test repo initialized\n");
    }
    
    /* Create links - no actual Git operations */
    gm_link_create(ctx, "test1.c", "test2.c", "tests");
    gm_link_create(ctx, "test2.c", "test3.c", "tests");
    
    /* Traverse - test backend returns fake but consistent data */
    printf("Traversing from test1.c:\n");
    gm_traverse(ctx, "test1.c", 2, 
        [](const gm_link_t* link, int level, void* userdata) {
            for (int i = 0; i < level; i++) printf("  ");
            printf("%s -> %s\n", link->source, link->target);
        }, NULL);
    
    gm_destroy_context(ctx);
}

/* Example 4: Error handling */
void example_error_handling(void) {
    printf("\n=== Example 4: Error Handling ===\n");
    
    gm_context_t* ctx = gm_create_context(NULL);
    
    /* Try to create link without init */
    if (gm_link_create(ctx, "a", "b", "test") != GM_OK) {
        printf("Expected error: %s\n", gm_last_error(ctx));
        gm_clear_error(ctx);
    }
    
    /* Try invalid paths */
    if (gm_link_create(ctx, "../../../etc/passwd", "b", "hack") != GM_OK) {
        printf("Path validation error: %s\n", gm_last_error(ctx));
    }
    
    gm_destroy_context(ctx);
}

int main(void) {
    printf("GitMind Library Examples - Version %s\n\n", gm_version_string());
    
    example_default_backend();
    example_custom_backend();
    example_test_backend();
    example_error_handling();
    
    return 0;
}