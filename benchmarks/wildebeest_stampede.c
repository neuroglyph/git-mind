/* SPDX-License-Identifier: LicenseRef-MIND-UCAL-1.0 */
/* © 2025 J. Kirby Ross / Neuroglyph Collective */

/*
 * WILDEBEEST STAMPEDE BENCHMARK 🦬⚡
 *
 * "Start the stampede..." - Scar
 *
 * Just as Disney invented AI flocking for the wildebeest scene,
 * we use Roaring Bitmaps to handle massive edge queries.
 *
 * This benchmark creates a stampede of edges and measures
 * how fast our cache can dodge them.
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../include/gitmind.h"
#include "gitmind/adapters/fs/posix_temp_adapter.h"
#include "gitmind/adapters/git/libgit2_repository_port.h"
#include "gitmind/result.h"

#define WILDEBEEST_COUNT 100000 /* Start with 100K edges */
#define STAMPEDE_ROUNDS 10      /* Multiple runs */
#define MUFASA_NODE "README.md" /* Poor Mufasa... */

/* Colors for output */
#define RED "\033[0;31m"
#define GREEN "\033[0;32m"
#define YELLOW "\033[0;33m"
#define BLUE "\033[0;34m"
#define RESET "\033[0m"

/* Benchmark results */
typedef struct {
    double journal_scan_ms;
    double cache_query_ms;
    double speedup;
    size_t edges_found;
} stampede_result_t;

/* Generate random SHA for wildebeest */
static void generate_wildebeest_sha(uint8_t *sha) {
    for (int i = 0; i < GM_SHA1_SIZE; i++) {
        sha[i] = rand() & 0xFF;
    }
}

/* Run the stampede! */
static void run_stampede(git_repository *repo, const char *branch,
                         int wildebeest_count, stampede_result_t *result) {
    printf("\n" YELLOW "🦬 STARTING STAMPEDE with %d wildebeest!" RESET "\n",
           wildebeest_count);

    /* Create edges (wildebeest) */
    printf("Creating wildebeest edges...\n");
    gm_edge_t *herd = malloc(wildebeest_count * sizeof(gm_edge_t));

    /* Mufasa's SHA (he's at the center of it all) */
    uint8_t mufasa_sha[GM_SHA1_SIZE];
    generate_wildebeest_sha(mufasa_sha);

    /* Generate the herd */
    for (int i = 0; i < wildebeest_count; i++) {
        gm_edge_t *beast = &herd[i];

        /* Half run TO Mufasa, half run FROM */
        if (i % 2 == 0) {
            generate_wildebeest_sha(beast->src_sha);
            memcpy(beast->tgt_sha, mufasa_sha, GM_SHA1_SIZE);
        } else {
            memcpy(beast->src_sha, mufasa_sha, GM_SHA1_SIZE);
            generate_wildebeest_sha(beast->tgt_sha);
        }

        beast->rel_type = GM_REL_DEPENDS_ON; /* They all depend on the king */
        beast->confidence = 0x3C00;          /* 1.0 */
        beast->timestamp = time(NULL) * 1000 + i;
        snprintf(beast->src_path, GM_PATH_MAX, "wildebeest_%d.md", i);
        snprintf(beast->tgt_path, GM_PATH_MAX, "target_%d.md", i);
    }

    /* Add to journal */
    printf("Releasing the herd into the journal...\n");
    gm_context_t ctx = (gm_context_t){0};

    gm_result_void_t repo_port_result =
        gm_libgit2_repository_port_create(&ctx.git_repo_port, NULL,
                                          &ctx.git_repo_port_dispose, repo);
    if (!repo_port_result.ok) {
        fprintf(stderr, "failed to create git repository port\n");
        exit(EXIT_FAILURE);
    }

    gm_result_void_t fs_result =
        gm_posix_fs_temp_port_create(&ctx.fs_temp_port, NULL,
                                     &ctx.fs_temp_port_dispose);
    if (!fs_result.ok) {
        fprintf(stderr, "failed to create fs temp port\n");
        exit(EXIT_FAILURE);
    }

    /* Batch insert for efficiency */
    for (int i = 0; i < wildebeest_count; i += 100) {
        int batch_size =
            (i + 100 < wildebeest_count) ? 100 : (wildebeest_count - i);
        gm_journal_append(&ctx, &herd[i], batch_size);
    }

    /* Benchmark 1: Journal scan (no cache) */
    printf("\n" RED "⚡ MUFASA WITHOUT CACHE (journal scan):" RESET "\n");
    clock_t start = clock();

    gm_cache_result_t scan_result = {0};
    gm_cache_query_fanout(&ctx, branch, mufasa_sha, &scan_result);

    clock_t end = clock();
    result->journal_scan_ms = ((double)(end - start) / CLOCKS_PER_SEC) * 1000;
    result->edges_found = scan_result.count;

    printf("Found %zu edges in %.2f ms\n", scan_result.count,
           result->journal_scan_ms);
    gm_cache_result_free(&scan_result);

    /* Build cache - "Simba returns!" */
    printf("\n" BLUE "🦁 SIMBA BUILDS THE CACHE..." RESET "\n");
    gm_cache_rebuild(&ctx, branch, true);

    /* Benchmark 2: Cache query */
    printf("\n" GREEN "⚡ SIMBA WITH ROARING CACHE:" RESET "\n");
    start = clock();

    gm_cache_result_t cache_result = {0};
    gm_cache_query_fanout(&ctx, branch, mufasa_sha, &cache_result);

    end = clock();
    result->cache_query_ms = ((double)(end - start) / CLOCKS_PER_SEC) * 1000;

    printf("Found %zu edges in %.2f ms\n", cache_result.count,
           result->cache_query_ms);
    printf("From cache: %s\n", cache_result.from_cache ? "YES 🦁" : "NO 😢");
    gm_cache_result_free(&cache_result);

    /* Calculate speedup */
    result->speedup = result->journal_scan_ms / result->cache_query_ms;

    if (ctx.fs_temp_port_dispose != NULL) {
        ctx.fs_temp_port_dispose(&ctx.fs_temp_port);
    }
    if (ctx.git_repo_port_dispose != NULL) {
        ctx.git_repo_port_dispose(&ctx.git_repo_port);
    }
    free(herd);
}

/* Main benchmark */
int main(int argc, char **argv) {
    printf("\n");
    printf("🦬🦬🦬 WILDEBEEST STAMPEDE BENCHMARK 🦬🦬🦬\n");
    printf("==========================================\n");
    printf("Just as Disney invented AI flocking for the stampede,\n");
    printf("we use Roaring Bitmaps for massive edge queries!\n");

    /* Initialize */
    git_libgit2_init();
    git_repository *repo = NULL;

    /* Create temp repo */
    char temp_path[] = "/tmp/stampede_XXXXXX";
    if (!mkdtemp(temp_path)) {
        fprintf(stderr, "Failed to create temp directory\n");
        return 1;
    }

    /* Initialize repo */
    if (git_repository_init(&repo, temp_path, 0) < 0) {
        fprintf(stderr, "Failed to init repository\n");
        return 1;
    }

    /* Configure git */
    git_config *config;
    git_repository_config(&config, repo);
    git_config_set_string(config, "user.name", "Mufasa");
    git_config_set_string(config, "user.email", "king@pridelands.gov");
    git_config_free(config);

    /* Run stampedes of increasing size */
    int sizes[] = {1000, 10000, 100000};
    stampede_result_t results[3];

    for (int i = 0; i < 3; i++) {
        run_stampede(repo, "main", sizes[i], &results[i]);
    }

    /* Print results */
    printf("\n" YELLOW "🏆 STAMPEDE RESULTS:" RESET "\n");
    printf("================================================\n");
    printf("Edges  | Journal Scan | Cache Query | Speedup\n");
    printf("-------|--------------|-------------|----------\n");

    for (int i = 0; i < 3; i++) {
        printf("%6d | %9.2f ms | %8.2f ms | %6.1fx\n", sizes[i],
               results[i].journal_scan_ms, results[i].cache_query_ms,
               results[i].speedup);
    }

    printf("================================================\n");

    /* The moral */
    printf("\n" GREEN "✨ CIRCLE OF LIFE COMPLETE ✨" RESET "\n");
    printf("With Roaring Bitmaps, we handle stampedes at O(log N)!\n");
    printf("No TODOs were harmed in the making of this benchmark.\n");
    printf("\n\"Remember who you are...\" - A developer with a working cache! "
           "🦁\n\n");

    /* Cleanup */
    git_repository_free(repo);
    git_libgit2_shutdown();

    /* Remove temp dir */
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "rm -rf %s", temp_path);
    system(cmd);

    return 0;
}
